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
    explicit GenericFactory(Factory::Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data,
                             Parameters::Container::Ptr properties) const override
    {
      if (auto chiptune = Delegate->CreateChiptune(data, std::move(properties)))
      {
        return CreateHolder(std::move(chiptune));
      }
      else
      {
        return {};
      }
    }

  private:
    const Factory::Ptr Delegate;
  };

  Module::Factory::Ptr CreateModuleFactory(Factory::Ptr delegate)
  {
    return MakePtr<GenericFactory>(std::move(delegate));
  }
}  // namespace Module::TFM
