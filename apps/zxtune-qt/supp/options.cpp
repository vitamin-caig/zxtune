/**
 *
 * @file
 *
 * @brief Options access implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/supp/options.h"

#include "apps/zxtune-qt/ui/utils.h"

#include "parameters/src/names_set.h"

#include "binary/container_factories.h"
#include "binary/data.h"
#include "binary/view.h"
#include "parameters/convert.h"
#include "parameters/delegated.h"
#include "parameters/identifier.h"
#include "parameters/merged_accessor.h"
#include "parameters/tools.h"
#include "parameters/tracking.h"
#include "parameters/types.h"

#include "contract.h"
#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"
#include "types.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QFunctionPointer>
#include <QtCore/QLatin1Char>
#include <QtCore/QSettings>
#include <QtCore/QVariant>

#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>

namespace
{
  using namespace Parameters;

  const QLatin1Char PATH_SEPARATOR('/');

  class SettingsContainer : public Container
  {
  public:
    SettingsContainer() = default;

    uint_t Version() const override
    {
      return VersionValue;
    }

    std::optional<IntType> FindInteger(Identifier name) const override
    {
      const Value value(Storage, name);
      if (!value.IsValid())
      {
        return std::nullopt;
      }
      const QVariant& var = value.Get();
      const QVariant::Type type = var.type();
      if (type == QVariant::String)
      {
        return ConvertIntegerFromString(FromQString(var.toString()));
      }
      else if (type == QVariant::LongLong || type == QVariant::Int)
      {
        return var.toLongLong();
      }
      return std::nullopt;
    }

    std::optional<StringType> FindString(Identifier name) const override
    {
      const Value value(Storage, name);
      if (!value.IsValid())
      {
        return std::nullopt;
      }
      const QVariant& var = value.Get();
      if (var.type() == QVariant::String)
      {
        return ConvertStringFromString(FromQString(var.toString()));
      }
      return std::nullopt;
    }

    Binary::Data::Ptr FindData(Identifier name) const override
    {
      const Value value(Storage, name);
      if (!value.IsValid())
      {
        return {};
      }
      const QVariant& var = value.Get();
      if (var.type() == QVariant::ByteArray)
      {
        const QByteArray& arr = var.toByteArray();
        return Binary::CreateContainer(Binary::View(arr.data(), arr.size()));
      }
      return {};
    }

    void Process(Visitor& /*visitor*/) const override
    {
      // TODO: implement later
    }

    void SetValue(Identifier name, IntType val) override
    {
      Value value(Storage, name);
      value.Set(QVariant(qlonglong(val)));
      ++VersionValue;
    }

    void SetValue(Identifier name, StringView val) override
    {
      Value value(Storage, name);
      value.Set(QVariant(ToQString(ConvertToString(val))));
      ++VersionValue;
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      Value value(Storage, name);
      const auto size = val.Size();
      const QByteArray arr(size == 0 ? QByteArray() : QByteArray(val.As<char>(), size));
      value.Set(QVariant(arr));
      ++VersionValue;
    }

    void RemoveValue(Identifier name) override
    {
      Value val(Storage, name);
      val.Remove();
      ++VersionValue;
    }

  private:
    using SettingsPtr = std::shared_ptr<QSettings>;
    using SettingsStorage = std::map<QString, SettingsPtr>;

    class Value
    {
    public:
      Value(SettingsStorage& storage, Identifier name)
        : Storage(storage)
        , FullName(std::move(name))
      {}

      bool IsValid() const
      {
        return FullName.IsPath();
      }

      QVariant Get() const
      {
        const QString& name = GetName();
        return Setup->value(name);
      }

      void Set(const QVariant& val)
      {
        const QString& name = GetName();
        return Setup->setValue(name, val);
      }

      void Remove()
      {
        const QString& name = GetName();
        return Setup->remove(name);
      }

    private:
      static QString GetKeyName(StringView name)
      {
        auto res = ToQString(name);
        res.replace(QLatin1Char('.'), PATH_SEPARATOR);
        return res;
      }

      const QString& GetName() const
      {
        Require(IsValid());
        if (!Setup)
        {
          FillCache();
        }
        return ParamName;
      }

      void FillCache() const
      {
        Require(!Setup);
        const QString fullKey = GetKeyName(FullName);
        const QString rootNamespace = fullKey.section(PATH_SEPARATOR, 0, 0);
        ParamName = fullKey.section(PATH_SEPARATOR, 1);
        const auto it = Storage.find(rootNamespace);
        if (it != Storage.end())
        {
          Setup = it->second;
        }
        else
        {
          Setup = std::make_shared<QSettings>(QCoreApplication::applicationName(), rootNamespace);
          Storage.insert(SettingsStorage::value_type(rootNamespace, Setup));
        }
      }

    private:
      SettingsStorage& Storage;
      const Identifier FullName;
      mutable SettingsPtr Setup;
      mutable QString ParamName;
    };

  private:
    uint_t VersionValue = 0;
    mutable SettingsStorage Storage;
  };

  class CachedSettingsContainer : public Container
  {
  public:
    explicit CachedSettingsContainer(Container::Ptr delegate)
      : Persistent(std::move(delegate))
      , Temporary(Container::Create())
    {}

    uint_t Version() const override
    {
      return Temporary->Version();
    }

    std::optional<IntType> FindInteger(Identifier name) const override
    {
      if (auto tmp = Temporary->FindInteger(name))
      {
        return tmp;
      }
      else if (Removed.Has(name))
      {
        return std::nullopt;
      }
      else if (auto pers = Persistent->FindInteger(name))
      {
        Temporary->SetValue(name, *pers);
        return pers;
      }
      return std::nullopt;
    }

    std::optional<StringType> FindString(Identifier name) const override
    {
      if (auto tmp = Temporary->FindString(name))
      {
        return tmp;
      }
      else if (Removed.Has(name))
      {
        return std::nullopt;
      }
      else if (auto pers = Persistent->FindString(name))
      {
        Temporary->SetValue(name, *pers);
        return pers;
      }
      return std::nullopt;
    }

    Binary::Data::Ptr FindData(Identifier name) const override
    {
      if (auto res = Temporary->FindData(name))
      {
        return res;
      }
      else if (Removed.Has(name))
      {
        return {};
      }
      auto res = Persistent->FindData(name);
      if (res)
      {
        Temporary->SetValue(name, *res);
      }
      return res;
    }

    void Process(Visitor& visitor) const override
    {
      Persistent->Process(visitor);
    }

    void SetValue(Identifier name, IntType val) override
    {
      Removed.Erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    void SetValue(Identifier name, StringView val) override
    {
      Removed.Erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      Removed.Erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    void RemoveValue(Identifier name) override
    {
      Removed.Insert(name);
      Temporary->RemoveValue(name);
      Persistent->RemoveValue(name);
    }

  private:
    const Container::Ptr Persistent;
    const Container::Ptr Temporary;
    Parameters::NamesSet Removed;
  };

  /*
    Simple cached policy may be inconsistent in case of delayed values reading, so implement full COW snapshot.
  */
  class CompositeModifier : public Modifier
  {
  public:
    using Subscription = std::shared_ptr<void>;

    Subscription Subscribe(Modifier::Ptr delegate)
    {
      const std::lock_guard<std::mutex> lock(Guard);
      Delegates.insert(delegate);
      return {this, [delegate = std::move(delegate)](auto&& owner) { owner->Unsubscribe(delegate); }};
    }

    void SetValue(Identifier name, IntType val) override
    {
      const std::lock_guard<std::mutex> lock(Guard);
      for (const auto& delegate : Delegates)
      {
        delegate->SetValue(name, val);
      }
    }

    void SetValue(Identifier name, StringView val) override
    {
      const std::lock_guard<std::mutex> lock(Guard);
      for (const auto& delegate : Delegates)
      {
        delegate->SetValue(name, val);
      }
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      const std::lock_guard<std::mutex> lock(Guard);
      for (const auto& delegate : Delegates)
      {
        delegate->SetValue(name, val);
      }
    }

    void RemoveValue(Identifier name) override
    {
      const std::lock_guard<std::mutex> lock(Guard);
      for (const auto& delegate : Delegates)
      {
        delegate->RemoveValue(name);
      }
    }

  private:
    void Unsubscribe(const Modifier::Ptr& delegate)
    {
      const std::lock_guard<std::mutex> lock(Guard);
      Delegates.erase(delegate);
    }

  private:
    mutable std::mutex Guard;
    using ModifiersSet = std::set<Modifier::Ptr>;
    mutable ModifiersSet Delegates;
  };

  class CopyOnWrite : public Modifier
  {
  public:
    CopyOnWrite(Accessor::Ptr stored, Container::Ptr changed)
      : Stored(std::move(stored))
      , Changed(std::move(changed))
    {}

    void SetValue(Identifier name, IntType /*val*/) override
    {
      CopyExistingValue<IntType>(*Stored, *Changed, name);
    }

    void SetValue(Identifier name, StringView /*val*/) override
    {
      CopyExistingValue<StringType>(*Stored, *Changed, name);
    }

    void SetValue(Identifier name, Binary::View /*val*/) override
    {
      CopyExistingData(*Stored, *Changed, name);
    }

    void RemoveValue(Identifier /*name*/) override {}

  private:
    const Accessor::Ptr Stored;
    const Container::Ptr Changed;
  };

  class SettingsSnapshot : public DelegatedAccessor
  {
  public:
    SettingsSnapshot(Accessor::Ptr delegate, CompositeModifier::Subscription subscription)
      : DelegatedAccessor(std::move(delegate))
      , Subscription(std::move(subscription))
    {}

    static Accessor::Ptr Create(Accessor::Ptr stored, CompositeModifier& modifiers)
    {
      auto changed = Container::Create();
      auto delegate = CreateMergedAccessor(changed, stored);
      auto callback = MakePtr<CopyOnWrite>(std::move(stored), std::move(changed));
      return MakePtr<SettingsSnapshot>(std::move(delegate), modifiers.Subscribe(std::move(callback)));
    }

  public:
    const Accessor::Ptr Delegate;
    const CompositeModifier::Subscription Subscription;
  };

  class GlobalOptionsStorage : public GlobalOptions
  {
  public:
    Container::Ptr Get() const override
    {
      if (!TrackedOptions)
      {
        const Container::Ptr persistent = MakePtr<SettingsContainer>();
        Options = MakePtr<CachedSettingsContainer>(persistent);
        TrackedOptions = CreatePreChangePropertyTrackedContainer(Options, Modifiers);
      }
      return TrackedOptions;
    }

    Accessor::Ptr GetSnapshot() const override
    {
      return SettingsSnapshot::Create(Options, Modifiers);
    }

  private:
    mutable Container::Ptr Options;
    mutable Container::Ptr TrackedOptions;
    mutable CompositeModifier Modifiers;
  };
}  // namespace

GlobalOptions& GlobalOptions::Instance()
{
  static GlobalOptionsStorage storage;
  return storage;
}
