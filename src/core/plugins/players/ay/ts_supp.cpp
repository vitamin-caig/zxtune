/*
Abstract:
  TS modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/plugins/enumerator.h>
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <messages_collector.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <devices/aym.h>
#include <io/container.h>
#include <sound/render_params.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>

#define FILE_TAG 83089B6F

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const std::string THIS_MODULE("Core::TSSupp");

  //plugin attributes
  const Char TS_PLUGIN_ID[] = {'T', 'S', 0};
  const String TS_PLUGIN_VERSION(FromStdString("$Rev$"));

  const std::size_t TS_MIN_SIZE = 256;
  const std::size_t MAX_MODULE_SIZE = 1 << 14;
  const std::size_t TS_MAX_SIZE = MAX_MODULE_SIZE * 2;
  //TODO: parametrize
  const std::size_t SEARCH_THRESHOLD = TS_MIN_SIZE;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Footer
  {
    uint8_t ID1[4];//'PT3!' or other type
    uint16_t Size1;
    uint8_t ID2[4];//same
    uint16_t Size2;
    uint8_t ID3[4];//'02TS'
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  const uint8_t TS_ID[] = {'0', '2', 'T', 'S'};

  BOOST_STATIC_ASSERT(sizeof(Footer) == 16);

  template<class T>
  inline T avg(T val1, T val2)
  {
    return (val1 + val2) / 2;
  }

  template<uint_t Channels>
  class TSMixer : public Sound::MultichannelReceiver
  {
  public:
    TSMixer() : Buffer(), Cursor(Buffer.end()), SampleBuf(Channels), Receiver(0)
    {
    }

    virtual void ApplyData(const std::vector<Sound::Sample>& data)
    {
      assert(data.size() == Channels);

      if (Receiver) //mix and out
      {
        std::transform(data.begin(), data.end(), Cursor->begin(), SampleBuf.begin(), avg<Sound::Sample>);
        Receiver->ApplyData(SampleBuf);
      }
      else //store
      {
        std::memcpy(Cursor->begin(), &data[0], std::min<uint_t>(Channels, data.size()) * sizeof(Sound::Sample));
      }
      ++Cursor;
    }

    virtual void Flush()
    {
    }

    void Reset(const Sound::RenderParameters& params)
    {
      //assert(Cursor == Buffer.end());
      Buffer.resize(params.SamplesPerFrame());
      Cursor = Buffer.begin();
      Receiver = 0;
    }

    void Switch(Sound::MultichannelReceiver& receiver)
    {
      //assert(Cursor == Buffer.end());
      Receiver = &receiver;
      Cursor = Buffer.begin();
    }
  private:
    typedef boost::array<Sound::Sample, Channels> InternalSample;
    std::vector<InternalSample> Buffer;
    typename std::vector<InternalSample>::iterator Cursor;
    std::vector<Sound::Sample> SampleBuf;
    Sound::MultichannelReceiver* Receiver;
  };

  void MergeMap(const Parameters::Map& lh, const Parameters::Map& rh, Parameters::Map& result)
  {
    result = lh;
    for (Parameters::Map::const_iterator it = rh.begin(), lim = rh.end(); it != lim; ++it)
    {
      Parameters::Map::iterator fnd(result.find(it->first));
      if (fnd == result.end() || fnd->second.empty())//new or was empty
      {
        result.insert(*it);
      }
      else if (it->first == ATTR_TYPE)
      {
        fnd->second = TS_PLUGIN_ID;
      }
      else if (!(it->second == fnd->second))
      {
        const String& asStr1(Parameters::ConvertToString(fnd->second));
        const String& asStr2(Parameters::ConvertToString(it->second));
        if (asStr1.empty())
        {
          fnd->second = asStr1;
        }
        else if (asStr2.empty())
        {
          continue;//do nothing
        }
        else
        {
          fnd->second = asStr1 + Char('/') + asStr2;
        }
      }
    }
  }

  class MergedModuleInfo : public Information
  {
  public:
    MergedModuleInfo(Information::Ptr lh, Information::Ptr rh)
      : First(lh), Second(rh)
    {
      //assume first is leading
      MergeMap(First->Properties(), Second->Properties(), Props);
    }
    virtual uint_t PositionsCount() const
    {
      return First->PositionsCount();
    }
    virtual uint_t LoopPosition() const
    {
      return First->LoopPosition();
    }
    virtual uint_t PatternsCount() const
    {
      return First->PatternsCount() + Second->PatternsCount();
    }
    virtual uint_t FramesCount() const
    {
      return First->FramesCount();
    }
    virtual uint_t LoopFrame() const
    {
      return First->LoopFrame();
    }
    virtual uint_t LogicalChannels() const
    {
      return First->LogicalChannels() + Second->LogicalChannels();
    }
    virtual uint_t PhysicalChannels() const
    {
      return std::max(First->PhysicalChannels(), Second->PhysicalChannels());
    }
    virtual uint_t Tempo() const
    {
      return std::min(First->Tempo(), Second->Tempo());
    }
    virtual const Parameters::Map& Properties() const
    {
      return Props;
    }
  private:
    const Information::Ptr First;
    const Information::Ptr Second;
    Parameters::Map Props;
  };

  //////////////////////////////////////////////////////////////////////////

  class TSHolder;
  Player::Ptr CreateTSPlayer(boost::shared_ptr<const TSHolder> mod);

  class TSHolder : public Holder, public boost::enable_shared_from_this<TSHolder>
  {
  public:
    TSHolder(Plugin::Ptr plugin, const Dump& data, const Holder::Ptr& holder1, const Holder::Ptr& holder2)
      : SrcPlugin(plugin)
      , RawData(data)
      , Holder1(holder1), Holder2(holder2)
      , Info(new MergedModuleInfo(Holder1->GetModuleInformation(), Holder2->GetModuleInformation()))
    {
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return SrcPlugin;
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return CreateTSPlayer(shared_from_this());
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
        return Error();
      }
      else
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
    }
  private:
    friend class TSPlayer;
    const Plugin::Ptr SrcPlugin;
    Dump RawData;
    const Holder::Ptr Holder1;
    const Holder::Ptr Holder2;
    const Information::Ptr Info;
  };

  class TSPlayer : public Player
  {
  public:
    explicit TSPlayer(boost::shared_ptr<const TSHolder> holder)
      : Module(holder)
      , Player1(holder->Holder1->CreatePlayer())
      , Player2(holder->Holder2->CreatePlayer())
    {
    }

    virtual const Holder& GetModule() const
    {
      return *Module;
    }

    virtual Error GetPlaybackState(State& state,
                                   Analyze::ChannelsState& analyzeState) const
    {
      Analyze::ChannelsState firstAnalyze;
      if (const Error& err = Player1->GetPlaybackState(state, firstAnalyze))
      {
        return err;
      }
      State secondState;
      Analyze::ChannelsState secondAnalyze;
      if (const Error& err = Player2->GetPlaybackState(secondState, secondAnalyze))
      {
        return err;
      }
      assert(state.Frame == secondState.Frame);
      //merge
      analyzeState.resize(firstAnalyze.size() + secondAnalyze.size());
      std::copy(secondAnalyze.begin(), secondAnalyze.end(),
        std::copy(firstAnalyze.begin(), firstAnalyze.end(), analyzeState.begin()));
      state.Track.Channels += secondState.Track.Channels;
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      PlaybackState state1, state2;
      Mixer.Reset(params);
      if (const Error& e = Player1->RenderFrame(params, state1, Mixer))
      {
        return e;
      }
      Mixer.Switch(receiver);
      if (const Error& e = Player2->RenderFrame(params, state2, Mixer))
      {
        return e;
      }
      state = state1 == MODULE_STOPPED || state2 == MODULE_STOPPED ? MODULE_STOPPED : MODULE_PLAYING;
      return Error();
    }

    virtual Error Reset()
    {
      if (const Error& e = Player1->Reset())
      {
        return e;
      }
      return Player2->Reset();
    }

    virtual Error SetPosition(uint_t frame)
    {
      if (const Error& e = Player1->SetPosition(frame))
      {
        return e;
      }
      return Player2->SetPosition(frame);
    }

    virtual Error SetParameters(const Parameters::Map& params)
    {
      if (const Error& e = Player1->SetParameters(params))
      {
        return e;
      }
      return Player2->SetParameters(params);
    }

  private:
    const Holder::Ptr Module;
    Player::Ptr Player1, Player2;
    TSMixer<AYM::CHANNELS> Mixer;
  };

  Player::Ptr CreateTSPlayer(boost::shared_ptr<const TSHolder> holder)
  {
    return Player::Ptr(new TSPlayer(holder));
  }

  /////////////////////////////////////////////////////////////////////////////
  std::size_t FindFooter(const IO::FastDump& dump, std::size_t threshold)
  {
    const std::size_t limit = std::min(dump.Size(), TS_MAX_SIZE);
    if (limit < TS_MIN_SIZE)
    {
      return 0;
    }
    const std::size_t inStart = limit >= threshold
      ? std::max(TS_MIN_SIZE, limit - threshold) : TS_MIN_SIZE;
    for (const uint8_t* begin = dump.Data() + inStart, *end = dump.Data() + limit;;)
    {
      const uint8_t* const found = std::search(begin, end, TS_ID, ArrayEnd(TS_ID));
      if (found == end)
      {
        return 0;
      }
      const std::size_t footerOffset = found + sizeof(TS_ID) - sizeof(Footer) - dump.Data();
      const Footer* const footer = safe_ptr_cast<const Footer*>(&dump[footerOffset]);
      const std::size_t size1 = fromLE(footer->Size1);
      const std::size_t size2 = fromLE(footer->Size2);
      if (size1 + size2 <= footerOffset &&
          size1 >= TS_MIN_SIZE && size2 >= TS_MIN_SIZE)
      {
        return footerOffset;
      }
      begin = found + ArraySize(TS_ID);
    }
  }
  
  inline bool InvalidHolder(const Module::Holder& holder)
  {
    const uint_t caps = holder.GetPlugin()->Capabilities();
    return 0 != (caps & (CAP_STORAGE_MASK ^ CAP_STOR_MODULE)) ||
           0 != (caps & (CAP_DEVICE_MASK ^ CAP_DEV_AYM)); 
  }

  //////////////////////////////////////////////////////////////////////////
  class TSPlugin : public PlayerPlugin
                  , public boost::enable_shared_from_this<TSPlugin>
  {
  public:
    virtual String Id() const
    {
      return TS_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::TS_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return TS_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_TS | CAP_CONV_RAW;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const IO::FastDump dump(inputData);
      return FindFooter(dump, SEARCH_THRESHOLD) != 0;
    }

    virtual Module::Holder::Ptr CreateModule(const Parameters::Map& commonParams,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const
    {
      const IO::FastDump dump(*container.Data);

      const std::size_t footerOffset = FindFooter(dump, SEARCH_THRESHOLD);
      assert(!footerOffset);

      const Footer* const footer = safe_ptr_cast<const Footer*>(dump.Data() + footerOffset);
      const std::size_t firstModuleSize = fromLE(footer->Size1);

      const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());
      MetaContainer subdata(container);

      Module::Holder::Ptr holder1;
      subdata.Data = container.Data->GetSubcontainer(0, firstModuleSize);
      if (enumerator.OpenModule(commonParams, subdata, holder1) || InvalidHolder(*holder1))
      {
        Log::Debug(THIS_MODULE, "Invalid first module holder");
        return Module::Holder::Ptr();
      }
      Module::Holder::Ptr holder2;
      subdata.Data = container.Data->GetSubcontainer(firstModuleSize, footerOffset - firstModuleSize);
      if (enumerator.OpenModule(commonParams, subdata, holder2) || InvalidHolder(*holder2))
      {
        Log::Debug(THIS_MODULE, "Failed to create second module holder");
        return Module::Holder::Ptr();
      }
      //try to create holder
      try
      {
        const Plugin::Ptr plugin = shared_from_this();
        if (firstModuleSize + fromLE(footer->Size2) != footerOffset)
        {
          Log::Debug(THIS_MODULE, "Invalid footer structure");
        }
        const std::size_t newSize = footerOffset + sizeof(*footer);
        const Module::Holder::Ptr holder(new TSHolder(plugin, Dump(dump.Data(), dump.Data() + newSize), holder1, holder2));
        region.Offset = 0;
        region.Size = newSize;
        return holder;
      }
      catch (const Error&)
      {
        Log::Debug(THIS_MODULE, "Failed to create holder");
      }
      return Module::Holder::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterTSSupport(PluginsEnumerator& enumerator)
  {
    const PlayerPlugin::Ptr plugin(new TSPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
