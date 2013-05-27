/*
Abstract:
  TS modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ts_base.h"
#include "core/src/core.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/tracking.h"
#include "core/src/callback.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/module_attrs.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <sound/mixer_factory.h>
//std includes
#include <set>
#include <typeinfo>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Debug::Stream Dbg("Core::TSSupp");

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

      virtual void SetValue(const NameType& name, IntType val)
      {
        if (DoneIntegers.insert(name).second)
        {
          return Delegate.SetValue(name, val);
        }
      }

      virtual void SetValue(const NameType& name, const StringType& val)
      {
        const StringsValuesMap::iterator it = Strings.find(name);
        if (it == Strings.end())
        {
          Strings.insert(StringsValuesMap::value_type(name, val));
        }
        else
        {
          MergeStringProperty(name, it->second, val);
        }
      }

      virtual void SetValue(const NameType& name, const DataType& val)
      {
        if (DoneDatas.insert(name).second)
        {
          return Delegate.SetValue(name, val);
        }
      }

      void ProcessRestStrings() const
      {
        for (StringsValuesMap::const_iterator it = Strings.begin(), lim = Strings.end(); it != lim; ++it)
        {
          Delegate.SetValue(it->first, it->second);
        }
      }
    private:
      Visitor& Delegate;
      typedef std::map<NameType, StringType> StringsValuesMap;
      StringsValuesMap Strings;
      std::set<NameType> DoneIntegers;
      std::set<NameType> DoneDatas;
    };
  public:
    MergedModuleProperties(Accessor::Ptr first, Accessor::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return First->FindValue(name, val) || Second->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      String val1, val2;
      const bool res1 = First->FindValue(name, val1);
      const bool res2 = Second->FindValue(name, val2);
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

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return First->FindValue(name, val) || Second->FindValue(name, val);
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
    virtual uint_t ChannelsCount() const
    {
      return First->ChannelsCount() + Second->ChannelsCount();
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
    TSHolder(ModuleProperties::Ptr props, const AYM::Holder::Ptr& holder1, const AYM::Holder::Ptr& holder2)
      : Properties(props)
      , Holder1(holder1), Holder2(holder2)
      , Info(new MergedModuleInfo(Holder1->GetModuleInformation(), Holder2->GetModuleInformation()))
    {
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      const Parameters::Accessor::Ptr mixProps = boost::make_shared<MergedModuleProperties>(Holder1->GetModuleProperties(), Holder2->GetModuleProperties());
      return Parameters::CreateMergedAccessor(Properties, mixProps);
    }

    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const
    {
      const Sound::ThreeChannelsMixer::Ptr mixer = Sound::CreateThreeChannelsMixer(params);
      mixer->SetTarget(target);
      const Devices::AYM::Receiver::Ptr receiver = AYM::CreateReceiver(mixer);
      const boost::array<Devices::AYM::Receiver::Ptr, 2> tsMixer = CreateTSAYMixer(receiver);
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(params);
      const Devices::AYM::Chip::Ptr chip1 = Devices::AYM::CreateChip(chipParams, tsMixer[0]);
      const Devices::AYM::Chip::Ptr chip2 = Devices::AYM::CreateChip(chipParams, tsMixer[1]);

      const Information::Ptr info = GetModuleInformation();
      const Renderer::Ptr renderer1 = Holder1->CreateRenderer(params, chip1);
      const Renderer::Ptr renderer2 = Holder2->CreateRenderer(params, chip2);
      return CreateTSRenderer(renderer1, renderer2);
    }
  private:
    const ModuleProperties::Ptr Properties;
    const AYM::Holder::Ptr Holder1;
    const AYM::Holder::Ptr Holder2;
    const Information::Ptr Info;
  };
}

namespace
{
  using namespace ::ZXTune;

  //plugin attributes
  const Char* const ID = TS_PLUGIN_ID;
  const Char* const INFO = Text::TS_PLUGIN_INFO;
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
      : Description(CreatePluginDescription(ID, INFO, CAPS))
      , FooterFormat(Binary::Format::Create(TS_FOOTER_FORMAT))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      //TODO:
      return FooterFormat;
    }

    virtual Analysis::Result::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const Binary::Container::Ptr data = inputData->GetData();
      const uint8_t* const rawData = static_cast<const uint8_t*>(data->Start());
      const std::size_t size = data->Size();
      const std::size_t footerOffset = FooterFormat->NextMatchOffset(*data);
      //no footer in nearest data
      if (footerOffset == size)
      {
        return Analysis::CreateUnmatchedResult(size);
      }
      const Footer& footer = *safe_ptr_cast<const Footer*>(rawData + footerOffset);
      const std::size_t firstModuleSize = fromLE(footer.Size1);
      const std::size_t secondModuleSize = fromLE(footer.Size2);
      const std::size_t totalModulesSize = firstModuleSize + secondModuleSize;
      const std::size_t dataSize = footerOffset + sizeof(footer);
      if (!totalModulesSize || totalModulesSize != footerOffset)
      {
        const std::size_t lookahead = !totalModulesSize || totalModulesSize > footerOffset
          ? dataSize
          : footerOffset - totalModulesSize;
        return Analysis::CreateUnmatchedResult(lookahead);
      }

      const DataLocation::Ptr firstSubLocation = CreateNestedLocation(inputData, data->GetSubcontainer(0, firstModuleSize));
      const Module::AYM::Holder::Ptr holder1 = boost::dynamic_pointer_cast<const Module::AYM::Holder>(Module::Open(firstSubLocation));
      if (!holder1)
      {
        Dbg("Invalid first module holder");
        return Analysis::CreateUnmatchedResult(dataSize);
      }
      const DataLocation::Ptr secondSubLocation = CreateNestedLocation(inputData, data->GetSubcontainer(firstModuleSize, footerOffset - firstModuleSize));
      const Module::AYM::Holder::Ptr holder2 = boost::dynamic_pointer_cast<const Module::AYM::Holder>(Module::Open(secondSubLocation));
      if (!holder2)
      {
        Dbg("Failed to create second module holder");
        return Analysis::CreateUnmatchedResult(dataSize);
      }
      const ModuleProperties::RWPtr properties = ModuleProperties::Create(ID, inputData);
      properties->SetSource(dataSize, ModuleRegion(0, dataSize));
      const Module::Holder::Ptr holder(new TSHolder(properties, holder1, holder2));
      callback.ProcessModule(inputData, holder);
      return Analysis::CreateMatchedResult(dataSize);
    }
  private:
    const Plugin::Ptr Description;
    const Binary::Format::Ptr FooterFormat;
  };
}

namespace ZXTune
{
  void RegisterTSSupport(PlayerPluginsRegistrator& registrator)
  {
    const PlayerPlugin::Ptr plugin(new TSPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
