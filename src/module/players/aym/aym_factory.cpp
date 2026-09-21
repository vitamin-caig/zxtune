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
    explicit GenericFactory(ChiptuneCreator create)
      : Create(create)
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data,
                                     Parameters::Container::Ptr properties) const override
    {
      if (auto chiptune = Create(data, std::move(properties)))
      {
        return CreateHolder(std::move(chiptune));
      }
      else
      {
        return {};
      }
    }

  private:
    const ChiptuneCreator Create;
  };

  Module::Factory::Ptr CreateModuleFactory(ChiptuneCreator create)
  {
    return MakePtr<GenericFactory>(create);
  }
}  // namespace Module::AYM
