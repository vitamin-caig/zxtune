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
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QSettings>
//text includes
#include <text/text.h>

namespace
{
  class SettingsContainer : public Parameters::Container
  {
  public:
    virtual bool FindIntValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      const Value value(Storage, name);
      if (!value.IsValid())
      {
        return false;
      }
      const QVariant& var = value.Get();
      if (var.type() == QVariant::LongLong)
      {
        val = var.toLongLong();
        return true;
      }
      return false;
    }

    virtual bool FindStringValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      const Value value(Storage, name);
      if (!value.IsValid())
      {
        return false;
      }
      const QVariant& var = value.Get();
      if (var.type() == QVariant::String)
      {
        val = FromQString(var.toString());
        return true;
      }
      return false;
    }

    virtual bool FindDataValue(const Parameters::NameType& name, Parameters::DataType& val) const
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

    virtual void Process(Parameters::Visitor& visitor) const
    {
      Require(!"Should not be called");
    }

    virtual void SetIntValue(const Parameters::NameType& name, Parameters::IntType val)
    {
      Value value(Storage, name);
      value.Set(QVariant(val));
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      Value value(Storage, name);
      value.Set(QVariant(ToQString(val)));
    }

    virtual void SetDataValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      Value value(Storage, name);
      const QByteArray arr(val.empty() ? QByteArray() : QByteArray(safe_ptr_cast<const char*>(&val[0]), val.size()));
      value.Set(QVariant(arr));
    }

    virtual void RemoveIntValue(const Parameters::NameType& name)
    {
      Value val(Storage, name);
      val.Remove();
    }

    virtual void RemoveStringValue(const Parameters::NameType& name)
    {
      Value val(Storage, name);
      val.Remove();
    }

    virtual void RemoveDataValue(const Parameters::NameType& name)
    {
      Value val(Storage, name);
      val.Remove();
    }
  private:
    typedef boost::shared_ptr<QSettings> SettingsPtr;
    typedef std::map<Parameters::NameType, SettingsPtr> SettingsStorage;

    class Value
    {
    public:
      Value(SettingsStorage& storage, const Parameters::NameType& name)
        : Storage(storage)
        , FullName(name)
      {
      }

      bool IsValid() const
      {
        return FullName.npos != FullName.find(Parameters::NAMESPACE_DELIMITER);
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
      static QString GetKeyName(const Parameters::NameType& name)
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
        const Parameters::NameType::size_type delimPos = FullName.find(Parameters::NAMESPACE_DELIMITER);
        const Parameters::NameType rootNamespace = FullName.substr(0, delimPos);
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
      const Parameters::NameType FullName;
      mutable SettingsPtr Setup;
      mutable QString ParamName;
    };
  private:
    mutable SettingsStorage Storage;
  };

  class GlobalOptionsStorage : public GlobalOptions
  {
  public:
    virtual Parameters::Container::Ptr Get() const
    {
      if (!Options)
      {
        Options = boost::make_shared<SettingsContainer>();
      }
      return Options;
    }
  private:
    mutable Parameters::Container::Ptr Options;
  };
}

GlobalOptions& GlobalOptions::Instance()
{
  static GlobalOptionsStorage storage;
  return storage;
}
