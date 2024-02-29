/**
 *
 * @file
 *
 * @brief  Parameters containers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/container_factories.h>
#include <parameters/container.h>
// std includes
#include <map>
#include <utility>

namespace Parameters
{
  class StorageContainer : public Container
  {
  public:
    StorageContainer() = default;

    StorageContainer(const StorageContainer& src)
      : VersionValue(src.VersionValue)
      , Integers(src.Integers)
      , Strings(src.Strings)
      , Datas(src.Datas)
    {}

    // accessor virtuals
    uint_t Version() const override
    {
      return VersionValue;
    }

    bool FindValue(Identifier name, IntType& val) const override
    {
      if (const auto* ptr = Integers.Find(name))
      {
        val = *ptr;
        return true;
      }
      return false;
    }

    bool FindValue(Identifier name, StringType& val) const override
    {
      if (const auto* ptr = Strings.Find(name))
      {
        val = *ptr;
        return true;
      }
      return false;
    }

    Binary::Data::Ptr FindData(Identifier name) const override
    {
      if (const auto* ptr = Datas.Find(name))
      {
        return *ptr;
      }
      return {};
    }

    void Process(Visitor& visitor) const override
    {
      Integers.Visit(visitor);
      Strings.Visit(visitor);
      Datas.Visit(visitor);
    }

    // visitor virtuals
    void SetValue(Identifier name, IntType val) override
    {
      if (Integers.Update(name, val) | Strings.Erase(name) | Datas.Erase(name))
      {
        ++VersionValue;
      }
    }

    void SetValue(Identifier name, StringView val) override
    {
      if (Integers.Erase(name) | Strings.Update(name, val) | Datas.Erase(name))
      {
        ++VersionValue;
      }
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      if (Integers.Erase(name) | Strings.Erase(name) | Datas.Update(name, val))
      {
        ++VersionValue;
      }
    }

    // modifier virtuals
    void RemoveValue(Identifier name) override
    {
      if (Integers.Erase(name) | Strings.Erase(name) | Datas.Erase(name))
      {
        ++VersionValue;
      }
    }

  private:
    template<class T>
    class TransientMap
    {
    public:
      TransientMap() = default;
      TransientMap(const TransientMap<T>& rh)
        : Storage(rh.Storage)
      {}

      const T* Find(StringView name) const
      {
        const auto it = Storage.find(name);
        return it != Storage.end() ? &(it->second) : nullptr;
      }

      void Visit(Visitor& visitor) const
      {
        for (const auto& entry : Storage)
        {
          if constexpr (std::is_same_v<T, Binary::Data::Ptr>)
          {
            visitor.SetValue(entry.first, *entry.second);
          }
          else
          {
            visitor.SetValue(entry.first, entry.second);
          }
        }
      }

      bool Erase(StringView name)
      {
        const auto it = Storage.find(name);
        return it != Storage.end() ? (Storage.erase(it), true) : false;
      }

      template<class Ref>
      bool Update(StringView name, Ref value)
      {
        const auto lower = Storage.lower_bound(name);
        if (lower != Storage.end() && lower->first == name)
        {
          return Update(lower->second, value);
        }
        const auto it = Storage.insert(lower, {name.to_string(), {}});
        return Update(it->second, value);
      }

    private:
      static bool Update(IntType& ref, IntType update)
      {
        return ref != update ? (ref = update, true) : false;
      }

      static bool Update(StringType& ref, StringView update)
      {
        return ref != update ? (ref = update.to_string(), true) : false;
      }

      static bool Update(Binary::Data::Ptr& ref, Binary::View update)
      {
        // TODO: remove support of empty containers
        if (update.Size())
        {
          ref = Binary::CreateContainer(update);
        }
        else
        {
          static const EmptyData INSTANCE;
          ref = MakeSingletonPointer(INSTANCE);
        }
        return true;
      }

    private:
      std::map<String, T, std::less<>> Storage;
    };

    class EmptyData : public Binary::Data
    {
    public:
      const void* Start() const
      {
        return this;
      }

      std::size_t Size() const
      {
        return 0;
      }
    };

  private:
    uint_t VersionValue = 0;
    TransientMap<IntType> Integers;
    TransientMap<StringType> Strings;
    TransientMap<Binary::Data::Ptr> Datas;
  };

  class CompositeContainer : public Container
  {
  public:
    CompositeContainer(Accessor::Ptr accessor, Modifier::Ptr modifier)
      : AccessDelegate(std::move(accessor))
      , ModifyDelegate(std::move(modifier))
    {}

    // accessor virtuals
    uint_t Version() const override
    {
      return AccessDelegate->Version();
    }

    bool FindValue(Identifier name, IntType& val) const override
    {
      return AccessDelegate->FindValue(name, val);
    }

    bool FindValue(Identifier name, StringType& val) const override
    {
      return AccessDelegate->FindValue(name, val);
    }

    Binary::Data::Ptr FindData(Identifier name) const override
    {
      return AccessDelegate->FindData(name);
    }

    void Process(Visitor& visitor) const override
    {
      return AccessDelegate->Process(visitor);
    }

    // visitor virtuals
    void SetValue(Identifier name, IntType val) override
    {
      return ModifyDelegate->SetValue(name, val);
    }

    void SetValue(Identifier name, StringView val) override
    {
      return ModifyDelegate->SetValue(name, val);
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      return ModifyDelegate->SetValue(name, val);
    }

    // modifier virtuals
    void RemoveValue(Identifier name) override
    {
      return ModifyDelegate->RemoveValue(name);
    }

  private:
    const Accessor::Ptr AccessDelegate;
    const Modifier::Ptr ModifyDelegate;
  };

  Container::Ptr Container::Create()
  {
    return MakePtr<StorageContainer>();
  }

  Container::Ptr Container::Clone(const Accessor& source)
  {
    if (const auto* storage = dynamic_cast<const StorageContainer*>(&source))
    {
      return MakePtr<StorageContainer>(*storage);
    }
    auto res = Container::Create();
    source.Process(*res);
    return res;
  }

  Container::Ptr Container::CreateAdapter(Accessor::Ptr accessor, Modifier::Ptr modifier)
  {
    return MakePtr<CompositeContainer>(std::move(accessor), std::move(modifier));
  }
}  // namespace Parameters
