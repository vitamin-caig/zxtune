/*
Abstract:
  Merged accessor implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <parameters/merged_accessor.h>
#include <parameters/visitor.h>
//std includes
#include <set>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace Parameters;

  class MergedVisitor : public Visitor
  {
  public:
    explicit MergedVisitor(Visitor& delegate)
      : Delegate(delegate)
    {
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      if (DoneIntegers.insert(name).second)
      {
        return Delegate.SetValue(name, val);
      }
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      if (DoneStrings.insert(name).second)
      {
        return Delegate.SetValue(name, val);
      }
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      if (DoneDatas.insert(name).second)
      {
        return Delegate.SetValue(name, val);
      }
    }
  private:
    Visitor& Delegate;
    std::set<NameType> DoneIntegers;
    std::set<NameType> DoneStrings;
    std::set<NameType> DoneDatas;
  };

  class DoubleAccessor : public Accessor
  {
  public:
    DoubleAccessor(Accessor::Ptr first, Accessor::Ptr second)
      : First(first)
      , Second(second)
    {
    }

    virtual uint_t Version() const
    {
      return First->Version() + Second->Version();
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      MergedVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
    }
  private:
    const Accessor::Ptr First;
    const Accessor::Ptr Second;
  };

  class TripleAccessor : public Accessor
  {
  public:
    TripleAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third)
      : First(first)
      , Second(second)
      , Third(third)
    {
    }

    virtual uint_t Version() const
    {
      return First->Version() + Second->Version() + Third->Version();
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val) ||
             Third->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, StringType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val) ||
             Third->FindValue(name, val);
    }

    virtual bool FindValue(const NameType& name, DataType& val) const
    {
      return First->FindValue(name, val) || 
             Second->FindValue(name, val) ||
             Third->FindValue(name, val);
    }

    virtual void Process(Visitor& visitor) const
    {
      MergedVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
      Third->Process(mergedVisitor);
    }
  private:
    const Accessor::Ptr First;
    const Accessor::Ptr Second;
    const Accessor::Ptr Third;
  };
}

namespace Parameters
{
  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second)
  {
    return boost::make_shared<DoubleAccessor>(first, second);
  }

  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third)
  {
    return boost::make_shared<TripleAccessor>(first, second, third);
  }
}
