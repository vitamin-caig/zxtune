/*
Abstract:
  Parameters tracking implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <pointers.h>
//library includes
#include <parameters/tracking.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace
{
  using namespace Parameters;

  class CompositeModifier : public Modifier
  {
  public:
    CompositeModifier(Modifier::Ptr first, Modifier::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      First->SetValue(name, val);
      Second->SetValue(name, val);
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      First->SetValue(name, val);
      Second->SetValue(name, val);
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      First->SetValue(name, val);
      Second->SetValue(name, val);
    }

    virtual void RemoveValue(const NameType& name)
    {
      First->RemoveValue(name);
      Second->RemoveValue(name);
    }
  private:
    const Modifier::Ptr First;
    const Modifier::Ptr Second;
  };
}

namespace Parameters
{
  Container::Ptr CreatePreChangePropertyTrackedContainer(Container::Ptr delegate, Modifier& callback)
  {
    //TODO: get rid of fake pointers
    const Modifier::Ptr asPtr = Modifier::Ptr(&callback, NullDeleter<Modifier>());
    const Modifier::Ptr modifier = boost::make_shared<CompositeModifier>(asPtr, delegate);
    return Container::CreateAdapter(delegate, modifier);
  }

  Container::Ptr CreatePostChangePropertyTrackedContainer(Container::Ptr delegate, Modifier& callback)
  {
    //TODO: get rid of fake pointers
    const Modifier::Ptr asPtr = Modifier::Ptr(&callback, NullDeleter<Modifier>());
    const Modifier::Ptr modifier = boost::make_shared<CompositeModifier>(delegate, asPtr);
    return Container::CreateAdapter(delegate, modifier);
  }
}
