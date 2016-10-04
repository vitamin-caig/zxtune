/**
* 
* @file
*
* @brief  XMP support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <contract.h>
//library includes
#include <binary/format_factories.h>
#include <core/core_parameters.h>
#include <core/plugin_attrs.h>
#include <formats/chiptune/container.h>
#include <module/players/properties_helper.h>
#include <parameters/tracking_helper.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
#include <time/stamp.h>
//3rdparty includes
#define BUILDING_STATIC
#include <3rdparty/xmp/include/xmp.h>
#include <3rdparty/xmp/src/xmp_private.h>
//boost includes
#include <boost/range/end.hpp>

namespace Module
{
namespace Xmp
{
  class BaseContext
  {
  public:
    BaseContext()
      : Data(::xmp_create_context())
    {
    }
    
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
    void operator = (const BaseContext& rh);

    static void CheckError(int code)
    {
      //TODO
      Require(code >= 0);
    }
  protected:
    xmp_context Data;
  };

  class Context : public BaseContext
  {
  public:
    typedef boost::shared_ptr<Context> Ptr;

    Context(const Binary::Container& rawData, const struct format_loader* loader)
    {
      Call(&::xmp_load_typed_module_from_memory, const_cast<void*>(rawData.Start()), static_cast<long>(rawData.Size()), loader);
    }

    ~Context()
    {
      ::xmp_release_module(Data);
    }
  };

  typedef Time::Milliseconds TimeType;

  class Information : public Module::Information
  {
  public:
    typedef boost::shared_ptr<const Information> Ptr;

    Information(const xmp_module& module, TimeType duration)
      : Info(module)
      , Frames(duration.Get() / GetFrameDuration().Get())
    {
    }

    virtual uint_t PositionsCount() const
    {
      return Info.len;
    }

    virtual uint_t LoopPosition() const
    {
      return Info.rst;
    }

    virtual uint_t PatternsCount() const
    {
      return Info.pat;
    }

    virtual uint_t FramesCount() const
    {
      return Frames;
    }

    virtual uint_t LoopFrame() const
    {
      return 0;//TODO
    }

    virtual uint_t ChannelsCount() const
    {
      return Info.chn;
    }

    virtual uint_t Tempo() const
    {
      return Info.spd;
    }

    std::vector<uint_t> GetPatternSizes() const
    {
      std::vector<uint_t> res(Info.pat);
      for (uint_t i = 0; i != Info.pat; ++i)
      {
        res[i] = Info.xxp[i]->rows;
      }
      return res;
    }

    TimeType GetFrameDuration() const
    {
      //fps = 50 * bpm / 125
      return TimeType(TimeType::PER_SECOND * 125 / (50 * Info.bpm));
    }
  private:
    const xmp_module Info;
    const int Frames;
  };

  typedef boost::shared_ptr<xmp_frame_info> StatePtr;

  class TrackState : public Module::TrackState
  {
  public:
    TrackState(Information::Ptr info, StatePtr state)
      : PatternSizes(info->GetPatternSizes())
      , FrameDuration(info->GetFrameDuration())
      , State(state)
    {
    }

    virtual uint_t Position() const
    {
      return State->pos;
    }

    virtual uint_t Pattern() const
    {
      return State->pattern;
    }

    virtual uint_t PatternSize() const
    {
      return PatternSizes[State->pattern];
    }

    virtual uint_t Line() const
    {
      return State->row;
    }

    virtual uint_t Tempo() const
    {
      return State->speed;
    }

    virtual uint_t Quirk() const
    {
      return State->frame;//???
    }

    virtual uint_t Frame() const
    {
      return TimeType(State->time).Get() / FrameDuration.Get();
    }

    virtual uint_t Channels() const
    {
      return State->virt_used;//????
    }
  private:
    const std::vector<uint_t> PatternSizes;
    const TimeType FrameDuration;
    const StatePtr State;
  };

  class Analyzer : public Module::Analyzer
  {
  public:
    Analyzer(uint_t channels, StatePtr state)
      : Channels(channels)
      , State(state)
    {
    }

    virtual void GetState(std::vector<ChannelState>& channels) const
    {
      //difference between libxmp and regular spectrum formats is 2 octaves
      const int C2OFFSET = 24;
      std::vector<ChannelState> result;
      result.reserve(Channels);
      ChannelState chan;
      for (uint_t idx = 0; idx != Channels; ++idx)
      {
        const xmp_frame_info::xmp_channel_info& info = State->channel_info[idx];
        if (info.note != uint8_t(-1) && info.volume != 0)
        {
          //TODO: use period as precise playback speed
          chan.Band = std::max<int>(0, info.note - C2OFFSET);
          //TODO: also take into account sample's RMS
          chan.Level = info.volume;
          result.push_back(chan);
        }
      }
      channels.swap(result);
    }
  private:
    const uint_t Channels;
    const StatePtr State;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Context::Ptr ctx, Sound::Receiver::Ptr target, Parameters::Accessor::Ptr params, Information::Ptr info)
      : Ctx(ctx)
      , State(new xmp_frame_info())
      , Target(target)
      , Params(params)
      , SoundParams(Sound::RenderParameters::Create(params))
      , Track(MakePtr<TrackState>(info, State))
      , Analysis(MakePtr<Analyzer>(info->ChannelsCount(), State))
      , FrameDuration(info->GetFrameDuration())
      , SoundFreq()
      , Looped(false)
    {
      const Sound::RenderParameters::Ptr soundParams = Sound::RenderParameters::Create(params);
      SoundFreq = soundParams->SoundFreq();
      Looped = soundParams->Looped();
      Ctx->Call(&::xmp_start_player, static_cast<int>(SoundFreq), 0);
    }

    virtual ~Renderer()
    {
      Ctx->Call(&::xmp_end_player);
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return Track;
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return Analysis;
    }

    virtual bool RenderFrame()
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      static_assert(sizeof(Sound::Sample) == 4, "Incompatible sound sample size");

      try
      {
        ApplyParameters();
        Ctx->Call(&::xmp_play_frame);
        Ctx->Call(&::xmp_get_frame_info, State.get());
        Sound::ChunkBuilder builder;
        if (const std::size_t bytes = State->buffer_size)
        {
          const std::size_t samples = bytes / sizeof(Sound::Sample);
          builder.Reserve(samples);
          std::memcpy(builder.Allocate(samples), State->buffer, bytes);
          Target->ApplyData(builder.GetResult());
        }
        return Looped || State->loop_count == 0;
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    virtual void Reset()
    {
      Params.Reset();
      Ctx->Call(&::xmp_restart_module);
    }

    virtual void SetPosition(uint_t frame)
    {
      Ctx->Call(&::xmp_seek_time, static_cast<int>(FrameDuration.Get() * frame));
    }
  private:
    void ApplyParameters()
    {
      if (Params.IsChanged())
      {
        if (SoundFreq != SoundParams->SoundFreq())
        {
          SoundFreq = SoundParams->SoundFreq();
          Ctx->Call(&::xmp_end_player);
          Ctx->Call(&::xmp_start_player, static_cast<int>(SoundFreq), 0);
        }
        Parameters::IntType val = 0;
        Params->FindValue(Parameters::ZXTune::Sound::LOOPED, val);
        Looped = val != 0;
        val = Parameters::ZXTune::Core::DAC::INTERPOLATION_DEFAULT;
        Params->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, val);
        const int interpolation = val != Parameters::ZXTune::Core::DAC::INTERPOLATION_NO ? XMP_INTERP_SPLINE : XMP_INTERP_LINEAR;
        Ctx->Call(&::xmp_set_player, int(XMP_PLAYER_INTERP), interpolation);
      }
    }
  private:
    const Context::Ptr Ctx;
    const StatePtr State;
    const Sound::Receiver::Ptr Target;
    Parameters::TrackingHelper<Parameters::Accessor> Params;
    const Sound::RenderParameters::Ptr SoundParams;
    const TrackState::Ptr Track;
    const Analyzer::Ptr Analysis;
    const TimeType FrameDuration;
    uint_t SoundFreq;
    bool Looped;
  };

  class Holder : public Module::Holder
  {
  public:
    explicit Holder(Context::Ptr ctx, const xmp_module_info& modInfo, TimeType duration, Parameters::Accessor::Ptr props)
      : Ctx(ctx)
      , Info(MakePtr<Information>(*modInfo.mod, duration))
      , Properties(props)
    {
    }

    virtual Module::Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Properties;
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      return MakePtr<Renderer>(Ctx, target, params, Info);
    }
  private:
    const Context::Ptr Ctx;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };


  class Format : public Binary::Format
  {
  public:
    virtual bool Match(const Binary::Data& /*data*/) const
    {
      return true;
    }

    virtual std::size_t NextMatchOffset(const Binary::Data& data) const
    {
      return data.Size();
    }
  };

  struct PluginDescription
  {
    const char* const Id;
    const char* const Format;
    const struct format_loader* const Loader;
  };

  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    explicit Decoder(const PluginDescription& desc)
      : Desc(desc)
      , Fmt(Desc.Format ? Binary::CreateMatchOnlyFormat(Desc.Format) : MakePtr<Format>())
    {
    }

    virtual String GetDescription() const
    {
      return xmp_get_loader_name(Desc.Loader);
    }


    virtual Binary::Format::Ptr GetFormat() const
    {
      return Fmt;
    }

    virtual bool Check(const Binary::Container& rawData) const
    {
      return Fmt->Match(rawData);
    }

    virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const
    {
      return Formats::Chiptune::Container::Ptr();//TODO
    }
  private:
    const PluginDescription& Desc;
    const Binary::Format::Ptr Fmt;
  };
  
  void ParseStrings(const xmp_module& mod, PropertiesHelper& props)
  {
    Strings::Array strings;
    for (uint_t idx = 0; idx < mod.smp; ++idx)
    {
      strings.push_back(FromStdString(mod.xxs[idx].name));
    }
    for (uint_t idx = 0; idx < mod.ins; ++idx)
    {
      strings.push_back(FromStdString(mod.xxi[idx].name));
    }
    props.SetStrings(strings);
  }

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(const PluginDescription& desc)
      : Desc(desc)
    {
    }

    virtual Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData, Parameters::Container::Ptr properties) const
    {
      try
      {
        const Context::Ptr ctx = MakePtr<Context>(rawData, Desc.Loader);
        xmp_module_info modInfo;
        ctx->Call(&::xmp_get_module_info, &modInfo);
        xmp_frame_info frmInfo;
        ctx->Call(&::xmp_get_frame_info, &frmInfo);

        PropertiesHelper props(*properties);
        props.SetTitle(FromStdString(modInfo.mod->name));
        props.SetAuthor(FromStdString(modInfo.mod->author));
        props.SetProgram(FromStdString(modInfo.mod->type));
        if (const char* comment = modInfo.comment)
        {
          props.SetComment(FromStdString(comment));
        }
        ParseStrings(*modInfo.mod, props);
        const Binary::Container::Ptr data = rawData.GetSubcontainer(0, modInfo.size);
        const Formats::Chiptune::Container::Ptr source = Formats::Chiptune::CreateCalculatingCrcContainer(data, 0, modInfo.size);
        props.SetSource(*source);

        return MakePtr<Holder>(ctx, modInfo, TimeType(frmInfo.total_time), properties);
      }
      catch (const std::exception&)
      {
        return Holder::Ptr();
      }
    }
  private:
    const PluginDescription& Desc;
  };

  const PluginDescription PLUGINS[] =
  {
    //Composer 669
    {
      "669"
      ,
      "('i|'J)"
      "('f|'N)" //marker
      /*
      "?{108}"        //message
      "0-40"          //samples count
      "0-80"          //patterns count
      "0-7f"          //loop
      */
      ,
      &ssn_loader
    },
    //DSMI Advanced Module Format
    {
      "AMF"
      ,
      "'A'M'F"        //signature
      "0a-0e"         //version
      ,
      &amf_loader
    },
    //{"ARCH", &arch_loader},
    //Asylum Music Format
    {
      "AMF"
      ,
      "'A'S'Y'L'U'M' 'M'u's'i'c' 'F'o'r'm'a't' 'V'1'.'0"
      "00{8}"
      ,
      &asylum_loader
    },
    //{"COCO", &coco_loader},
    //DigiBooster Pro
    {
      "DBM"
      ,
      "'D'B'M'0"
      ,
      &dbm_loader
    },
    //DIGI Booster
    {
      "DBM"
      ,
      "'D'I'G'I' 'B'o'o's't'e'r' 'm'o'd'u'l'e"
      "00"
      ,
      &digi_loader
    },
    //X-Tracker
    {
      "DMF"
      ,
      "'D'D'M'F"
      ,
      &dmf_loader
    },
    //Digital Tracker
    {
      "DTM"
      ,
      "'D'.'T'."
      ,
      &dt_loader
    },
    //Desktop Tracker
    {
      "DTT"
      ,
      "'D's'k'T"
      ,
      &dtt_loader
    },
    //Quadra Composer
    {
      "EMOD"
      ,
      "'F'O'R'M"
      "????"
      "'E'M'O'D"
      ,
      &emod_loader
    },
    //Farandole Composer
    {
      "FAR"
      ,
      "'F'A'R"
      "fe"
      ,
      &far_loader
    },
    /*
    //Startrekker   may require additional files
    {
      "MOD"
      ,
      "?{1080}"
      "('F|'E)('L|'X)('T|'O)"
      "('4|'8|'M)"
      ,
      &flt_loader
    },
    */
    //Funktracker
    {
      "FNK"
      ,
      "'F'u'n'k"
      "?"
      "14-ff"     //(year-1980)*2
      "00-79"     //cpu and card (really separate)
      "?"
      ,
      &fnk_loader
    },
    //{"J2B", &gal4_loader},//requires depacking from MUSE packer
    //{"J2B", &gal5_loader},
    //Generic Digital Music
    {
      "GDM"
      ,
      "'G'D'M"
      "fe"
      "?{67}"
      "'G'M'F'S"
      ,
      &gdm_loader
    },
    //Graoumf Tracker
    {
      "GTK"
      ,
      "'G'T'K"
      "00-03"
      ,
      &gtk_loader
    },
    //His Master's Noise
    {
      "MOD"
      ,
      "?{1080}"
      "('F|'M)"
      "('E|'&)"
      "('S|'K)"
      "('T|'!)"
      ,
      &hmn_loader
    },
    //Soundtracker 2.6/Ice Tracker
    {
      "MTN"
      ,
      "?{1464}"
      "('M|'I)"
      "'T"
      "('N|'1)"
      "(00|'0)"
      ,
      &ice_loader
    },
    //Imago Orpheus
    {
      "IMF"
      ,
      "?{60}"
      "'I'M'1'0"
      ,
      &imf_loader
    },
    //Images Music System
    {
      "IMS"
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
      ,
      &ims_loader
    },
    //Impulse Tracker
    {
      "IT"
      ,
      "'I'M'P'M"
      ,
      &it_loader
    },
    //Liquid Tracker
    {
      "LIQ"
      ,
      "'L'i'q'u'i'd' 'M'o'd'u'l'e':"
      ,
      &liq_loader
    },
    //Epic MegaGames MASI
    {
      "PSM"
      ,
      "'P'S'M' "
      "???00"
      "'F'I'L'E"
      ,
      &masi_loader
    },
    //Digitrakker
    {
      "MDL"
      ,
      "'D'M'D'L"
      ,
      &mdl_loader
    },
    //MED 1.12 MED2
    {
      "MED"
      ,
      "'M'E'D"
      "02"
      ,
      &med2_loader
    },
    //MED 2.00 MED3
    {
      "MED"
      ,
      "'M'E'D"
      "03"
      ,
      &med3_loader
    },
    {
      "MED"
      ,
      "'M'E'D"
      "04"
      ,
      &med4_loader
    },
    //{"MFP", &mfp_loader},//requires additional files
    //{"MGT", &mgt_loader},experimental
    //MED 2.10/OctaMED
    {
      "MED"
      ,
      "'M'M'D('0|'1)"
      ,
      &mmd1_loader
    },
    //OctaMED
    {
      "MED"
      ,
      "'M'M'D('2|'3)"
      ,
      &mmd3_loader
    },
    //Protracker
    {
      "MOD"
      ,
      /*
      "?{20}"
      "("        //instruments
       "?{22}"   // name
       "00-7f?"  // BE size
       "0x"      // finetune
       "00-40"   // volume
       "00-7f?"  // BE loop start
       "00-7f?"  // BE loop size
      "){31}"   
      //+20+30*31=+950
      "?{130}"
      */
      "?{1080}"
      //+1080
      "('0-'3|'1-'9|'M      |'N|'C   |'T|'F      |'N)"
      "('0-'9|'C   |'.|'!|'&|'.|'D   |'D|'A      |'S)"
      "('C   |'H   |'K      |'T|'6|'8|'Z|'0      |'M)"
      "('H   |'N   |'.|'!   |'.|'1   |'4|'4|'6|'8|'S)"
      ,
      &mod_loader
    },
    //Multitracker
    {
      "MTM"
      ,
      "'M'T'M"
      "10"
      ,
      &mtm_loader
    },
    //Liquid Tracker NO
    {
      "LIQ"
      ,
      "'N'O"
      "0000"
      ,
      &no_loader
    },
    //Oktalyzer
    {
      "OKT"
      ,
      "'O'K'T'A'S'O'N'G"
      ,
      &okt_loader
    },
    //{"MOD", &polly_loader},//rle packed, too weak structure
    //Protracker Studio
    {
      "PSM"
      ,
      "'P'S'M"
      "fe"
      ,
      &psm_loader
    },
    //Protracker 3
    {
      "PT36"
      ,
      "'F'O'R'M"
      "????"
      "'M'O'D'L"
      "'V'E'R'S"
      ,
      &pt3_loader
    },
    //Poly Tracker
    {
      "PTM"
      ,
      "?{44}"
      "'P'T'M'F"
      ,
      &ptm_loader
    },
    //{"MOD", &pw_loader},//requires depacking
    //Real Tracker
    {
      "RTM"
      ,
      "'R'T'M'M"
      "20"
      ,
      &rtm_loader
    },
    //Scream Tracker 3
    {
      "S3M"
      ,
      "?{44}"
      "'S'C'R'M"
      ,
      &s3m_loader
    },
    //SoundFX
    {
      "SFX"
      ,
      "?{60}"
      "'S'O'N'G"
      "?{60}"
      "'S'O'N'G"
      ,
      &sfx_loader
    },
    //{"MTP", &mtp_loader},//experimental
    //Soundtracker
    {
      "MOD"
      ,
      "(00|08|20-7f){20}"  //name
      "("                  //instruments
       "(00|08|20-7f){22}" // name
       "00-7f?"            // BE size
       "0x"                // finetune
       "00-40"             // volume
       "??"                // BE loop start
       "00-7f?"            // BE loop size
      "){15}"
      "01-7f"           //len
      "?"               //restart
      "(00-7f){128}"    //order
      ,
      &st_loader
    },
    //Slamtilt
    {
      "STIM"
      ,
      "'S'T'I'M"         //signature
      "00???"            //BE samples offsets (assume 16Mb is enough)
      "?{8}"             //unknown
      "00?"              //BE number of samples (assume 255 is enough)
      "0001-80"          //BE count of positions (1-128)
      "0001-80"          //BE count of saved patterns (1-128)
      ,
      &stim_loader
    },
    //Scream Tracker 2
    {
      "STM"
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
      "?"
      "02"      //type=module
      "01-ff"   //no stx
      ,
      &stm_loader
    },
    //STMIK 0.2
    {
      "STX"
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
      ,
      &stx_loader
    },
    //{"SYM", &sym_loader},
    //TCB Tracker
    {
      "TCB"
      ,
      "'A'N' 'C'O'O'L('.|'!)"
      ,
      &tcb_loader
    },
    //Ultra Tracker
    {
      "ULT"
      ,
      "'M'A'S'_'U'T'r'a'c'k'_'V'0'0"
      "('0-'4)"
      ,
      &ult_loader
    },
    //{"UMX", &umx_loader},//container
    //Fast Tracker II
    {
      "XM"
      ,
      "'E'x't'e'n'd'e'd' 'M'o'd'u'l'e':' "
      ,
      &xm_loader
    },
  };
}
}

namespace ZXTune
{
  void RegisterXMPPlugins(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = Capabilities::Module::Type::TRACK | Capabilities::Module::Device::DAC;
    for (const Module::Xmp::PluginDescription* it = Module::Xmp::PLUGINS; it != boost::end(Module::Xmp::PLUGINS); ++it)
    {
      const Module::Xmp::PluginDescription& desc = *it;

      const Formats::Chiptune::Decoder::Ptr decoder = MakePtr<Module::Xmp::Decoder>(desc);
      const Module::Factory::Ptr factory = MakePtr<Module::Xmp::Factory>(desc);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(FromStdString(desc.Id), CAPS, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
