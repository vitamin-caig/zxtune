/*
Abstract:
  TS modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ts_base.h"
#include <core/plugins/enumerator.h>
#include <core/plugins/players/tracking.h>
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
//std includes
#include <set>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
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
  const std::size_t MAX_MODULE_SIZE = 16384;
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

  using namespace Parameters;
  class MergedModuleProperties : public Accessor
  {
    static void MergeString(String& lh, const String& rh)
    {
      if (lh != rh)
      {
        lh += '/';
        lh += rh;
      }
    }

    class MergedStringsVisitor : public Visitor
    {
    public:
      explicit MergedStringsVisitor(Visitor& delegate)
        : Delegate(delegate)
      {
      }

      virtual void SetIntValue(const NameType& name, IntType val)
      {
        if (DoneIntegers.insert(name).second)
        {
          return Delegate.SetIntValue(name, val);
        }
      }

      virtual void SetStringValue(const NameType& name, const StringType& val)
      {
        const StringMap::iterator it = Strings.find(name);
        if (it == Strings.end())
        {
          Strings.insert(StringMap::value_type(name, val));
        }
        else
        {
          MergeString(it->second, val);
        }
      }

      virtual void SetDataValue(const NameType& name, const DataType& val)
      {
        if (DoneDatas.insert(name).second)
        {
          return Delegate.SetDataValue(name, val);
        }
      }

      void ProcessRestStrings() const
      {
        std::for_each(Strings.begin(), Strings.end(), boost::bind(&Visitor::SetStringValue, &Delegate,
          boost::bind(&StringMap::value_type::first, _1), boost::bind(&StringMap::value_type::second, _1)));
      }
    private:
      Visitor& Delegate;
      StringMap Strings;
      std::set<NameType> DoneIntegers;
      std::set<NameType> DoneDatas;
    };
  public:
    MergedModuleProperties(Accessor::Ptr first, Accessor::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual bool FindIntValue(const NameType& name, IntType& val) const
    {
      return First->FindIntValue(name, val) || Second->FindIntValue(name, val);
    }

    virtual bool FindStringValue(const NameType& name, StringType& val) const
    {
      String val1, val2;
      const bool res1 = First->FindStringValue(name, val1);
      const bool res2 = Second->FindStringValue(name, val2);
      if (res1 && res2)
      {
        MergeString(val1, val2);
        val = val1;
      }
      else if (res1 != res2)
      {
        val = res1 ? val1 : val2;
      }
      return res1 || res2;
    }

    virtual bool FindDataValue(const NameType& name, DataType& val) const
    {
      return First->FindDataValue(name, val) || Second->FindDataValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      MergedStringsVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
      mergedVisitor.ProcessRestStrings();
    }
  private:
    const Accessor::Ptr First;
    const Accessor::Ptr Second;
  };

  class TSModuleProperties : public Accessor
  {
  public:
    virtual bool FindIntValue(const NameType& /*name*/, IntType& /*val*/) const
    {
      return false;
    }

    virtual bool FindStringValue(const NameType& name, StringType& val) const
    {
      if (name == ATTR_TYPE)
      {
        val = TS_PLUGIN_ID;
        return true;
      }
      return false;
    }

    virtual bool FindDataValue(const NameType& /*name*/, DataType& /*val*/) const
    {
      return false;
    }

    virtual void Process(Visitor& visitor) const
    {
      visitor.SetStringValue(ATTR_TYPE, TS_PLUGIN_ID);
    }
  };

  class MergedModuleInfo : public Information
  {
  public:
    MergedModuleInfo(Information::Ptr lh, Information::Ptr rh)
      : First(lh)
      , Second(rh)
    {
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
    virtual Parameters::Accessor::Ptr Properties() const
    {
      if (!Props)
      {
        const Parameters::Accessor::Ptr tsProps = boost::make_shared<TSModuleProperties>();
        const Parameters::Accessor::Ptr mixProps = boost::make_shared<MergedModuleProperties>(First->Properties(), Second->Properties());
        Props = CreateMergedAccessor(tsProps, mixProps);
      }
      return Props;
    }
  private:
    const Information::Ptr First;
    const Information::Ptr Second;
    mutable Parameters::Accessor::Ptr Props;
  };

  //////////////////////////////////////////////////////////////////////////

  class TSHolder;
  Player::Ptr CreateTSPlayer(boost::shared_ptr<const TSHolder> mod);

  class TSHolder : public Holder, public boost::enable_shared_from_this<TSHolder>
  {
  public:
    TSHolder(Plugin::Ptr plugin, IO::DataContainer::Ptr data, const Holder::Ptr& holder1, const Holder::Ptr& holder2)
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
        const uint8_t* const data = static_cast<const uint8_t*>(RawData->Data());
        dst.assign(data, data + RawData->Size());
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
    IO::DataContainer::Ptr RawData;
    const Holder::Ptr Holder1;
    const Holder::Ptr Holder2;
    const Information::Ptr Info;
  };

  class TSPlayer : public Player
  {
  public:
    explicit TSPlayer(boost::shared_ptr<const TSHolder> holder)
      : Info(holder->GetModuleInformation())
      , Player1(holder->Holder1->CreatePlayer())
      , Player2(holder->Holder2->CreatePlayer())
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual TrackState::Ptr GetTrackState() const
    {
      return CreateTSTrackState(Player1->GetTrackState(), Player2->GetTrackState());
    }

    virtual Analyzer::Ptr GetAnalyzer() const
    {
      return CreateTSAnalyzer(Player1->GetAnalyzer(), Player2->GetAnalyzer());
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
  private:
    const Information::Ptr Info;
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
    const std::size_t limit = dump.Size();
    if (!in_range(limit, TS_MIN_SIZE, TS_MAX_SIZE))
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

    virtual Module::Holder::Ptr CreateModule(Parameters::Accessor::Ptr parameters,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const
    {
      const IO::FastDump dump(*container.Data);

      const std::size_t footerOffset = FindFooter(dump, SEARCH_THRESHOLD);
      assert(footerOffset);

      const Footer* const footer = safe_ptr_cast<const Footer*>(dump.Data() + footerOffset);
      const std::size_t firstModuleSize = fromLE(footer->Size1);

      const PluginsEnumerator& enumerator(PluginsEnumerator::Instance());

      try
      {
        MetaContainer subdata(container);
        Module::Holder::Ptr holder1;
        subdata.Data = container.Data->GetSubcontainer(0, firstModuleSize);
        enumerator.OpenModule(parameters, subdata, holder1);
        if (InvalidHolder(*holder1))
        {
          Log::Debug(THIS_MODULE, "Invalid first module holder");
          return Module::Holder::Ptr();
        }
        Module::Holder::Ptr holder2;
        subdata.Data = container.Data->GetSubcontainer(firstModuleSize, footerOffset - firstModuleSize);
        enumerator.OpenModule(parameters, subdata, holder2);
        if (InvalidHolder(*holder2))
        {
          Log::Debug(THIS_MODULE, "Failed to create second module holder");
          return Module::Holder::Ptr();
        }
        //try to create merged holder
        const Plugin::Ptr plugin = shared_from_this();
        if (firstModuleSize + fromLE(footer->Size2) != footerOffset)
        {
          Log::Debug(THIS_MODULE, "Invalid footer structure");
        }
        region.Offset = 0;
        region.Size = footerOffset + sizeof(*footer);
        const IO::DataContainer::Ptr rawData = region.Extract(*container.Data);
        const Module::Holder::Ptr holder(new TSHolder(plugin, rawData, holder1, holder2));
        //TODO: proper data attributes calculation calculation
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
