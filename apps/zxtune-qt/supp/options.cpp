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
#include <tools.h>
//std includes
#include <set>
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QSettings>
//text includes
#include <text/text.h>

namespace
{
  using namespace Parameters;

  class SettingsContainer : public Container
  {
  public:
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

    virtual void Process(Visitor& visitor) const
    {
      Require(!"Should not be called");
    }

    virtual void SetValue(const NameType& name, IntType val)
    {
      Value value(Storage, name);
      value.Set(QVariant(qlonglong(val)));
    }

    virtual void SetValue(const NameType& name, const StringType& val)
    {
      Value value(Storage, name);
      value.Set(QVariant(ToQString(ConvertToString(val))));
    }

    virtual void SetValue(const NameType& name, const DataType& val)
    {
      Value value(Storage, name);
      const QByteArray arr(val.empty() ? QByteArray() : QByteArray(safe_ptr_cast<const char*>(&val[0]), val.size()));
      value.Set(QVariant(arr));
    }

    virtual void RemoveValue(const NameType& name)
    {
      Value val(Storage, name);
      val.Remove();
    }
  private:
    typedef boost::shared_ptr<QSettings> SettingsPtr;
    typedef std::map<NameType, SettingsPtr> SettingsStorage;

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
        return FullName.npos != FullName.find(NAMESPACE_DELIMITER);
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
        QString res(ToQString(name));
        res.replace('.', '/');
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
        const NameType::size_type delimPos = FullName.find(NAMESPACE_DELIMITER);
        const NameType rootNamespace = FullName.substr(0, delimPos);
        ParamName = GetKeyName(FullName.substr(delimPos + 1));
        const SettingsStorage::const_iterator it = Storage.find(rootNamespace);
        if (it != Storage.end())
        {
          Setup = it->second;
        }
        else
        {
          Setup = boost::make_shared<QSettings>(QString(Text::PROJECT_NAME), ToQString(rootNamespace));
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

  class GlobalOptionsStorage : public GlobalOptions
  {
  public:
    virtual Container::Ptr Get() const
    {
      if (!Options)
      {
        const Container::Ptr persistent = boost::make_shared<SettingsContainer>();
        Options = boost::make_shared<CachedSettingsContainer>(persistent);
      }
      return Options;
    }
  private:
    mutable Container::Ptr Options;
  };
}

GlobalOptions& GlobalOptions::Instance()
{
  static GlobalOptionsStorage storage;
  return storage;
}
