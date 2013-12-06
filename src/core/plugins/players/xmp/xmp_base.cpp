/**
* 
* @file
*
* @brief  XMP support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <contract.h>
//local includes
#include "core/plugins/registrator.h"
#include "core/plugins/players/plugin.h"
//library includes
#include <core/plugin_attrs.h>
#include <formats/chiptune/container.h>
#include <sound/chunk_builder.h>
#include <sound/render_params.h>
#include <time/stamp.h>
//3rdparty includes
#define BUILDING_STATIC
#include <3rdparty/xmp/include/xmp.h>
#include <3rdparty/xmp/src/xmp_private.h>
//boost includes
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
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
    explicit TrackState(Information::Ptr info, StatePtr state)
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
    virtual void GetState(std::vector<ChannelState>& channels) const
    {
      //TODO
      channels.clear();
    }

    static Ptr Get()
    {
      static Analyzer self;
      return MakeSingletonPointer(self);
    }
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Context::Ptr ctx, Sound::Receiver::Ptr target, Sound::RenderParameters::Ptr params, Information::Ptr info)
      : Ctx(ctx)
      , State(new xmp_frame_info())
      , Target(target)
      , Params(params)
      , Track(boost::make_shared<TrackState>(info, State))
      , FrameDuration(info->GetFrameDuration())
    {
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
      return Analyzer::Get();
    }

    virtual bool RenderFrame()
    {
      BOOST_STATIC_ASSERT(Sound::Sample::CHANNELS == 2);
      BOOST_STATIC_ASSERT(Sound::Sample::BITS == 16);
      BOOST_STATIC_ASSERT(Sound::Sample::MID == 0);
      BOOST_STATIC_ASSERT(sizeof(Sound::Sample) == 4);

      try
      {
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
        return Params->Looped() || State->loop_count == 0;
      }
      catch (const std::exception&)
      {
        return false;
      }
    }

    virtual void Reset()
    {
      Ctx->Call(&::xmp_restart_module);
    }

    virtual void SetPosition(uint_t frame)
    {
      Ctx->Call(&::xmp_seek_time, static_cast<int>(FrameDuration.Get() * frame));
    }
  private:
    const Context::Ptr Ctx;
    const StatePtr State;
    const Sound::Receiver::Ptr Target;
    const Sound::RenderParameters::Ptr Params;
    const TrackState::Ptr Track;
    const TimeType FrameDuration;
  };

  class Holder : public Module::Holder
  {
  public:
    explicit Holder(Context::Ptr ctx, const xmp_module_info& modInfo, TimeType duration, Parameters::Accessor::Ptr props)
      : Ctx(ctx)
      , Info(boost::make_shared<Information>(boost::cref(*modInfo.mod), duration))
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
      const Sound::RenderParameters::Ptr parameters = Sound::RenderParameters::Create(params);
      Ctx->Call(&::xmp_start_player, static_cast<int>(parameters->SoundFreq()), 0);
      return boost::make_shared<Renderer>(Ctx, target, parameters, Info);
    }
  private:
    const Context::Ptr Ctx;
    const Information::Ptr Info;
    const Parameters::Accessor::Ptr Properties;
  };


  class Format : public Binary::Format
  {
  public:
    virtual bool Match(const Binary::Data& data) const
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
    const struct format_loader* const Loader;
  };

  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    explicit Decoder(const PluginDescription& desc)
      : Desc(desc)
      , Fmt(boost::make_shared<Format>())
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
      return true;//TODO
    }

    virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
    {
      return Formats::Chiptune::Container::Ptr();//TODO
    }
  private:
    const PluginDescription& Desc;
    const Binary::Format::Ptr Fmt;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(const PluginDescription& desc)
      : Desc(desc)
    {
    }

    virtual Module::Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      try
      {
        const Context::Ptr ctx = boost::make_shared<Context>(rawData, Desc.Loader);
        xmp_module_info modInfo;
        ctx->Call(&::xmp_get_module_info, &modInfo);
        xmp_frame_info frmInfo;
        ctx->Call(&::xmp_get_frame_info, &frmInfo);

        propBuilder.SetTitle(FromStdString(modInfo.mod->name));
        propBuilder.SetProgram(FromStdString(modInfo.mod->type));
        if (const char* comment = modInfo.comment)
        {
          propBuilder.SetComment(FromStdString(comment));
        }
        const Binary::Container::Ptr data = rawData.GetSubcontainer(0, modInfo.size);
        const Formats::Chiptune::Container::Ptr source = Formats::Chiptune::CreateCalculatingCrcContainer(data, 0, modInfo.size);
        propBuilder.SetSource(*source);

        return boost::make_shared<Holder>(ctx, boost::cref(modInfo), TimeType(frmInfo.total_time), propBuilder.GetResult());
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
    {"669", &ssn_loader},
    {"AMF", &amf_loader},
    //{"ARCH", &arch_loader},
    {"AMF", &asylum_loader},
    //{"COCO", &coco_loader},
    {"DBM", &dbm_loader},
    {"DBM", &digi_loader},
    {"DMF", &dmf_loader},
    {"DTM", &dt_loader},
    {"DTT", &dtt_loader},
    {"EMOD", &emod_loader},
    {"FAR", &far_loader},
    {"MOD", &flt_loader},//may require additional files
    {"FNK", &fnk_loader},
    //{"J2B", &gal4_loader},//requires depacking from MUSE packer
    //{"J2B", &gal5_loader},
    {"GDM", &gdm_loader},
    {"GTK", &gtk_loader},
    {"MOD", &hmn_loader},
    {"MOD", &ice_loader},
    {"IMF", &imf_loader},
    {"IMS", &ims_loader},
    {"IT", &it_loader},
    {"LIQ", &liq_loader},
    {"PSM", &masi_loader},
    {"MDL", &mdl_loader},
    {"MED", &med2_loader},
    {"MED", &med3_loader},
    //{"MED", &med4_loader},fix later
    //{"MFP", &mfp_loader},//requires additional files
    //{"MGT", &mgt_loader},experimental
    {"MED", &mmd1_loader},
    {"MED", &mmd3_loader},
    {"MOD", &mod_loader},
    {"MTM", &mtm_loader},
    {"LIQ", &no_loader},
    {"OKT", &okt_loader},
    //{"MOD", &polly_loader},//rle packed, too weak structure
    {"PSM", &psm_loader},
    {"PT36", &pt3_loader},
    {"PTM", &ptm_loader},
    //{"MOD", &pw_loader},//requires depacking
    {"RTM", &rtm_loader},
    {"SFX", &sfx_loader},
    //{"MTP", &mtp_loader},//experimental
    {"MOD", &st_loader},
    {"STIM", &stim_loader},
    {"STM", &stm_loader},
    {"STX", &stx_loader},
    //{"SYM", &sym_loader},
    {"MOD", &tcb_loader},
    {"ULT", &ult_loader},
    //{"UMX", &umx_loader},//container
    {"XM", &xm_loader},
  };
}
}

namespace ZXTune
{
  void RegisterXMPPlugins(PlayerPluginsRegistrator& registrator)
  {
    const uint_t CAPS = CAP_DEV_DAC | CAP_STOR_MODULE | CAP_CONV_RAW;
    for (const Module::Xmp::PluginDescription* it = Module::Xmp::PLUGINS; it != boost::end(Module::Xmp::PLUGINS); ++it)
    {
      const Module::Xmp::PluginDescription& desc = *it;

      const Formats::Chiptune::Decoder::Ptr decoder = boost::make_shared<Module::Xmp::Decoder>(desc);
      const Module::Factory::Ptr factory = boost::make_shared<Module::Xmp::Factory>(desc);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(FromStdString(desc.Id), CAPS, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
