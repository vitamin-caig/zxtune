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
// library includes
#include <parameters/container.h>
// std includes
#include <map>
#include <utility>

namespace Parameters
{
  class StorageContainer : public Container
  {
  public:
    StorageContainer() {}

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
      return Integers.Find(name, val);
    }

    bool FindValue(Identifier name, StringType& val) const override
    {
      return Strings.Find(name, val);
    }

    bool FindValue(Identifier name, DataType& val) const override
    {
      return Datas.Find(name, val);
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

      bool Find(StringView name, T& res) const
      {
        const auto it = Storage.find(name);
        return it != Storage.end() ? (res = it->second, true) : false;
      }

      void Visit(Visitor& visitor) const
      {
        for (const auto& entry : Storage)
        {
          visitor.SetValue(entry.first, entry.second);
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

      static bool Update(DataType& ref, Binary::View update)
      {
        const auto* raw = update.As<uint8_t>();
        ref.assign(raw, raw + update.Size());
        return true;
      }

    private:
      std::map<String, T, std::less<>> Storage;
    };

  private:
    uint_t VersionValue = 0;
    TransientMap<IntType> Integers;
    TransientMap<StringType> Strings;
    TransientMap<DataType> Datas;
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

    bool FindValue(Identifier name, DataType& val) const override
    {
      return AccessDelegate->FindValue(name, val);
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
    const auto res = Container::Create();
    source.Process(*res);
    return res;
  }

  Container::Ptr Container::CreateAdapter(Accessor::Ptr accessor, Modifier::Ptr modifier)
  {
    return MakePtr<CompositeContainer>(std::move(accessor), std::move(modifier));
  }
}  // namespace Parameters
