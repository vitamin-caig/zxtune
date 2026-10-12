/**
 *
 * @file
 *
 * @brief  DAC-based modules factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/dac/dac_factory.h"

#include "module/players/dac/dac_base.h"
#include "module/players/dac/dac_properties_helper.h"

#include "make_ptr.h"

#include <utility>

namespace Module::DAC
{
  class GenericFactory : public Module::Factory
  {
  public:
    explicit GenericFactory(Factory create)
      : Create(create)
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data,
                             Parameters::Container::Ptr properties) const override
    {
      PropertiesHelper props(*properties);
      if (auto chiptune = Create(data, props))
      {
        return CreateHolder(std::move(chiptune), std::move(properties));
      }
      return {};
    }

  private:
    const Factory Create;
  };

  Module::Factory::Ptr CreateModuleFactory(Factory create)
  {
    return MakePtr<GenericFactory>(create);
  }
}  // namespace Module::DAC
