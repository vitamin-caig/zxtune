/**
 *
 * @file
 *
 * @brief  TurboSound containers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/ts.h"

#include "formats/chiptune/aym/turbosound.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/aym_properties_helper.h"
#include "module/players/aym/turbosound.h"
#include "module/players/properties_helper.h"

#include "debug/log.h"

#include "make_ptr.h"

namespace Module::TS
{
  const Debug::Stream Dbg("Module::TS");

  class DataBuilder : public Formats::Chiptune::TurboSound::Builder
  {
  public:
    DataBuilder(const Parameters::Accessor& params, const Binary::Container& data, const Factory& delegate)
      : Params(params)
      , Data(data)
      , Delegate(delegate)
    {}

    void SetFirstSubmoduleLocation(std::size_t offset, std::size_t size) override
    {
      if (!LoadChiptune(offset, size, First, FirstProperties))
      {
        Dbg("Failed to create first module");
      }
    }

    void SetSecondSubmoduleLocation(std::size_t offset, std::size_t size) override
    {
      if (!LoadChiptune(offset, size, Second, SecondProperties))
      {
        Dbg("Failed to create second module");
      }
    }

    bool HasResult() const
    {
      return First && Second;
    }

    AYM::Chiptune::Ptr GetFirst() const
    {
      return First;
    }

    AYM::Chiptune::Ptr GetSecond() const
    {
      return Second;
    }

    Parameters::Accessor::Ptr GetFirstProperties() const
    {
      return FirstProperties;
    }

    Parameters::Accessor::Ptr GetSecondProperties() const
    {
      return SecondProperties;
    }

  private:
    bool LoadChiptune(std::size_t offset, std::size_t size, AYM::Chiptune::Ptr& tune,
                      Parameters::Accessor::Ptr& properties) const
    {
      const auto content = Data.GetSubcontainer(offset, size);
      if (const auto holder = std::dynamic_pointer_cast<const AYM::Holder>(TryOpenAYModule(*content)))
      {
        tune = holder->GetChiptune();
        properties = holder->GetModuleProperties();
        return true;
      }
      return false;
    }

    Module::Holder::Ptr TryOpenAYModule(const Binary::Container& data) const
    {
      const auto initialProperties = Parameters::Container::Create();
      return Delegate.CreateModule(Params, data, initialProperties);
    }

  private:
    const Parameters::Accessor& Params;
    const Binary::Container& Data;
    const Factory& Delegate;
    AYM::Chiptune::Ptr First;
    AYM::Chiptune::Ptr Second;
    Parameters::Accessor::Ptr FirstProperties;
    Parameters::Accessor::Ptr SecondProperties;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& data,
                                     Parameters::Container::Ptr properties) const override
    {
      DataBuilder dataBuilder(params, data, *Delegate);
      if (const auto container = Formats::Chiptune::TurboSound::Parse(data, dataBuilder))
      {
        if (dataBuilder.HasResult())
        {
          AYM::PropertiesHelper props(*properties);
          props.SetSource(*container);
          props.SetChipsCount(Devices::TurboSound::CHIPS);
          auto moduleProperties = TurboSound::CreateModuleProperties(properties, dataBuilder.GetFirstProperties(),
                                                                     dataBuilder.GetSecondProperties());
          auto chiptune = TurboSound::CreateChiptune(dataBuilder.GetFirst(), dataBuilder.GetSecond());
          return TurboSound::CreateHolder(std::move(chiptune), std::move(moduleProperties));
        }
      }
      return {};
    }

  private:
    const Ptr Delegate;
  };

  Factory::Ptr CreateFactory(Factory::Ptr delegate)
  {
    return MakePtr<Factory>(std::move(delegate));
  }
}  // namespace Module::TS
