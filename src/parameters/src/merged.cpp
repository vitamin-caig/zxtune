/**
 *
 * @file
 *
 * @brief  Merged adapters implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <parameters/merged_accessor.h>
#include <parameters/merged_container.h>
#include <parameters/src/names_set.h>
#include <parameters/visitor.h>
// std includes
#include <set>
#include <utility>

namespace Parameters
{
  class MergedVisitor : public Visitor
  {
  public:
    explicit MergedVisitor(Visitor& delegate)
      : Delegate(delegate)
    {}

    void SetValue(Identifier name, IntType val) override
    {
      if (DoneIntegers.Insert(name))
      {
        return Delegate.SetValue(name, val);
      }
    }

    void SetValue(Identifier name, StringView val) override
    {
      if (DoneStrings.Insert(name))
      {
        return Delegate.SetValue(name, val);
      }
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      if (DoneDatas.Insert(name))
      {
        return Delegate.SetValue(name, val);
      }
    }

  private:
    Visitor& Delegate;
    NamesSet DoneIntegers;
    NamesSet DoneStrings;
    NamesSet DoneDatas;
  };

  template<class Base, class Type1 = Base, class Type2 = Type1>
  class DoubleAdapter : public Base
  {
  public:
    DoubleAdapter(typename Type1::Ptr first, typename Type2::Ptr second)
      : First(std::move(first))
      , Second(std::move(second))
    {}

    uint_t Version() const override
    {
      return First->Version() + Second->Version();
    }

    std::optional<IntType> FindInteger(Identifier name) const override
    {
      return Find(name, &Accessor::FindInteger);
    }

    std::optional<StringType> FindString(Identifier name) const override
    {
      return Find(name, &Accessor::FindString);
    }

    Binary::Data::Ptr FindData(Identifier name) const override
    {
      return Find(name, &Accessor::FindData);
    }

    void Process(Visitor& visitor) const override
    {
      MergedVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
    }

  protected:
    const typename Type1::Ptr First;
    const typename Type2::Ptr Second;

  private:
    template<class R>
    R Find(Identifier name, R (Accessor::*func)(Identifier) const) const
    {
      if (auto first = (First.get()->*func)(name))
      {
        return first;
      }
      return (Second.get()->*func)(name);
    }
  };

  class TripleAccessor : public Accessor
  {
  public:
    TripleAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third)
      : First(std::move(first))
      , Second(std::move(second))
      , Third(std::move(third))
    {}

    uint_t Version() const override
    {
      return First->Version() + Second->Version() + Third->Version();
    }

    std::optional<IntType> FindInteger(Identifier name) const override
    {
      return Find(name, &Accessor::FindInteger);
    }

    std::optional<StringType> FindString(Identifier name) const override
    {
      return Find(name, &Accessor::FindString);
    }

    Binary::Data::Ptr FindData(Identifier name) const override
    {
      return Find(name, &Accessor::FindData);
    }

    void Process(Visitor& visitor) const override
    {
      MergedVisitor mergedVisitor(visitor);
      First->Process(mergedVisitor);
      Second->Process(mergedVisitor);
      Third->Process(mergedVisitor);
    }

  private:
    template<class R>
    R Find(Identifier name, R (Accessor::*func)(Identifier) const) const
    {
      if (auto first = (First.get()->*func)(name))
      {
        return first;
      }
      if (auto second = (Second.get()->*func)(name))
      {
        return second;
      }
      return (Third.get()->*func)(name);
    }

  private:
    const Accessor::Ptr First;
    const Accessor::Ptr Second;
    const Accessor::Ptr Third;
  };

  class ShadowingMergedContainer : public DoubleAdapter<Container, Accessor, Container>
  {
  public:
    ShadowingMergedContainer(Accessor::Ptr first, Container::Ptr second)
      : DoubleAdapter<Container, Accessor, Container>(std::move(first), std::move(second))
    {}

    void SetValue(Identifier name, IntType val) override
    {
      Second->SetValue(name, val);
    }

    void SetValue(Identifier name, StringView val) override
    {
      Second->SetValue(name, val);
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      Second->SetValue(name, val);
    }

    void RemoveValue(Identifier name) override
    {
      Second->RemoveValue(name);
    }
  };

  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second)
  {
    return MakePtr<DoubleAdapter<Accessor>>(std::move(first), std::move(second));
  }

  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third)
  {
    return MakePtr<TripleAccessor>(std::move(first), std::move(second), std::move(third));
  }

  Container::Ptr CreateMergedContainer(Accessor::Ptr first, Container::Ptr second)
  {
    return MakePtr<ShadowingMergedContainer>(std::move(first), std::move(second));
  }
}  // namespace Parameters
