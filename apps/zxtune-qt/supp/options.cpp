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
      const QVariant value = FindValue(name);
      if (value.type() == QVariant::LongLong)
      {
        val = value.toLongLong();
        return true;
      }
      return false;
    }

    virtual bool FindStringValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      const QVariant value = FindValue(name);
      if (value.type() == QVariant::String)
      {
        val = FromQString(value.toString());
        return true;
      }
      return false;
    }

    virtual bool FindDataValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      const QVariant value = FindValue(name);
      if (value.type() == QVariant::ByteArray)
      {
        const QByteArray& arr = value.toByteArray();
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
      SetValue(name, QVariant(val));
    }

    virtual void SetStringValue(const Parameters::NameType& name, const Parameters::StringType& val)
    {
      SetValue(name, QVariant(ToQString(val)));
    }

    virtual void SetDataValue(const Parameters::NameType& name, const Parameters::DataType& val)
    {
      const QByteArray arr(val.empty() ? QByteArray() : QByteArray(safe_ptr_cast<const char*>(&val[0]), val.size()));
      SetValue(name, QVariant(arr));
    }

    virtual void RemoveIntValue(const Parameters::NameType& name)
    {
      RemoveValue(name);
    }

    virtual void RemoveStringValue(const Parameters::NameType& name)
    {
      RemoveValue(name);
    }

    virtual void RemoveDataValue(const Parameters::NameType& name)
    {
      RemoveValue(name);
    }
  private:
    static QString GetKeyName(const Parameters::NameType& name)
    {
      QString res(ToQString(name));
      res.replace('.', '/');
      return res;
    }

    typedef boost::shared_ptr<QSettings> SettingsPtr;

    std::pair<SettingsPtr, QString> GetStorageAndName(const Parameters::NameType& name) const
    {
      const Parameters::NameType::size_type delimPos = name.find(Parameters::NAMESPACE_DELIMITER);
      Require(delimPos != Parameters::NameType::npos);
      const Parameters::NameType rootNamespace = name.substr(0, delimPos);
      const QString paramName = GetKeyName(name.substr(delimPos + 1));
      const SettingsStorage::const_iterator it = Settings.find(rootNamespace);
      if (it != Settings.end())
      {
        return std::make_pair(it->second, paramName);
      }
      const SettingsPtr settings = boost::make_shared<QSettings>(Text::PROJECT_NAME, ToQString(rootNamespace));
      Settings.insert(SettingsStorage::value_type(rootNamespace, settings));
      return std::make_pair(settings, paramName);
    }

    QVariant FindValue(const Parameters::NameType& name) const
    {
      const std::pair<SettingsPtr, QString> setEntry = GetStorageAndName(name);
      return setEntry.first->value(setEntry.second);
    }

    void SetValue(const Parameters::NameType& name, const QVariant& value)
    {
      const std::pair<SettingsPtr, QString> setEntry = GetStorageAndName(name);
      setEntry.first->setValue(setEntry.second, value);
    }

    void RemoveValue(const Parameters::NameType& name)
    {
      const std::pair<SettingsPtr, QString> setEntry = GetStorageAndName(name);
      setEntry.first->remove(setEntry.second);
    }
  private:
    typedef std::map<Parameters::NameType, SettingsPtr> SettingsStorage;
    mutable SettingsStorage Settings;
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
