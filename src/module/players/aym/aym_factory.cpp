/**
 *
 * @file
 *
 * @brief  AYM-based modules factory implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/aym/aym_factory.h"

#include "module/players/aym/aym_base.h"

#include "make_ptr.h"

#include <utility>

namespace Module::AYM
{
  class GenericFactory : public Module::Factory
  {
  public:
    explicit GenericFactory(Factory::Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data,
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
}  // namespace Module::AYM
