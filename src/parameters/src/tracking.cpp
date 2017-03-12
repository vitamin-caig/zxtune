/**
*
* @file
*
* @brief  Parameters tracking implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <parameters/tracking.h>
//std includes
#include <utility>

namespace Parameters
{
  class CompositeModifier : public Modifier
  {
  public:
    CompositeModifier(Modifier::Ptr first, Modifier::Ptr second)
      : First(std::move(first))
      , Second(std::move(second))
    {
    }

    void SetValue(const NameType& name, IntType val) override
    {
      First->SetValue(name, val);
      Second->SetValue(name, val);
    }

    void SetValue(const NameType& name, const StringType& val) override
    {
      First->SetValue(name, val);
      Second->SetValue(name, val);
    }

    void SetValue(const NameType& name, const DataType& val) override
    {
      First->SetValue(name, val);
      Second->SetValue(name, val);
    }

    void RemoveValue(const NameType& name) override
    {
      First->RemoveValue(name);
      Second->RemoveValue(name);
    }
  private:
    const Modifier::Ptr First;
    const Modifier::Ptr Second;
  };

  Container::Ptr CreatePreChangePropertyTrackedContainer(Container::Ptr delegate, Modifier& callback)
  {
    //TODO: get rid of fake pointers
    auto asPtr = Modifier::Ptr(&callback, NullDeleter<Modifier>());
    auto modifier = MakePtr<CompositeModifier>(std::move(asPtr), delegate);
    return Container::CreateAdapter(std::move(delegate), std::move(modifier));
  }

  Container::Ptr CreatePostChangePropertyTrackedContainer(Container::Ptr delegate, Modifier& callback)
  {
    //TODO: get rid of fake pointers
    auto asPtr = Modifier::Ptr(&callback, NullDeleter<Modifier>());
    auto modifier = MakePtr<CompositeModifier>(delegate, std::move(asPtr));
    return Container::CreateAdapter(std::move(delegate), std::move(modifier));
  }
}
