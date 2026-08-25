/**
 *
 * @file
 *
 * @brief  XMP support plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/external/xmp.h"

#include "formats/chiptune/common/decoder.h"
#include "module/players/properties_helper.h"

#include "core/core_parameters.h"
#include "parameters/tracking_helper.h"
#include "strings/sanitize.h"
#include "time/duration.h"

#include "contract.h"
#include "make_ptr.h"

#include "3rdparty/xmp/include/xmp.h"
#include "3rdparty/xmp/src/xmp_private.h"

#include <utility>

namespace Module::Xmp
{
  class BaseContext
  {
  public:
    BaseContext()
      : Data(::xmp_create_context())
    {}

    ~BaseContext()
    {
      ::xmp_free_context(Data);
    }

    BaseContext(const BaseContext& rh) = delete;
    BaseContext& operator=(const BaseContext& rh) = delete;

    template<class... P>
    void Call(void (*func)(xmp_context, P...), P... p)
    {
      func(Data, p...);
    }

    template<class... P>
    void Call(int (*func)(xmp_context, P...), P... p)
    {
      CheckError(func(Data, p...));
    }

  private:
    static void CheckError(int code)
    {
      // TODO
      Require(code >= 0);
    }

  protected:
    xmp_context Data;
  };

  class Context : public BaseContext
  {
  public:
    using Ptr = std::shared_ptr<Context>;

    Context(const Binary::Container& rawData, const struct format_loader* loader)
    {
      Call(&::xmp_load_typed_module_from_memory, const_cast<void*>(rawData.Start()), static_cast<long>(rawData.Size()),
           loader);
    }

    ~Context()
    {
      ::xmp_release_module(Data);
    }
  };

  using DurationType = Time::Milliseconds;

  Information MakeInformation(xmp_module info, DurationType duration)
  {
    TrackLayout track{.ChannelsCount = static_cast<uint_t>(info.chn),
                      .PositionsCount = static_cast<uint_t>(info.len),
                      .LoopPosition = static_cast<uint_t>(info.rst)};
    return {.Duration = duration, .LoopDuration = duration /*TODO*/, .Track = std::move(track)};
  }

  State MakeTrackState(const xmp_frame_info& info, Time::Microseconds total)
  {
    return {.At = Time::AtMillisecond() + DurationType(info.time),
            .Total = total.CastTo<Time::Millisecond>(),
            .LoopCount = static_cast<uint_t>(info.loop_count),
            .Track = {{.Position = static_cast<uint_t>(info.pos),
                       .Pattern = static_cast<uint_t>(info.pattern),
                       .Line = static_cast<uint_t>(info.row),
                       .Tempo = static_cast<uint_t>(info.speed),
                       .Quirk = static_cast<uint_t>(info.frame),
                       .Channels = static_cast<uint_t>(info.virt_used)}}};
  }

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(uint_t channels, Context::Ptr ctx, uint_t samplerate, Parameters::Accessor::Ptr params)
      : Ctx(std::move(ctx))
      , Params(std::move(params))
      , SoundFreq(samplerate)
      , ChannelsCount(channels)
    {
      // Required in order to perform initial seeking
      Ctx->Call(&::xmp_start_player, static_cast<int>(samplerate), 0);
    }

    ~Renderer() override
    {
      Ctx->Call(&::xmp_end_player);
    }

    Module::State GetState() const override
    {
      return MakeTrackState(State, TotalDuration);
    }

    Sound::Chunk Render() override
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      static_assert(sizeof(Sound::Sample) == 4, "Incompatible sound sample size");

      for (;;)
      {
        ApplyParameters();
        Ctx->Call(&::xmp_play_frame);
        Ctx->Call(&::xmp_get_frame_info, &State);
        if (const std::size_t bytes = State.buffer_size)
        {
          const std::size_t samples = bytes / sizeof(Sound::Sample);
          TotalDuration += Time::Microseconds::FromRatio(samples, SoundFreq);
          Sound::Chunk chunk(samples);
          std::memcpy(chunk.data(), State.buffer, samples * sizeof(Sound::Sample));
          return chunk;
        }
      }
      return {};
    }

    void Reset() override
    {
      Params.Reset();
      Ctx->Call(&::xmp_restart_module);
      TotalDuration = {};
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      static_assert(request.PER_SECOND == DurationType::PER_SECOND, "Fail");
      Ctx->Call(&::xmp_seek_time, int(request.Get()));
    }

  private:
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        using namespace Parameters::ZXTune::Core;
        ApplyInterpolation(Parameters::GetInteger(*Params, DAC::INTERPOLATION, DAC::INTERPOLATION_DEFAULT));
        ApplyMuting(Parameters::GetInteger(*Params, CHANNELS_MASK, CHANNELS_MASK_DEFAULT));
      }
    }

    void ApplyInterpolation(uint_t val)
    {
      const int interpolation = val != Parameters::ZXTune::Core::DAC::INTERPOLATION_NO ? XMP_INTERP_SPLINE
                                                                                       : XMP_INTERP_LINEAR;
      Ctx->Call(&::xmp_set_player, int(XMP_PLAYER_INTERP), interpolation);
    }

    void ApplyMuting(uint_t mask)
    {
      for (uint_t chan = 0; chan != ChannelsCount; ++chan)
      {
        const int mute = (mask & (1 << chan)) ? 1 : 0;
        Ctx->Call(&::xmp_channel_mute, static_cast<int>(chan), mute);
      }
    }

  private:
    const Context::Ptr Ctx;
    xmp_frame_info State;
    Time::Microseconds TotalDuration;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    const uint_t SoundFreq;
    const uint_t ChannelsCount;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Context::Ptr ctx, Information info, Parameters::Accessor::Ptr props)
      : Ctx(std::move(ctx))
      , Info(std::move(info))
      , Properties(std::move(props))
    {}

    Information GetModuleInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      return MakePtr<Renderer>(Info.Track->ChannelsCount, Ctx, samplerate, std::move(params));
    }

  private:
    const Context::Ptr Ctx;
    const Information Info;
    const Parameters::Accessor::Ptr Properties;
  };

  void ParseStrings(const xmp_module& mod, PropertiesHelper& props)
  {
    Strings::Array strings;
    for (int idx = 0; idx < mod.smp; ++idx)
    {
      strings.emplace_back(Strings::SanitizeKeepPadding(mod.xxs[idx].name));
    }
    for (int idx = 0; idx < mod.ins; ++idx)
    {
      strings.emplace_back(Strings::SanitizeKeepPadding(mod.xxi[idx].name));
    }
    props.SetStrings(strings);
  }

  class Factory : public ExternalParsingFactory
  {
  public:
    explicit Factory(const format_loader* loader)
      : Loader(loader)
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/,
                                     const Formats::Chiptune::Container& container,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        auto ctx = MakePtr<Context>(container, Loader);
        xmp_module_info modInfo;
        ctx->Call(&::xmp_get_module_info, &modInfo);
        xmp_frame_info frmInfo;
        ctx->Call(&::xmp_get_frame_info, &frmInfo);

        PropertiesHelper props(*properties);
        props.SetTitle(Strings::Sanitize(modInfo.mod->name));
        props.SetAuthor(Strings::Sanitize(modInfo.mod->author));
        props.SetProgram(Strings::Sanitize(modInfo.mod->type));
        if (const char* comment = modInfo.comment)
        {
          props.SetComment(Strings::SanitizeMultiline(comment));
        }
        ParseStrings(*modInfo.mod, props);
        props.SetChannels("Ch"sv, modInfo.mod->chn);
        auto info = MakeInformation(*modInfo.mod, DurationType(frmInfo.total_time));
        return MakePtr<Holder>(std::move(ctx), std::move(info), std::move(properties));
      }
      catch (const std::exception&)
      {}
      return {};
    }

  private:
    const format_loader* const Loader;
  };

  struct Description
  {
    const StringView Format;
    const struct format_loader* const Loader;
  };

  // clang-format off
  //Desktop Tracker
  const Description DTT_DESC =
  {
    "'D's'k'T"sv,
    &dtt_loader
  };

  //Quadra Composer
  const Description EMOD_DESC =
  {
    "'F'O'R'M"
    "????"
    "'E'M'O'D"
    ""sv,
    &emod_loader
  };

  //Funktracker
  const Description FNK_DESC =
  {
    "'F'u'n'k"
    "?"
    "14-ff"     //(year-1980)*2
    "00-79"     //cpu and card (really separate)
    "?"
    ""sv,
    &fnk_loader
  };

  //Liquid Tracker
  const Description LIQ_DESC =
  {
    "'L'i'q'u'i'd' 'M'o'd'u'l'e':"
    ""sv,
    &liq_loader
  };

  //MED 1.12 MED2
  const Description MED2_DESC =
  {
    "'M'E'D"
    "02"
    ""sv,
    &med2_loader
  };

  //MED 2.00 MED3
  const Description MED3_DESC =
  {
    "'M'E'D"
    "03"
    ""sv,
    &med3_loader
  };

  const Description MED4_DESC =
  {
    "'M'E'D"
    "04"
    ""sv,
    &med4_loader
  };

  //Liquid Tracker NO
  const Description LIQ_NO_DESC =
  {
    "'N'O"
    "0000"
    ""sv,
    &no_loader
  };

  //Slamtilt
  const Description STIM_DESC =
  {
    "'S'T'I'M"         //signature
    "00???"            //BE samples offsets (assume 16Mb is enough)
    "?{8}"             //unknown
    "00?"              //BE number of samples (assume 255 is enough)
    "0001-80"          //BE count of positions (1-128)
    "0001-80"          //BE count of saved patterns (1-128)
    ""sv,
    &stim_loader
  };

  //STMIK 0.2
  const Description STX_DESC =
  {
    "?{20}"
    "('!|'B)"
    "('S|'M)"
    "('c|'O)"
    "('r|'D)"
    "('e|'2)"
    "('a|'S)"
    "('m|'T)"
    "('!|'M)"
    //+28
    "?{32}"
    //+60
    "'S'C'R'M"
    ""sv,
    &stx_loader
  };

  // known but not implemented formats
  //{"ARCH"_id, &arch_loader},
  //{"COCO"_id, &coco_loader},
  //{"MFP"_id, &mfp_loader},//requires additional files
  //{"MGT"_id, &mgt_loader},experimental
  //{"MOD"_id, &polly_loader},//rle packed, too weak structure
  //{"MOD"_id, &pw_loader},//requires depacking
  //{"MTP"_id, &mtp_loader},//experimental
  //{"SYM"_id, &sym_loader},
  // clang-format on

  ExternalParsingFactory::Ptr CreateDTTFactory()
  {
    return MakePtr<Factory>(DTT_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateEMODFactory()
  {
    return MakePtr<Factory>(EMOD_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateFNKFactory()
  {
    return MakePtr<Factory>(FNK_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateLIQFactory()
  {
    return MakePtr<Factory>(LIQ_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateMED2Factory()
  {
    return MakePtr<Factory>(MED2_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateMED3Factory()
  {
    return MakePtr<Factory>(MED3_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateMED4Factory()
  {
    return MakePtr<Factory>(MED4_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateNOFactory()
  {
    return MakePtr<Factory>(LIQ_NO_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateSTIMFactory()
  {
    return MakePtr<Factory>(STIM_DESC.Loader);
  }

  ExternalParsingFactory::Ptr CreateSTXFactory()
  {
    return MakePtr<Factory>(STX_DESC.Loader);
  }
}  // namespace Module::Xmp

namespace Formats::Chiptune
{
  Decoder::Ptr CreateDecoder(const Module::Xmp::Description& desc)
  {
    return CreateFormatDecoder(desc.Format, ::xmp_get_loader_name(desc.Loader));
  }

  Decoder::Ptr CreateDTTDecoder()
  {
    return CreateDecoder(Module::Xmp::DTT_DESC);
  }

  Decoder::Ptr CreateEMODDecoder()
  {
    return CreateDecoder(Module::Xmp::EMOD_DESC);
  }

  Decoder::Ptr CreateFNKDecoder()
  {
    return CreateDecoder(Module::Xmp::FNK_DESC);
  }

  Decoder::Ptr CreateLIQDecoder()
  {
    return CreateDecoder(Module::Xmp::LIQ_DESC);
  }

  Decoder::Ptr CreateMED2Decoder()
  {
    return CreateDecoder(Module::Xmp::MED2_DESC);
  }

  Decoder::Ptr CreateMED3Decoder()
  {
    return CreateDecoder(Module::Xmp::MED3_DESC);
  }

  Decoder::Ptr CreateMED4Decoder()
  {
    return CreateDecoder(Module::Xmp::MED4_DESC);
  }

  Decoder::Ptr CreateNODecoder()
  {
    return CreateDecoder(Module::Xmp::LIQ_NO_DESC);
  }

  Decoder::Ptr CreateSTIMDecoder()
  {
    return CreateDecoder(Module::Xmp::STIM_DESC);
  }

  Decoder::Ptr CreateSTXDecoder()
  {
    return CreateDecoder(Module::Xmp::STX_DESC);
  }
}  // namespace Formats::Chiptune
