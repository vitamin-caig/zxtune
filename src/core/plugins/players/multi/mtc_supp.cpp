/**
* 
* @file
*
* @brief  MTC support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/registrator.h"
#include "core/plugins/players/analyzer.h"
#include "core/plugins/players/plugin.h"
#include "core/plugins/players/streaming.h"
//common includes
#include <contract.h>
#include <error.h>
//library includes
#include <core/module_attrs.h>
#include <core/module_open.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/multi/multitrackcontainer.h>
#include <parameters/merged_accessor.h>
#include <parameters/serialize.h>
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//std includes
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>
#include <boost/make_shared.hpp>

namespace
{
  const Debug::Stream Dbg("Core::MTCSupp");
}

namespace Module
{
namespace MTC
{
  typedef std::vector<Module::Holder::Ptr> HoldersArray;
  typedef std::vector<Module::Renderer::Ptr> RenderersArray;
  typedef std::vector<Module::Analyzer::Ptr> AnalyzersArray;

  class CompositeAnalyzer : public Module::Analyzer
  {
  public:
    explicit CompositeAnalyzer(const AnalyzersArray& delegates)
      : Delegates(delegates)
      , MaxBands(4 * delegates.size())//approx
    {
    }

    virtual void GetState(std::vector<ChannelState>& channels) const
    {
      std::vector<ChannelState> result;
      result.reserve(MaxBands);
      for (AnalyzersArray::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        std::vector<ChannelState> portion;
        (*it)->GetState(portion);
        std::copy(portion.begin(), portion.end(), std::back_inserter(result));
      }
      MaxBands = std::max(MaxBands, result.size());
      channels.swap(result);
    }
    
    static Ptr Create(const RenderersArray& renderers)
    {
      AnalyzersArray delegates(renderers.size());
      std::transform(renderers.begin(), renderers.end(), delegates.begin(), boost::mem_fn(&Module::Renderer::GetAnalyzer));
      return boost::make_shared<CompositeAnalyzer>(boost::cref(delegates));
    }
  private:
    const AnalyzersArray Delegates;
    mutable std::size_t MaxBands;
  };
  
  class CompositeReceiver : public Sound::Receiver
  {
  public:
    typedef boost::shared_ptr<CompositeReceiver> Ptr;
    
    CompositeReceiver(Sound::Receiver::Ptr delegate, uint_t bufferSize)
      : Delegate(delegate)
      , Buffer(bufferSize)
      , DoneStreams()
    {
    }
    
    virtual void ApplyData(const Sound::Chunk::Ptr& data)
    {
      if (DoneStreams++)
      {
        Buffer.Mix(*data);
      }
      else
      {
        Buffer.Fill(*data);
      }
    }
    
    virtual void Flush()
    {
    }
    
    void FinishFrame()
    {
      Delegate->ApplyData(Buffer.Convert(DoneStreams));
      Delegate->Flush();
      DoneStreams = 0;
    }
  private:
    class WideSample
    {
    public:
      WideSample()
        : Left()
        , Right()
      {
      }
      
      explicit WideSample(const Sound::Sample& rh)
        : Left(rh.Left())
        , Right(rh.Right())
      {
      }
      
      void Add(const Sound::Sample& rh)
      {
        Left += rh.Left();
        Right += rh.Right();
      }
      
      Sound::Sample Convert(int_t divisor) const
      {
        BOOST_STATIC_ASSERT(Sound::Sample::MID == 0);
        return Sound::Sample(Left / divisor, Right / divisor);
      }
    private:
      Sound::Sample::WideType Left;
      Sound::Sample::WideType Right;
    };
    
    class CumulativeChunk
    {
    public:
      explicit CumulativeChunk(uint_t size)
        : Buffer(size)
      {
      }
      
      void Fill(const Sound::Chunk& data)
      {
        std::size_t idx = 0;
        for (const std::size_t size = GetDataSize(data); idx != size; ++idx)
        {
          Buffer[idx] = WideSample(data[idx]);
        }
        for (const std::size_t limit = Buffer.size(); idx != limit; ++idx)
        {
          Buffer[idx] = WideSample();
        }
      }
      
      void Mix(const Sound::Chunk& data)
      {
        for (std::size_t idx = 0, size = GetDataSize(data); idx != size; ++idx)
        {
          Buffer[idx].Add(data[idx]);
        }
      }
      
      Sound::Chunk::Ptr Convert(uint_t sources) const
      {
        const Sound::Chunk::Ptr result = boost::make_shared<Sound::Chunk>(Buffer.size());
        std::transform(Buffer.begin(), Buffer.end(), result->begin(), std::bind2nd(std::mem_fun_ref(&WideSample::Convert), sources));
        return result;
      }
    private:
      std::size_t GetDataSize(const Sound::Chunk& data) const
      {
        const std::size_t bufSize = Buffer.size();
        const std::size_t dataSize = data.size();
        if (bufSize != dataSize)
        {
          Dbg("Sound data size mismatch buffer=%u data=%u", bufSize, dataSize);
        }
        return std::min(bufSize, dataSize);
      }
    private:
      std::vector<WideSample> Buffer;
    };
  private:
    const Sound::Receiver::Ptr Delegate;
    CumulativeChunk Buffer;
    uint_t DoneStreams;
  };
  
  class ForcedLoopParam : public Parameters::Accessor
  {
  public:
    virtual uint_t Version() const
    {
      return 1;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      if (name == Parameters::ZXTune::Sound::LOOPED)
      {
        val = 1;
        return true;
      }
      else
      {
        return false;
      }
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::StringType& /*val*/) const
    {
      return false;
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const
    {
      return false;
    }

    virtual void Process(class Parameters::Visitor& visitor) const
    {
      visitor.SetValue(Parameters::ZXTune::Sound::LOOPED, 1);
    }
  };

  class CompositeRenderer : public Module::Renderer
  {
  public:
    CompositeRenderer(StateIterator::Ptr iterator, const RenderersArray& delegates, CompositeReceiver::Ptr target)
      : Iterator(iterator)
      , State(iterator->GetStateObserver())
      , Delegates(delegates)
      , Target(target)
      , Analyzer(CompositeAnalyzer::Create(Delegates))
    {
    }

    virtual Module::TrackState::Ptr GetTrackState() const
    {
      return State;
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
    {
      return Analyzer;
    }

    virtual bool RenderFrame()
    {
      bool result = true;
      for (std::size_t idx = 0, lim = Delegates.size(); idx != lim; ++idx)
      {
        result &= Delegates[idx]->RenderFrame();
      }
      if (result)
      {
        Target->FinishFrame();
      }
      //TODO: support looping
      Iterator->NextFrame(false);
      return result && Iterator->IsValid();
    }

    virtual void Reset()
    {
      std::for_each(Delegates.begin(), Delegates.end(), boost::mem_fn(&Module::Renderer::Reset));
    }

    virtual void SetPosition(uint_t frame)
    {
      std::for_each(Delegates.begin(), Delegates.end(), boost::bind(&Module::Renderer::SetPosition, _1, frame));
      Module::SeekIterator(*Iterator, frame);
    }
    
    static Ptr Create(Parameters::Accessor::Ptr params, Module::Information::Ptr info, const HoldersArray& holders, Sound::Receiver::Ptr target)
    {
      const std::size_t size = holders.size();
      const std::size_t bufSize = Sound::RenderParameters::Create(params)->SamplesPerFrame();
      const CompositeReceiver::Ptr receiver = boost::make_shared<CompositeReceiver>(target, bufSize);
      const Parameters::Accessor::Ptr forcedLoop = boost::make_shared<ForcedLoopParam>();
      RenderersArray delegates(size);
      for (std::size_t idx = 0; idx != size; ++idx)
      {
        const Module::Holder::Ptr holder = holders[idx];
        const Parameters::Accessor::Ptr delegateParams = Parameters::CreateMergedAccessor(forcedLoop, params, holder->GetModuleProperties());
        delegates[idx] = holder->CreateRenderer(delegateParams, receiver);
      }
      return boost::make_shared<CompositeRenderer>(Module::CreateStreamStateIterator(info), boost::cref(delegates), receiver);
    }
  private:
    const StateIterator::Ptr Iterator;
    const TrackState::Ptr State;
    const RenderersArray Delegates;
    const CompositeReceiver::Ptr Target;
    const Module::Analyzer::Ptr Analyzer;
  };
  
  template<class It>
  uint_t GetFramesCount(It begin, It end)
  {
    uint_t frames = 0;
    for (It it = begin; it != end; ++it)
    {
      frames = std::max(frames, (*it)->GetModuleInformation()->FramesCount());
    }
    return frames;
  }
  
  class CompositeHolder : public Module::Holder
  {
  public:
    CompositeHolder(Parameters::Accessor::Ptr props, const HoldersArray& delegates)
      : Properties(props)
      , Delegates(delegates)
      , Info(Module::CreateStreamInfo(GetFramesCount(Delegates.begin(), Delegates.end())))
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

    virtual Module::Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      return CompositeRenderer::Create(params, Info, Delegates, target);
    }
    
    static Ptr Create(Parameters::Accessor::Ptr props, const HoldersArray& delegates)
    {
      return boost::make_shared<CompositeHolder>(props, boost::cref(delegates));
    }
  private:
    const Parameters::Accessor::Ptr Properties;
    const HoldersArray Delegates;
    const Information::Ptr Info;
  };

  Parameters::Accessor::Ptr CombineProps(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second)
  {
    return first
           ? (second
             ? Parameters::CreateMergedAccessor(first, second)
             : first)
           : second;
  }

  Parameters::Accessor::Ptr CombineProps(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second, Parameters::Accessor::Ptr third)
  {
    return first
         ? (second
           ? (third
             ? Parameters::CreateMergedAccessor(first, second, third)
             : CombineProps(first, second))
           : CombineProps(first, third))
         : CombineProps(second, third);
  }
  
  
  class DataBuilder : public Formats::Chiptune::MultiTrackContainer::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& propBuilder)
      : Module(propBuilder)
      , CurTrack()
      , CurStream()
      , CurEntity(&Module)
    {
    }

    virtual void SetAuthor(const String& author)
    {
      GetCurrentProperties().SetAuthor(author);
    }
    
    virtual void SetTitle(const String& title)
    {
      GetCurrentProperties().SetTitle(title);
    }
    
    virtual void SetAnnotation(const String& annotation)
    {
      GetCurrentProperties().SetComment(annotation);
    }
    
    virtual void SetProperty(const String& property)
    {
      Strings::Map pairs;
      //TODO: extract
      const String::size_type delim = property.find_first_of('=');
      if (delim != String::npos)
      {
        pairs[property.substr(0, delim)] = property.substr(delim + 1);
      }
      else
      {
        pairs[property] = String();
      }
      Parameters::Convert(pairs, GetCurrentProperties());
    }
    
    virtual void StartTrack(uint_t idx)
    {
      Dbg("Start track %1%", idx);
      CurEntity = CurTrack = Module.AddTrack(idx);
    }
    
    virtual void SetData(Binary::Container::Ptr data)
    {
      Dbg("Set track data");
      CurEntity = CurStream = CurTrack->AddStream(data);
    }

    Module::Holder::Ptr GetResult() const
    {
      return Module.GetHolder();
    }
  private:
    class TrackEntity
    {
    public:
      typedef boost::shared_ptr<TrackEntity> Ptr;
      
      virtual ~TrackEntity() {}
      
      virtual Module::Holder::Ptr GetHolder() const = 0;

      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual Module::PropertiesBuilder& GetPropertiesBuilder() = 0;
    };
    
    class StaticPropertiesTrackEntity : public TrackEntity
    {
    public:
      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        return Props ? Props->GetResult() : Parameters::Accessor::Ptr();
      }
      
      virtual Module::PropertiesBuilder& GetPropertiesBuilder()
      {
        if (!Props)
        {
          Props = boost::make_shared<Module::PropertiesBuilder>();
        }
        return *Props;
      }
    private:
      boost::shared_ptr<Module::PropertiesBuilder> Props;
    };
    
    class Stream : public StaticPropertiesTrackEntity
    {
    public:
      explicit Stream(Module::Holder::Ptr holder)
        : Holder(holder)
      {
      }
      
      virtual Module::Holder::Ptr GetHolder() const
      {
        Require(IsValid());
        return Holder;
      }

      bool operator < (const Stream& rh) const
      {
        const bool isValid = IsValid();
        if (isValid != rh.IsValid())
        {
          return isValid;
        }
        else
        {
          return GetPenalty() < rh.GetPenalty();
        }
      }

      String GetType() const
      {
        if (Type.empty())
        {
          Require(Holder->GetModuleProperties()->FindValue(Module::ATTR_TYPE, Type));
        }
        return Type;
      }
    private:
      bool IsValid() const
      {
        return !!Holder;
      }

      uint_t GetPenalty() const
      {
        const String& type = GetType();
        if (type == "STR")
        {
          //badly emulated
          return 2;
        }
        else if (type == "AY")
        {
          //too low quality
          return 1;
        }
        else
        {
          //all is ok
          return 0;
        }
      }
    private:
      const Module::Holder::Ptr Holder;
      mutable String Type;
    };
    
    class Track : public StaticPropertiesTrackEntity
    {
    public:      
      Track()
        : SelectedStream()
      {
      }
      
      Stream* AddStream(Binary::Container::Ptr data)
      {
        Streams.push_back(Stream(OpenModule(data)));
        SelectedStream = 0;
        return &Streams.back();
      }
    
      virtual Module::Holder::Ptr GetHolder() const
      {
        //each track requires its own properties to create renderer (e.g. notetable)
        const Stream& stream = SelectStream();
        const Module::Holder::Ptr holder = stream.GetHolder();
        const Parameters::Accessor::Ptr props = CombineProps(holder->GetModuleProperties(), stream.GetProperties(), StaticPropertiesTrackEntity::GetProperties());
        return Module::CreateMixedPropertiesHolder(holder, props);
      }
      
      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        const Stream& stream = SelectStream();
        return CombineProps(stream.GetProperties(), StaticPropertiesTrackEntity::GetProperties());
      }
    private:  
      static Module::Holder::Ptr OpenModule(Binary::Container::Ptr data)
      {
        try
        {
          return Module::Open(*data);
        }
        catch (const Error&/*ignored*/)
        {
          return Module::Holder::Ptr();
        }
      }
      
      const Stream& SelectStream() const
      {
        if (!SelectedStream)
        {
          Dbg("Select stream from %1% candiates", Streams.size());
          Require(!Streams.empty());
          SelectedStream = &*std::min_element(Streams.begin(), Streams.end());
          Dbg(" selected %1%", SelectedStream->GetType());
        }
        return *SelectedStream;
      }
    private:
      std::list<Stream> Streams;
      mutable const Stream* SelectedStream;
    };
    
    class Tune : public TrackEntity
    {
    public:
      explicit Tune(Module::PropertiesBuilder& props)
        : Props(props)
      {
      }
      
      Track* AddTrack(uint_t idx)
      {
        Require(idx == Tracks.size());
        Tracks.push_back(Track());
        return &Tracks.back();
      }
      
      virtual Module::Holder::Ptr GetHolder() const
      {
        const std::size_t tracksCount = Tracks.size();
        Dbg("Merge %1% tracks together", tracksCount);
        Require(tracksCount > 0);
        if (tracksCount == 1)
        {
          return Tracks.front().GetHolder();
        }
        else
        { 
          std::vector<Module::Holder::Ptr> holders(tracksCount);
          std::transform(Tracks.begin(), Tracks.end(), holders.begin(), std::mem_fun_ref(&TrackEntity::GetHolder));
          return CompositeHolder::Create(GetProperties(), holders);
        }
      }

      virtual Parameters::Accessor::Ptr GetProperties() const
      {
        return Props.GetResult();
      }
      
      virtual Module::PropertiesBuilder& GetPropertiesBuilder()
      {
        return Props;
      }
    private:
      Module::PropertiesBuilder& Props;
      std::list<Track> Tracks;
    };
  private:
    Module::PropertiesBuilder& GetCurrentProperties()
    {
      return CurEntity->GetPropertiesBuilder();
    }
  private:
    Tune Module;
    Track* CurTrack;
    Stream* CurStream;
    TrackEntity* CurEntity;
  };
  
  class Factory : public Module::Factory
  {
  public:
    virtual Module::Holder::Ptr CreateModule(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      try
      {
        DataBuilder dataBuilder(propBuilder);
        if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::MultiTrackContainer::Parse(rawData, dataBuilder))
        {
          propBuilder.SetSource(*container);
          return dataBuilder.GetResult();
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create MTC: %s", e.what());
      }
      return Module::Holder::Ptr();
    }
  };
}
}

namespace ZXTune
{
  void RegisterMTCSupport(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'M', 'T', 'C', 0};
    const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_MULTI | CAP_CONV_RAW;

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateMultiTrackContainerDecoder();
    const Module::MTC::Factory::Ptr factory = boost::make_shared<Module::MTC::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
