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
#include "core/plugins/enumerator.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
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
#include <core/module_open.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <devices/turbosound.h>
#include <formats/chiptune/container.h>
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

  class MergedModuleProperties : public Parameters::Accessor
  {
    static bool IsSingleProperty(const Parameters::NameType& propName)
    {
      return propName == Parameters::ZXTune::Core::AYM::TABLE;
    }

    static void MergeStringProperty(const Parameters::NameType& propName, String& lh, const String& rh)
    {
      if (!IsSingleProperty(propName) && lh != rh)
      {
        lh += '/';
        lh += rh;
      }
    }

    class MergedStringsVisitor : public Parameters::Visitor
    {
    public:
      explicit MergedStringsVisitor(Visitor& delegate)
        : Delegate(delegate)
      {
      }

      virtual void SetValue(const Parameters::NameType& name, Parameters::IntType val)
      {
        if (DoneIntegers.insert(name).second)
        {
          return Delegate.SetValue(name, val);
        }
      }

      virtual void SetValue(const Parameters::NameType& name, const Parameters::StringType& val)
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

      virtual void SetValue(const Parameters::NameType& name, const Parameters::DataType& val)
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
      Parameters::Visitor& Delegate;
      typedef std::map<Parameters::NameType, Parameters::StringType> StringsValuesMap;
      StringsValuesMap Strings;
      std::set<Parameters::NameType> DoneIntegers;
      std::set<Parameters::NameType> DoneDatas;
    };
  public:
    MergedModuleProperties(Parameters::Accessor::Ptr first, Parameters::Accessor::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual uint_t Version() const
    {
      return 1;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      return First->FindValue(name, val) || Second->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
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

    virtual bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return First->FindValue(name, val) || Second->FindValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      MergedStringsVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
      mergedVisitor.ProcessRestStrings();
    }
  private:
    const Parameters::Accessor::Ptr First;
    const Parameters::Accessor::Ptr Second;
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
    TSHolder(Parameters::Accessor::Ptr props, const AYM::Holder::Ptr& holder1, const AYM::Holder::Ptr& holder2)
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
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(params);
      const Sound::ThreeChannelsMatrixMixer::Ptr mixer = Sound::ThreeChannelsMatrixMixer::Create();
      Sound::FillMixer(*params, *mixer);
      const std::pair<Devices::AYM::Chip::Ptr, Devices::AYM::Chip::Ptr> chips = Devices::TurboSound::CreateChipsPair(chipParams, mixer, target);

      const Information::Ptr info = GetModuleInformation();
      const Renderer::Ptr renderer1 = Holder1->CreateRenderer(params, chips.first);
      const Renderer::Ptr renderer2 = Holder2->CreateRenderer(params, chips.second);
      return CreateTSRenderer(renderer1, renderer2);
    }
  private:
    const Parameters::Accessor::Ptr Properties;
    const AYM::Holder::Ptr Holder1;
    const AYM::Holder::Ptr Holder2;
    const Information::Ptr Info;
  };
}

namespace TS
{
  using namespace ZXTune;

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

  class ModuleTraits
  {
  public:
    ModuleTraits(const Binary::Data& data, std::size_t footerOffset)
      : FooterOffset(footerOffset)
      , Foot(footerOffset != data.Size() ? safe_ptr_cast<const Footer*>(static_cast<const uint8_t*>(data.Start()) + footerOffset) : 0)
      , FirstSize(Foot ? fromLE(Foot->Size1) : 0)
      , SecondSize(Foot ? fromLE(Foot->Size2) : 0)
    {
    }

    bool Matched() const
    {
      return Foot != 0 && FooterOffset == FirstSize + SecondSize;
    }

    std::size_t NextOffset() const
    {
      if (Foot == 0)
      {
        return FooterOffset;
      }
      const std::size_t totalSize = FirstSize + SecondSize;
      if (totalSize < FooterOffset)
      {
        return FooterOffset - totalSize;
      }
      else
      {
        return FooterOffset + sizeof(*Foot);
      }
    }

    Binary::Container::Ptr GetFirstContent(const Binary::Container& data) const
    {
      return data.GetSubcontainer(0, FirstSize);
    }

    Binary::Container::Ptr GetSecondContent(const Binary::Container& data) const
    {
      return data.GetSubcontainer(FirstSize, SecondSize);
    }

    Binary::Container::Ptr GetTotalContent(const Binary::Container& data) const
    {
      return data.GetSubcontainer(0, FooterOffset + sizeof(*Foot));
    }
  private:
    const std::size_t FooterOffset;
    const Footer* const Foot;
    const std::size_t FirstSize;
    const std::size_t SecondSize;
  };

  class FooterFormat : public Binary::Format
  {
  public:
    typedef boost::shared_ptr<const FooterFormat> Ptr;

    FooterFormat()
      : Delegate(Binary::Format::Create(TS_FOOTER_FORMAT))
    {
    }

    virtual bool Match(const Binary::Data& data) const
    {
      const ModuleTraits traits = GetTraits(data);
      return traits.Matched();
    }

    virtual std::size_t NextMatchOffset(const Binary::Data& data) const
    {
      const ModuleTraits traits = GetTraits(data);
      return traits.NextOffset();
    }

    ModuleTraits GetTraits(const Binary::Data& data) const
    {
      return ModuleTraits(data, Delegate->NextMatchOffset(data));
    }
  private:
    const Binary::Format::Ptr Delegate;
  };

  class Factory : public ModulesFactory
  {
  public:
    Factory()
      : Format(boost::make_shared<FooterFormat>())
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return Format->Match(inputData);
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Module::Holder::Ptr CreateModule(Module::PropertiesBuilder& properties, const Binary::Container& data) const
    {
      const ModuleTraits traits = Format->GetTraits(data);
      if (!traits.Matched())
      {
        return Module::Holder::Ptr();
      }
      try
      {
        const Module::AYM::Holder::Ptr holder1 = OpenAYMModule(traits.GetFirstContent(data));
        if (!holder1)
        {
          Dbg("Invalid first module holder");
          return Module::Holder::Ptr();
        }
        const Module::AYM::Holder::Ptr holder2 = OpenAYMModule(traits.GetSecondContent(data));
        if (!holder2)
        {
          Dbg("Failed to create second module holder");
          return Module::Holder::Ptr();
        }
        const Binary::Container::Ptr total = traits.GetTotalContent(data);
        properties.SetSource(Formats::Chiptune::CalculatingCrcContainer(total, 0, total->Size()));
        return boost::make_shared<TSHolder>(properties.GetResult(), holder1, holder2);
      }
      catch (const Error& e)
      {
        return Module::Holder::Ptr();
      }
    }
  private:
    static Module::AYM::Holder::Ptr OpenAYMModule(Binary::Container::Ptr data)
    {
      return boost::dynamic_pointer_cast<const Module::AYM::Holder>(Module::Open(*data));
    }
  private:
    const FooterFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterTSSupport(PlayerPluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<TS::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(TS::ID, TS::INFO, TS::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
