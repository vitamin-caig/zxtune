/**
 *
 * @file
 *
 * @brief  XMP support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <core/core_parameters.h>
#include <core/plugin_attrs.h>
#include <formats/chiptune/container.h>
#include <module/players/properties_helper.h>
#include <module/track_information.h>
#include <module/track_state.h>
#include <parameters/tracking_helper.h>
#include <strings/encoding.h>
#include <strings/trim.h>
#include <time/duration.h>
// std includes
#include <utility>
// 3rdparty includes
#define BUILDING_STATIC
#include <3rdparty/xmp/include/xmp.h>
#include <3rdparty/xmp/src/xmp_private.h>

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

    void Call(void (*func)(xmp_context))
    {
      func(Data);
    }

    template<class P1>
    void Call(void (*func)(xmp_context, P1), P1 p1)
    {
      func(Data, p1);
    }

    void Call(int (*func)(xmp_context))
    {
      CheckError(func(Data));
    }

    template<class P1>
    void Call(int (*func)(xmp_context, P1), P1 p1)
    {
      CheckError(func(Data, p1));
    }

    template<class P1, class P2>
    void Call(int (*func)(xmp_context, P1, P2), P1 p1, P2 p2)
    {
      CheckError(func(Data, p1, p2));
    }

    template<class P1, class P2, class P3>
    void Call(int (*func)(xmp_context, P1, P2, P3), P1 p1, P2 p2, P3 p3)
    {
      CheckError(func(Data, p1, p2, p3));
    }

  private:
    BaseContext(const BaseContext& rh);
    void operator=(const BaseContext& rh);

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
    typedef std::shared_ptr<Context> Ptr;

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

  class Information : public Module::TrackInformation
  {
  public:
    typedef std::shared_ptr<const Information> Ptr;

    Information(xmp_module module, DurationType duration)
      : Info(std::move(module))
      , TotalDuration(duration)
    {}

    Time::Milliseconds Duration() const override
    {
      return TotalDuration.CastTo<Time::Millisecond>();
    }

    Time::Milliseconds LoopDuration() const override
    {
      return Duration();  // TODO
    }

    uint_t PositionsCount() const override
    {
      return Info.len;
    }

    uint_t LoopPosition() const override
    {
      return Info.rst;
    }

    uint_t ChannelsCount() const override
    {
      return Info.chn;
    }

  private:
    const xmp_module Info;
    const DurationType TotalDuration;
  };

  typedef std::shared_ptr<xmp_frame_info> StatePtr;

  class TrackState : public Module::TrackState
  {
  public:
    using Ptr = std::shared_ptr<TrackState>;

    TrackState(StatePtr state)
      : State(std::move(state))
    {}

    Time::AtMillisecond At() const override
    {
      return Time::AtMillisecond() + DurationType(State->time);
    }

    Time::Milliseconds Total() const override
    {
      return TotalDuration.CastTo<Time::Millisecond>();
    }

    uint_t LoopCount() const override
    {
      return State->loop_count;
    }

    uint_t Position() const override
    {
      return State->pos;
    }

    uint_t Pattern() const override
    {
      return State->pattern;
    }

    uint_t Line() const override
    {
      return State->row;
    }

    uint_t Tempo() const override
    {
      return State->speed;
    }

    uint_t Quirk() const override
    {
      return State->frame;  //???
    }

    uint_t Channels() const override
    {
      return State->virt_used;  //????
    }

    void Add(Time::Microseconds played)
    {
      TotalDuration += played;
    }

    void Reset()
    {
      TotalDuration = {};
    }

  private:
    const StatePtr State;
    Time::Microseconds TotalDuration;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(uint_t channels, Context::Ptr ctx, uint_t samplerate, Parameters::Accessor::Ptr params)
      : Ctx(std::move(ctx))
      , State(new xmp_frame_info())
      , Params(std::move(params))
      , Track(MakePtr<TrackState>(State))
      , SoundFreq(samplerate)
    {
      // Required in order to perform initial seeking
      Ctx->Call(&::xmp_start_player, static_cast<int>(samplerate), 0);
    }

    ~Renderer() override
    {
      Ctx->Call(&::xmp_end_player);
    }

    Module::State::Ptr GetState() const override
    {
      return Track;
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
        Ctx->Call(&::xmp_get_frame_info, State.get());
        if (const std::size_t bytes = State->buffer_size)
        {
          const std::size_t samples = bytes / sizeof(Sound::Sample);
          Track->Add(Time::Microseconds::FromRatio(samples, SoundFreq));
          Sound::Chunk chunk(samples);
          std::memcpy(chunk.data(), State->buffer, samples * sizeof(Sound::Sample));
          return chunk;
        }
      }
      return {};
    }

    void Reset() override
    {
      Params.Reset();
      Ctx->Call(&::xmp_restart_module);
      Track->Reset();
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
        Parameters::IntType val = Parameters::ZXTune::Core::DAC::INTERPOLATION_DEFAULT;
        Params->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, val);
        const int interpolation = val != Parameters::ZXTune::Core::DAC::INTERPOLATION_NO ? XMP_INTERP_SPLINE
                                                                                         : XMP_INTERP_LINEAR;
        Ctx->Call(&::xmp_set_player, int(XMP_PLAYER_INTERP), interpolation);
      }
    }

  private:
    const Context::Ptr Ctx;
    const StatePtr State;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    const TrackState::Ptr Track;
    const uint_t SoundFreq;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Context::Ptr ctx, Information::Ptr info, Parameters::Accessor::Ptr props)
      : Ctx(std::move(ctx))
      , Info(std::move(info))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return Info;
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      return MakePtr<Renderer>(Info->ChannelsCount(), Ctx, samplerate, std::move(params));
    }

  private:
    const Context::Ptr Ctx;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };

  struct PluginDescription
  {
    const ZXTune::PluginId Id;
    const StringView Format;
    const struct format_loader* const Loader;
  };

  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    explicit Decoder(const PluginDescription& desc)
      : Desc(desc)
      , Fmt(Binary::CreateMatchOnlyFormat(Desc.Format))
    {}

    String GetDescription() const override
    {
      return xmp_get_loader_name(Desc.Loader);
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Fmt;
    }

    bool Check(Binary::View rawData) const override
    {
      return Fmt->Match(rawData);
    }

    Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (Check(rawData))
      {
        const auto size = rawData.Size();
        auto data = rawData.GetSubcontainer(0, size);
        return Formats::Chiptune::CreateCalculatingCrcContainer(std::move(data), 0, size);
      }
      return {};
    }

  private:
    const PluginDescription& Desc;
    const Binary::Format::Ptr Fmt;
  };

  String DecodeString(StringView str)
  {
    return Strings::ToAutoUtf8(Strings::TrimSpaces(str));
  }

  void ParseStrings(const xmp_module& mod, PropertiesHelper& props)
  {
    Strings::Array strings;
    for (int idx = 0; idx < mod.smp; ++idx)
    {
      strings.push_back(DecodeString(mod.xxs[idx].name));
    }
    for (int idx = 0; idx < mod.ins; ++idx)
    {
      strings.push_back(DecodeString(mod.xxi[idx].name));
    }
    props.SetStrings(strings);
  }

  class Factory : public Module::ExternalParsingFactory
  {
  public:
    explicit Factory(const PluginDescription& desc)
      : Desc(desc)
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/,
                                     const Formats::Chiptune::Container& container,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        auto ctx = MakePtr<Context>(container, Desc.Loader);
        xmp_module_info modInfo;
        ctx->Call(&::xmp_get_module_info, &modInfo);
        xmp_frame_info frmInfo;
        ctx->Call(&::xmp_get_frame_info, &frmInfo);

        PropertiesHelper props(*properties);
        props.SetTitle(DecodeString(modInfo.mod->name));
        props.SetAuthor(DecodeString(modInfo.mod->author));
        props.SetProgram(DecodeString(modInfo.mod->type));
        if (const char* comment = modInfo.comment)
        {
          props.SetComment(DecodeString(comment));
        }
        ParseStrings(*modInfo.mod, props);
        auto info = MakePtr<Information>(*modInfo.mod, DurationType(frmInfo.total_time));
        return MakePtr<Holder>(std::move(ctx), std::move(info), std::move(properties));
      }
      catch (const std::exception&)
      {}
      return {};
    }

  private:
    const PluginDescription& Desc;
  };

  using ZXTune::operator""_id;

  // clang-format off
  const PluginDescription PLUGINS[] =
  {
    //{"ARCH"_id, &arch_loader},
    //{"COCO"_id, &coco_loader},
    //Desktop Tracker
    {
      "DTT"_id
      ,
      "'D's'k'T"_sv
      ,
      &dtt_loader
    },
    //Quadra Composer
    {
      "EMOD"_id
      ,
      "'F'O'R'M"
      "????"
      "'E'M'O'D"
      ""_sv
      ,
      &emod_loader
    },
    //Funktracker
    {
      "FNK"_id
      ,
      "'F'u'n'k"
      "?"
      "14-ff"     //(year-1980)*2
      "00-79"     //cpu and card (really separate)
      "?"
      ""_sv
      ,
      &fnk_loader
    },
    //Graoumf Tracker
    {
      "GTK"_id
      ,
      "'G'T'K"
      "00-03"
      ""_sv
      ,
      &gtk_loader
    },
    //Images Music System
    {
      "IMS"_id
      ,
      "?{20}"
      "("                  //instruments
       "(00|08|20-7f){20}" // name
       "??"                // finetune
       "00-7f?"            // BE size
       "?"                 // unknown
       "00-40"             // volume
       "00-7f?"            // BE loop start
       "??"                // BE loop size
      "){31}"
      "01-7f"   //len
      "00-01"   //zero
      "?{128}"  //orders
      "???3c"   //magic
      ""_sv
      ,
      &ims_loader
    },
    //Liquid Tracker
    {
      "LIQ"_id
      ,
      "'L'i'q'u'i'd' 'M'o'd'u'l'e':"
      ""_sv
      ,
      &liq_loader
    },
    //MED 1.12 MED2
    {
      "MED"_id
      ,
      "'M'E'D"
      "02"
      ""_sv
      ,
      &med2_loader
    },
    //MED 2.00 MED3
    {
      "MED"_id
      ,
      "'M'E'D"
      "03"
      ""_sv
      ,
      &med3_loader
    },
    {
      "MED"_id
      ,
      "'M'E'D"
      "04"
      ""_sv
      ,
      &med4_loader
    },
    //{"MFP"_id, &mfp_loader},//requires additional files
    //{"MGT"_id, &mgt_loader},experimental
    //Liquid Tracker NO
    {
      "LIQ"_id
      ,
      "'N'O"
      "0000"
      ""_sv
      ,
      &no_loader
    },
    //{"MOD"_id, &polly_loader},//rle packed, too weak structure
    //{"MOD"_id, &pw_loader},//requires depacking
    //Real Tracker
    {
      "RTM"_id
      ,
      "'R'T'M'M"
      "20"
      ""_sv
      ,
      &rtm_loader
    },
    //{"MTP"_id, &mtp_loader},//experimental
    //Slamtilt
    {
      "STIM"_id
      ,
      "'S'T'I'M"         //signature
      "00???"            //BE samples offsets (assume 16Mb is enough)
      "?{8}"             //unknown
      "00?"              //BE number of samples (assume 255 is enough)
      "0001-80"          //BE count of positions (1-128)
      "0001-80"          //BE count of saved patterns (1-128)
      ""_sv
      ,
      &stim_loader
    },
    //STMIK 0.2
    {
      "STX"_id
      ,
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
      ""_sv
      ,
      &stx_loader
    },
    //{"SYM"_id, &sym_loader},
    //TCB Tracker
    {
      "TCB"_id
      ,
      "'A'N' 'C'O'O'L('.|'!)"
      ""_sv
      ,
      &tcb_loader
    },
  };
  // clang-format on
}  // namespace Module::Xmp

namespace ZXTune
{
  void RegisterXMPPlugins(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;
    for (const auto& desc : Module::Xmp::PLUGINS)
    {
      auto decoder = MakePtr<Module::Xmp::Decoder>(desc);
      auto factory = MakePtr<Module::Xmp::Factory>(desc);
      auto plugin = CreatePlayerPlugin(desc.Id, CAPS, std::move(decoder), std::move(factory));
      registrator.RegisterPlugin(std::move(plugin));
    }
  }
}  // namespace ZXTune
