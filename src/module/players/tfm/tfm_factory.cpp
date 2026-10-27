/**
 *
 * @file
 *
 * @brief  TFM-based modules factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/tfm/tfm_factory.h"

#include "module/players/properties_helper.h"
#include "module/players/tfm/tfm_base.h"
#include "module/players/tfm/tfm_parameters.h"

#include "make_ptr.h"

#include <utility>

namespace Module::TFM
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
        props.SetSupportedParameters(ChipParameters::ALL);
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
}  // namespace Module::TFM
