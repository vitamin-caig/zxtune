/*
Abstract:
  TS modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"

#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <messages_collector.h>
#include <tools.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <sound/render_params.h>
#include <core/devices/aym.h>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <text/core.h>
#include <text/plugins.h>
#include <text/warnings.h>

#define FILE_TAG 83089B6F

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const std::string THIS_MODULE("Core::TSSupp");

  //plugin attributes
  const Char TS_PLUGIN_ID[] = {'T', 'S', 0};
  const String TEXT_TS_VERSION(FromStdString("$Rev$"));

  const std::size_t TS_MIN_SIZE = 256;
  const std::size_t TS_MAX_SIZE = 1 << 17;

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

    virtual void ApplySample(const std::vector<Sound::Sample>& data)
    {
      assert(data.size() == Channels);

      if (Receiver) //mix and out
      {
        std::transform(data.begin(), data.end(), Cursor->begin(), SampleBuf.begin(), avg<Sound::Sample>);
        Receiver->ApplySample(SampleBuf);
      }
      else //store
      {
        std::memcpy(Cursor->begin(), &data[0], std::min(Channels, data.size()) * sizeof(Sound::Sample));
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

  //////////////////////////////////////////////////////////////////////////
  void DescribeTSPlugin(PluginInformation& info)
  {
    info.Id = TS_PLUGIN_ID;
    info.Description = TEXT_TS_INFO;
    info.Version = TEXT_TS_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_TS | CAP_CONV_RAW;
  }

  class TSHolder;
  Player::Ptr CreateTSPlayer(boost::shared_ptr<const TSHolder> mod);

  class TSHolder : public Holder, public boost::enable_shared_from_this<TSHolder>
  {
  public:
    TSHolder(const Dump& data, const Holder::Ptr& holder1, const Holder::Ptr& holder2)
      : RawData(data), Holder1(holder1), Holder2(holder2)
    {
      Holder1->GetModuleInformation(Info);
      //assume first is leading
      Information info2;
      Holder2->GetModuleInformation(info2);
      Info.Statistic.Channels += info2.Statistic.Channels;
      //physical channels are the same
      Parameters::Map mergedProps;
      MergeMap(Info.Properties, info2.Properties, mergedProps);
      Info.Properties.swap(mergedProps);
    }

    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribeTSPlugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = Info;
    }

    virtual void ModifyCustomAttributes(const Parameters::Map& attrs, bool replaceExisting)
    {
      return Parameters::MergeMaps(Info.Properties, attrs, Info.Properties, replaceExisting);
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
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
    }
  private:
    friend class TSPlayer;
    Dump RawData;
    Information Info;
    Holder::Ptr Holder1, Holder2;
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

    virtual Error GetPlaybackState(uint_t& timeState,
                                   Tracking& trackState,
                                   Analyze::ChannelsState& analyzeState) const
    {
      Analyze::ChannelsState firstAnalyze;
      if (const Error& err = Player1->GetPlaybackState(timeState, trackState, firstAnalyze))
      {
        return err;
      }
      Analyze::ChannelsState secondAnalyze;
      uint_t dummyTime = 0;
      Tracking dummyTracking;
      if (const Error& err = Player2->GetPlaybackState(dummyTime, dummyTracking, secondAnalyze))
      {
        return err;
      }
      assert(timeState == dummyTime);
      //merge
      analyzeState.resize(firstAnalyze.size() + secondAnalyze.size());
      std::copy(secondAnalyze.begin(), secondAnalyze.end(),
        std::copy(firstAnalyze.begin(), firstAnalyze.end(), analyzeState.begin()));
      trackState.Channels += dummyTracking.Channels;
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
    const Holder::ConstPtr Module;
    Player::Ptr Player1, Player2;
    TSMixer<AYM::CHANNELS> Mixer;
  };

  Player::Ptr CreateTSPlayer(boost::shared_ptr<const TSHolder> holder)
  {
    return Player::Ptr(new TSPlayer(holder));
  }

  /////////////////////////////////////////////////////////////////////////////
  inline bool OnlyAYMPlayersFilter(const PluginInformation& info)
  {
    return 0 != (info.Capabilities & (CAP_STORAGE_MASK ^ CAP_STOR_MODULE)) ||
           0 != (info.Capabilities & (CAP_DEVICE_MASK ^ CAP_DEV_AYM));
  }

  inline Error CopyModuleHolder(const String&, Module::Holder::Ptr src, Module::Holder::Ptr& dst)
  {
    dst = src;
    return Error();
  }

  bool CreateTSModule(const Parameters::Map& commonParams, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    const std::size_t limit = std::min(container.Data->Size(), TS_MAX_SIZE);
    if (limit < TS_MIN_SIZE)
    {
      return false;
    }
    //TODO: search for signature
    const uint8_t* const data(static_cast<const uint8_t*>(container.Data->Data()));
    const Footer* footer(safe_ptr_cast<const Footer*>(data + limit - sizeof(Footer)));
    if (0 != std::memcmp(footer->ID3, TS_ID, sizeof(TS_ID)) &&
        fromLE(footer->Size1) + fromLE(footer->Size2) + sizeof(*footer) != limit)
    {
      return false;
    }

    const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());
    MetaContainer subdata(container);
    ModuleRegion subregion;
    DetectParameters detectParams;
    detectParams.Filter = &OnlyAYMPlayersFilter;

    Module::Holder::Ptr holder1;
    detectParams.Callback = boost::bind(CopyModuleHolder, _1, _2, boost::ref(holder1));
    subdata.Data = container.Data->GetSubcontainer(0, fromLE(footer->Size1));
    if (enumerator.DetectModules(commonParams, detectParams, subdata, subregion) || !holder1)
    {
      Log::Debug(THIS_MODULE, "Failed to create first module holder");
      return false;
    }
    Module::Holder::Ptr holder2;
    detectParams.Callback = boost::bind(CopyModuleHolder, _1, _2, boost::ref(holder2));
    subdata.Data = container.Data->GetSubcontainer(fromLE(footer->Size1), fromLE(footer->Size2));
    if (enumerator.DetectModules(commonParams, detectParams, subdata, subregion) || !holder2)
    {
      Log::Debug(THIS_MODULE, "Failed to create second module holder");
      return false;
    }
    //try to create holder
    try
    {
      holder.reset(new TSHolder(Dump(data, data + limit), holder1, holder2));
      region.Offset = 0;
      region.Size = limit;
      return true;
    }
    catch (const Error&)
    {
      Log::Debug(THIS_MODULE, "Failed to create holder");
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterTSSupport(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribeTSPlugin(info);
    enumerator.RegisterPlayerPlugin(info, CreateTSModule);
  }
}
