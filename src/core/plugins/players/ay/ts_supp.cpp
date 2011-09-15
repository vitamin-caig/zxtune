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
#include "core/src/core.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/tracking.h"
#include "core/src/callback.h"
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
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <devices/aym.h>
#include <io/container.h>
//std includes
#include <set>
//boost includes
#include <boost/bind.hpp>
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

  const std::size_t TS_MIN_SIZE = 256;
  const std::size_t MAX_MODULE_SIZE = 16384;
  const std::size_t TS_MAX_SIZE = MAX_MODULE_SIZE * 2;

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

  using namespace Parameters;
  class MergedModuleProperties : public Accessor
  {
    static bool IsSingleProperty(const NameType& propName)
    {
      return propName == Parameters::ZXTune::Core::AYM::TABLE;
    }

    static void MergeStringProperty(const NameType& propName, String& lh, const String& rh)
    {
      if (!IsSingleProperty(propName) && lh != rh)
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
          MergeStringProperty(name, it->second, val);
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
        MergeStringProperty(name, val1, val2);
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
  private:
    const Information::Ptr First;
    const Information::Ptr Second;
  };

  //////////////////////////////////////////////////////////////////////////

  class TSHolder : public Holder
  {
  public:
    TSHolder(Plugin::Ptr plugin, Binary::Container::Ptr data, const Holder::Ptr& holder1, const Holder::Ptr& holder2)
      : Plug(plugin)
      , RawData(data)
      , Holder1(holder1), Holder2(holder2)
      , Info(new MergedModuleInfo(Holder1->GetModuleInformation(), Holder2->GetModuleInformation()))
    {
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Plug;
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }


    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      const Parameters::Accessor::Ptr tsProps = boost::make_shared<TSModuleProperties>();
      const Parameters::Accessor::Ptr mixProps = boost::make_shared<MergedModuleProperties>(Holder1->GetModuleProperties(), Holder2->GetModuleProperties());
      return Parameters::CreateMergedAccessor(tsProps, mixProps);
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::MultichannelReceiver::Ptr target) const
    {
      return CreateTSRenderer(params, Holder1, Holder2, target);
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
    const Plugin::Ptr Plug;
    const Binary::Container::Ptr RawData;
    const Holder::Ptr Holder1;
    const Holder::Ptr Holder2;
    const Information::Ptr Info;
  };

  inline bool InvalidHolder(Module::Holder::Ptr holder)
  {
    if (!holder)
    {
      return true;
    }
    const uint_t caps = holder->GetPlugin()->Capabilities();
    return 0 != (caps & (CAP_STORAGE_MASK ^ CAP_STOR_MODULE)) ||
           0 != (caps & (CAP_DEVICE_MASK ^ CAP_DEV_AYM));
  }
}

namespace
{
  using namespace ::ZXTune;

  //plugin attributes
  const Char* const ID = TS_PLUGIN_ID;
  const Char* const INFO = Text::TS_PLUGIN_INFO;
  const String VERSION(FromStdString("$Rev$"));
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_TS | CAP_CONV_RAW;


  const std::string TS_FOOTER_FORMAT(
    "%0xxxxxxx%0xxxxxxx%0xxxxxxx21"  // uint8_t ID1[4];//'PT3!' or other type
    "?%00xxxxxx"                     // uint16_t Size1;
    "%0xxxxxxx%0xxxxxxx%0xxxxxxx21"  // uint8_t ID2[4];//same
    "?%00xxxxxx"                     // uint16_t Size2;
    "'0'2'T'S"                       // uint8_t ID3[4];//'02TS'
  );

  /////////////////////////////////////////////////////////////////////////////
  class TSPlugin : public PlayerPlugin
  {
  public:
    typedef boost::shared_ptr<const TSPlugin> Ptr;

    TSPlugin()
      : Description(CreatePluginDescription(ID, INFO, VERSION, CAPS))
      , FooterFormat(Binary::Format::Create(TS_FOOTER_FORMAT))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const Binary::Container::Ptr data = inputData->GetData();
      const uint8_t* const rawData = static_cast<const uint8_t*>(data->Data());
      const std::size_t size = data->Size();
      const std::size_t footerOffset = FooterFormat->Search(rawData, size);
      //no footer in nearest data
      if (footerOffset == size)
      {
        return DetectionResult::CreateUnmatched(size);
      }
      const Footer& footer = *safe_ptr_cast<const Footer*>(rawData + footerOffset);
      const std::size_t firstModuleSize = fromLE(footer.Size1);
      const std::size_t secondModuleSize = fromLE(footer.Size2);
      const std::size_t totalModulesSize = firstModuleSize + secondModuleSize;
      const std::size_t dataSize = footerOffset + sizeof(footer);
      if (totalModulesSize != footerOffset)
      {
        const std::size_t lookahead = totalModulesSize > footerOffset
          ? dataSize
          : footerOffset - totalModulesSize;
        return DetectionResult::CreateUnmatched(lookahead);
      }

      const PluginsEnumerator::Ptr usedPlugins = PluginsEnumerator::Create();
      const DataLocation::Ptr firstSubLocation = CreateNestedLocation(inputData, data->GetSubcontainer(0, firstModuleSize));
      const Parameters::Accessor::Ptr parameters = callback.CreateModuleParameters(inputData);

      const Module::Holder::Ptr holder1 = Module::Open(firstSubLocation, usedPlugins, parameters);
      if (InvalidHolder(holder1))
      {
        Log::Debug(THIS_MODULE, "Invalid first module holder");
        return DetectionResult::CreateUnmatched(dataSize);
      }
      const DataLocation::Ptr secondSubLocation = CreateNestedLocation(inputData, data->GetSubcontainer(firstModuleSize, footerOffset - firstModuleSize));
      const Module::Holder::Ptr holder2 = Module::Open(secondSubLocation, usedPlugins, parameters);
      if (InvalidHolder(holder2))
      {
        Log::Debug(THIS_MODULE, "Failed to create second module holder");
        return DetectionResult::CreateUnmatched(dataSize);
      }
      //try to create merged holder
      const Binary::Container::Ptr tsData = data->GetSubcontainer(0, dataSize);

      const Module::Holder::Ptr holder(new TSHolder(Description, tsData, holder1, holder2));
      //TODO: proper data attributes calculation
      callback.ProcessModule(inputData, holder);
      return DetectionResult::CreateMatched(dataSize);
    }
  private:
    const Plugin::Ptr Description;
    const Binary::Format::Ptr FooterFormat;
  };
}

namespace ZXTune
{
  void RegisterTSSupport(PluginsRegistrator& registrator)
  {
    const PlayerPlugin::Ptr plugin(new TSPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
