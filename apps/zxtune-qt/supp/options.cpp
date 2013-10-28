/*
Abstract:
  Options accessing implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "options.h"
#include "ui/utils.h"
//common includes
#include <contract.h>
#include <pointers.h>
//library includes
#include <parameters/convert.h>
#include <parameters/merged_accessor.h>
#include <parameters/tools.h>
#include <parameters/tracking.h>
//std includes
#include <set>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>
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

    virtual uint_t Version() const
    {
      return VersionValue;
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
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

    virtual bool FindValue(const NameType& name, StringType& val) const
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

    virtual bool FindValue(const NameType& name, DataType& val) const
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

    virtual void Process(Visitor& /*visitor*/) const
    {
      //TODO: implement later
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      Value value(Storage, name);
      value.Set(QVariant(qlonglong(val)));
      ++VersionValue;
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      Value value(Storage, name);
      value.Set(QVariant(ToQString(ConvertToString(val))));
      ++VersionValue;
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      Value value(Storage, name);
      const QByteArray arr(val.empty() ? QByteArray() : QByteArray(safe_ptr_cast<const char*>(&val[0]), val.size()));
      value.Set(QVariant(arr));
      ++VersionValue;
    }

    virtual void RemoveValue(const NameType& name)
    {
      Value val(Storage, name);
      val.Remove();
      ++VersionValue;
    }
  private:
    typedef boost::shared_ptr<QSettings> SettingsPtr;
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
          Setup = boost::make_shared<QSettings>(QLatin1String(Text::PROJECT_NAME), rootNamespace);
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
      : Persistent(delegate)
      , Temporary(Container::Create())
    {
    }

    virtual uint_t Version() const
    {
      return Temporary->Version();
    }

    virtual bool FindValue(const NameType& name, IntType& val) const
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

    virtual bool FindValue(const NameType& name, StringType& val) const
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

    virtual bool FindValue(const NameType& name, DataType& val) const
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

    virtual void Process(Visitor& visitor) const
    {
      Persistent->Process(visitor);
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      Removed.erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      Removed.erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      Removed.erase(name);
      Temporary->SetValue(name, val);
      Persistent->SetValue(name, val);
    }

    virtual void RemoveValue(const NameType& name)
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
    typedef boost::shared_ptr<void> Subscription;

    Subscription Subscribe(Modifier::Ptr delegate)
    {
      const boost::mutex::scoped_lock lock(Guard);
      Delegates.insert(delegate);
      return Subscription(this, boost::bind(&CompositeModifier::Unsubscribe, _1, delegate));
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      const boost::mutex::scoped_lock lock(Guard);
      for (ModifiersSet::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        (*it)->SetValue(name, val);
      }
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      const boost::mutex::scoped_lock lock(Guard);
      for (ModifiersSet::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        (*it)->SetValue(name, val);
      }
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      const boost::mutex::scoped_lock lock(Guard);
      for (ModifiersSet::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        (*it)->SetValue(name, val);
      }
    }

    virtual void RemoveValue(const NameType& name)
    {
      const boost::mutex::scoped_lock lock(Guard);
      for (ModifiersSet::const_iterator it = Delegates.begin(), lim = Delegates.end(); it != lim; ++it)
      {
        (*it)->RemoveValue(name);
      }
    }
  private:
    void Unsubscribe(Modifier::Ptr delegate)
    {
      const boost::mutex::scoped_lock lock(Guard);
      Delegates.erase(delegate);
    }
  private:
    mutable boost::mutex Guard;
    typedef std::set<Modifier::Ptr> ModifiersSet;
    mutable ModifiersSet Delegates;
  };

  class CopyOnWrite : public Modifier
  {
  public:
    CopyOnWrite(Accessor::Ptr stored, Container::Ptr changed)
      : Stored(stored)
      , Changed(changed)
    {
    }

    virtual void SetValue(const NameType& name, IntType /*val*/)
    {
      CopyExistingValue<IntType>(*Stored, *Changed, name);
    }

    virtual void SetValue(const NameType& name, const StringType& /*val*/)
    {
      CopyExistingValue<StringType>(*Stored, *Changed, name);
    }

    virtual void SetValue(const NameType& name, const DataType& /*val*/)
    {
      CopyExistingValue<DataType>(*Stored, *Changed, name);
    }

    virtual void RemoveValue(const NameType& /*name*/)
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
      : Delegate(delegate)
      , Subscription(subscription)
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
      return Delegate->Process(visitor);
    }

    static Accessor::Ptr Create(Accessor::Ptr stored, CompositeModifier& modifiers)
    {
      const Container::Ptr changed = Container::Create();
      const Accessor::Ptr delegate = CreateMergedAccessor(changed, stored);
      const Modifier::Ptr callback = boost::make_shared<CopyOnWrite>(stored, changed);
      return boost::make_shared<SettingsSnapshot>(delegate, modifiers.Subscribe(callback));
    }
  public:
    const Accessor::Ptr Delegate;
    const CompositeModifier::Subscription Subscription;
  };

  class GlobalOptionsStorage : public GlobalOptions
  {
  public:
    virtual Container::Ptr Get() const
    {
      if (!TrackedOptions)
      {
        const Container::Ptr persistent = boost::make_shared<SettingsContainer>();
        Options = boost::make_shared<CachedSettingsContainer>(persistent);
        TrackedOptions = CreatePreChangePropertyTrackedContainer(Options, Modifiers);
      }
      return TrackedOptions;
    }

    virtual Accessor::Ptr GetSnapshot() const
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
