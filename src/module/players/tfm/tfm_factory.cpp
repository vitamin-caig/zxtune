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

#include "module/players/tfm/tfm_base.h"

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
      if (auto chiptune = Create(data, std::move(properties)))
      {
        return CreateHolder(std::move(chiptune));
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
