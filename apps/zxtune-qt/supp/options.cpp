/**
* 
* @file
*
* @brief Options access implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "options.h"
#include "ui/utils.h"
//common includes
#include <contract.h>
#include <pointers.h>
#include <make_ptr.h>
//library includes
#include <parameters/convert.h>
#include <parameters/merged_accessor.h>
#include <parameters/tools.h>
#include <parameters/tracking.h>
//std includes
#include <mutex>
#include <set>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QSettings>
//text includes
#include <text/text.h>

namespace
{
  using namespace Parameters;

  const QLatin1Char PATH_SEPARATOR('/');

  class SettingsContainer : public Container
  {
  public:
    SettingsContainer()
      : VersionValue()
    {
    }

    uint_t Version() const override
    {
      return VersionValue;
    }

    bool FindValue(const NameType& name, IntType& val) const override
    {
      const Value value(Storage, name);
      if (!value.IsValid())
      {
        return false;
      }
      const QVariant& var = value.Get();
      const QVariant::Type type = var.type();
      if (type == QVariant::String)
      {
        return ConvertFromString(FromQString(var.toString()), val);
      }
      else if (type == QVariant::LongLong || type == QVariant::Int)
      {
        val = var.toLongLong();
        return true;
      }
      return false;
    }

    bool FindValue(const NameType& name, StringType& val) const override
    {
      const Value value(Storage, name);
      if (!value.IsValid())
      {
        return false;
      }
      const QVariant& var = value.Get();
      if (var.type() == QVariant::String)
      {
        return ConvertFromString(FromQString(var.toString()), val);
      }
      return false;
    }

    bool FindValue(const NameType& name, DataType& val) const override
    {
      const Value value(Storage, name);
      if (!value.IsValid())
      {
        return false;
      }
      const QVariant& var = value.Get();
      if (var.type() == QVariant::ByteArray)
      {
        const QByteArray& arr = var.toByteArray();
        val.assign(arr.data(), arr.data() + arr.size());
        return true;
      }
      return false;
    }

    void Process(Visitor& /*visitor*/) const override
    {
      //TODO: implement later
    }

    void SetValue(const NameType& name, IntType val) override
    {
      Value value(Storage, name);
      value.Set(QVariant(qlonglong(val)));
      ++VersionValue;
    }

    void SetValue(const NameType& name, const StringType& val) override
    {
      Value value(Storage, name);
      value.Set(QVariant(ToQString(ConvertToString(val))));
      ++VersionValue;
    }

    void SetValue(const NameType& name, const DataType& val) override
    {
      Value value(Storage, name);
      const QByteArray arr(val.empty() ? QByteArray() : QByteArray(safe_ptr_cast<const char*>(&val[0]), val.size()));
      value.Set(QVariant(arr));
      ++VersionValue;
    }

    void RemoveValue(const NameType& name) override
    {
      Value val(Storage, name);
      val.Remove();
      ++VersionValue;
    }
  private:
    typedef std::shared_ptr<QSettings> SettingsPtr;
    typedef std::map<QString, SettingsPtr> SettingsStorage;

    class Value
    {
    public:
      Value(SettingsStorage& storage, const NameType& name)
        : Storage(storage)
        , FullName(name)
      {
      }

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
      static QString GetKeyName(const NameType& name)
      {
        QString res = QString::fromStdString(name.FullPath());
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
        const SettingsStorage::const_iterator it = Storage.find(rootNamespace);
        if (it != Storage.end())
        {
          Setup = it->second;
        }
        else
        {
          Setup = std::make_shared<QSettings>(QLatin1String(Text::PROJECT_NAME), rootNamespace);
          Storage.insert(SettingsStorage::value_type(rootNamespace, Setup));
        }
      }
    private:
      SettingsStorage& Storage;
      const NameType FullName;
      mutable SettingsPtr Setup;
      mutable QString ParamName;
    };
  private:
    uint_t VersionValue;
    mutable SettingsStorage Storage;
  };

  class CachedSettingsContainer : public Container
  {
  public:
    explicit CachedSettingsContainer(Container::Ptr delegate)
      : Persistent(std::move(delegate))
      , Temporary(Container::Create())
    {
    }

    uint_t Version() const override
    {
      return Temporary->Version();
    }

    bool FindValue(const NameType& name, IntType& val) const override
    {
      if (Temporary->FindValue(name, val))
      {
        return true;
      }
      else if (Removed.count(name))
      {
        return false;
      }
      else if (Persistent->FindValue(name, val))
      {
        Temporary->SetValue(name, val);
        return true;
      }
      return false;
    }

    bool FindValue(const NameType& name, StringType& val) const override
    {
      if (Temporary->FindValue(name, val))
      {
        return true;
      }
      else if (Removed.count(name))
      {
        return false;
      }
      else if (Persistent->FindValue(name, val))
      {
        Temporary->SetValue(name, val);
        return true;
      }
      return false;
    }

    bool FindValue(const NameType& name, DataType& val) const override
    {
      if (Temporary->FindValue(name, val))
      {
        return true;
      }
      else if (Removed.count(name))
      {
        return false;
      }
      else if (Persistent->FindValue(name, val))
      {
        Temporary->SetValue(name, val);
        return true;
      }
      return false;
    }

    void Process(Visitor& visitor) const override
    {
      Persistent->Process(visitor);
    }

    void SetValue(const NameType& name, IntType val) override
    {
      Removed.erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    void SetValue(const NameType& name, const StringType& val) override
    {
      Removed.erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    void SetValue(const NameType& name, const DataType& val) override
    {
      Removed.erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    void RemoveValue(const NameType& name) override
    {
      Removed.insert(name);
      Temporary->RemoveValue(name);
      Persistent->RemoveValue(name);
    }
  private:
    const Container::Ptr Persistent;
    const Container::Ptr Temporary;
    std::set<NameType> Removed;
  };

  /*
    Simple cached policy may be inconsistent in case of delayed values reading, so implement full COW snapshot.
  */
  class CompositeModifier : public Modifier
  {
  public:
    typedef std::shared_ptr<void> Subscription;

    Subscription Subscribe(Modifier::Ptr delegate)
    {
      const std::lock_guard<std::mutex> lock(Guard);
      Delegates.insert(delegate);
      return Subscription(this, boost::bind(&CompositeModifier::Unsubscribe, _1, delegate));
    }

    void SetValue(const NameType& name, IntType val) override
    {
      const std::lock_guard<std::mutex> lock(Guard);
      for (const auto& delegate : Delegates)
      {
        delegate->SetValue(name, val);
      }
    }

    void SetValue(const NameType& name, const StringType& val) override
    {
      const std::lock_guard<std::mutex> lock(Guard);
      for (const auto& delegate : Delegates)
      {
        delegate->SetValue(name, val);
      }
    }

    void SetValue(const NameType& name, const DataType& val) override
    {
      const std::lock_guard<std::mutex> lock(Guard);
      for (const auto& delegate : Delegates)
      {
        delegate->SetValue(name, val);
      }
    }

    void RemoveValue(const NameType& name) override
    {
      const std::lock_guard<std::mutex> lock(Guard);
      for (const auto& delegate : Delegates)
      {
        delegate->RemoveValue(name);
      }
    }
  private:
    void Unsubscribe(Modifier::Ptr delegate)
    {
      const std::lock_guard<std::mutex> lock(Guard);
      Delegates.erase(delegate);
    }
  private:
    mutable std::mutex Guard;
    typedef std::set<Modifier::Ptr> ModifiersSet;
    mutable ModifiersSet Delegates;
  };

  class CopyOnWrite : public Modifier
  {
  public:
    CopyOnWrite(Accessor::Ptr stored, Container::Ptr changed)
      : Stored(std::move(stored))
      , Changed(std::move(changed))
    {
    }

    void SetValue(const NameType& name, IntType /*val*/) override
    {
      CopyExistingValue<IntType>(*Stored, *Changed, name);
    }

    void SetValue(const NameType& name, const StringType& /*val*/) override
    {
      CopyExistingValue<StringType>(*Stored, *Changed, name);
    }

    void SetValue(const NameType& name, const DataType& /*val*/) override
    {
      CopyExistingValue<DataType>(*Stored, *Changed, name);
    }

    void RemoveValue(const NameType& /*name*/) override
    {
    }
  private:
    const Accessor::Ptr Stored;
    const Container::Ptr Changed;
  };

  class SettingsSnapshot : public Accessor
  {
  public:
    SettingsSnapshot(Accessor::Ptr delegate, CompositeModifier::Subscription subscription)
      : Delegate(std::move(delegate))
      , Subscription(std::move(subscription))
    {
    }

    uint_t Version() const override
    {
      return Delegate->Version();
    }

    bool FindValue(const NameType& name, IntType& val) const override
    {
      return Delegate->FindValue(name, val);
    }

    bool FindValue(const NameType& name, StringType& val) const override
    {
      return Delegate->FindValue(name, val);
    }

    bool FindValue(const NameType& name, DataType& val) const override
    {
      return Delegate->FindValue(name, val);
    }

    void Process(Visitor& visitor) const override
    {
      return Delegate->Process(visitor);
    }

    static Accessor::Ptr Create(Accessor::Ptr stored, CompositeModifier& modifiers)
    {
      const Container::Ptr changed = Container::Create();
      const Accessor::Ptr delegate = CreateMergedAccessor(changed, stored);
      const Modifier::Ptr callback = MakePtr<CopyOnWrite>(stored, changed);
      return MakePtr<SettingsSnapshot>(delegate, modifiers.Subscribe(callback));
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
}

GlobalOptions& GlobalOptions::Instance()
{
  static GlobalOptionsStorage storage;
  return storage;
}
