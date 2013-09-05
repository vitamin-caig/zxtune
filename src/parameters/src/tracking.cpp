/*
Abstract:
  Parameters tracking implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <parameters/tracking.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace Parameters;

  class PropertyTrackedContainer : public Container
  {
  public:
    PropertyTrackedContainer(Container::Ptr delegate, const PropertyChangedCallback& callback)
      : Delegate(delegate)
      , Callback(callback)
    {
    }

    virtual uint_t Version() const
    {
      return Delegate->Version();
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return Delegate->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return Delegate->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return Delegate->FindValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      Delegate->Process(visitor);
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      Delegate->SetValue(name, val);
      Callback.OnPropertyChanged(name);
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      Delegate->SetValue(name, val);
      Callback.OnPropertyChanged(name);
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      Delegate->SetValue(name, val);
      Callback.OnPropertyChanged(name);
    }

    virtual void RemoveValue(const NameType& name)
    {
      Delegate->RemoveValue(name);
      Callback.OnPropertyChanged(name);
    }
  private:
    const Container::Ptr Delegate;
    const PropertyChangedCallback& Callback;
  };
}

namespace Parameters
{
  Container::Ptr CreatePropertyTrackedContainer(Container::Ptr delegate, const PropertyChangedCallback& callback)
  {
    return boost::make_shared<PropertyTrackedContainer>(delegate, callback);
  }
}
