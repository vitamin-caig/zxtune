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
    virtual bool FindIntValue(const NameType& name, IntType& val) const
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

    virtual bool FindStringValue(const NameType& name, StringType& val) const
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

    virtual bool FindDataValue(const NameType& name, DataType& val) const
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

    virtual void SetIntValue(const NameType& name, IntType val)
    {
      Value value(Storage, name);
      value.Set(QVariant(qlonglong(val)));
    }

    virtual void SetStringValue(const NameType& name, const StringType& val)
    {
      Value value(Storage, name);
      value.Set(QVariant(ToQString(ConvertToString(val))));
    }

    virtual void SetDataValue(const NameType& name, const DataType& val)
    {
      Value value(Storage, name);
      const QByteArray arr(val.empty() ? QByteArray() : QByteArray(safe_ptr_cast<const char*>(&val[0]), val.size()));
      value.Set(QVariant(arr));
    }

    virtual void RemoveIntValue(const NameType& name)
    {
      Value val(Storage, name);
      val.Remove();
    }

    virtual void RemoveStringValue(const NameType& name)
    {
      Value val(Storage, name);
      val.Remove();
    }

    virtual void RemoveDataValue(const NameType& name)
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

    virtual bool FindIntValue(const NameType& name, IntType& val) const
    {
      if (Temporary->FindIntValue(name, val))
      {
        return true;
      }
      else if (RemovedInts.count(name))
      {
        return false;
      }
      else if (Persistent->FindIntValue(name, val))
      {
        Temporary->SetIntValue(name, val);
        return true;
      }
      return false;
    }

    virtual bool FindStringValue(const NameType& name, StringType& val) const
    {
      if (Temporary->FindStringValue(name, val))
      {
        return true;
      }
      else if (RemovedStrings.count(name))
      {
        return false;
      }
      else if (Persistent->FindStringValue(name, val))
      {
        Temporary->SetStringValue(name, val);
        return true;
      }
      return false;
    }

    virtual bool FindDataValue(const NameType& name, DataType& val) const
    {
      if (Temporary->FindDataValue(name, val))
      {
        return true;
      }
      else if (RemovedDatas.count(name))
      {
        return false;
      }
      else if (Persistent->FindDataValue(name, val))
      {
        Temporary->SetDataValue(name, val);
        return true;
      }
      return false;
    }

    virtual void Process(Visitor& visitor) const
    {
      Persistent->Process(visitor);
    }

    virtual void SetIntValue(const NameType& name, IntType val)
    {
      RemovedInts.erase(name);
      Temporary->SetIntValue(name, val);
      Persistent->SetIntValue(name, val);
    }

    virtual void SetStringValue(const NameType& name, const StringType& val)
    {
      RemovedStrings.erase(name);
      Temporary->SetStringValue(name, val);
      Persistent->SetStringValue(name, val);
    }

    virtual void SetDataValue(const NameType& name, const DataType& val)
    {
      RemovedDatas.erase(name);
      Temporary->SetDataValue(name, val);
      Persistent->SetDataValue(name, val);
    }

    virtual void RemoveIntValue(const NameType& name)
    {
      RemovedInts.insert(name);
      Temporary->RemoveIntValue(name);
      Persistent->RemoveIntValue(name);
    }

    virtual void RemoveStringValue(const NameType& name)
    {
      RemovedStrings.insert(name);
      Temporary->RemoveStringValue(name);
      Persistent->RemoveStringValue(name);
    }

    virtual void RemoveDataValue(const NameType& name)
    {
      RemovedDatas.insert(name);
      Temporary->RemoveDataValue(name);
      Persistent->RemoveDataValue(name);
    }
  private:
    const Container::Ptr Persistent;
    const Container::Ptr Temporary;
    std::set<NameType> RemovedInts, RemovedStrings, RemovedDatas;
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
