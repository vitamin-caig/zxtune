/*
Abstract:
  Parameters helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "parameters_helpers.h"
#include "ui/utils.h"
//common includes
#include <logging.h>
//qt includes
#include <QtGui/QAbstractButton>
#include <QtGui/QAction>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QSlider>
#include <QtGui/QSpinBox>

namespace
{
  using namespace Parameters;

  class BooleanValueImpl : public BooleanValue
  {
  public:
    template<class Holder>
    BooleanValueImpl(Holder& parent, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue, const Parameters::IntType& oneValue)
      : BooleanValue(parent)
      , Container(ctr)
      , Name(name)
      , OneValue(oneValue)
    {
      Parameters::IntType val = defValue ? oneValue : 0;
      Container.FindValue(Name, val);
      parent.setChecked(val != 0);
      this->connect(&parent, SIGNAL(toggled(bool)), SLOT(SetValue(bool)));
    }

    virtual void SetValue(bool value)
    {
      const Parameters::IntType val = value ? OneValue : 0;
      Log::Debug("Parameters::Helper", "%1%=%2%", Name, val);
      Container.SetValue(Name, val);
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const Parameters::IntType OneValue;
  };

  class StringSetValue : public ExclusiveValue
  {
  public:
    StringSetValue(QAbstractButton& parent, Parameters::Container& ctr, const Parameters::NameType& name, const Parameters::StringType& value)
      : ExclusiveValue(parent)
      , Container(ctr)
      , Name(name)
      , Value(value)
    {
      parent.setChecked(GetValue() == Value);
      this->connect(&parent, SIGNAL(toggled(bool)), SLOT(SetValue(bool)));
    }

    virtual void SetValue(bool value)
    {
      if (value)
      {
        Log::Debug("Parameters::Helper", "%1%=%2%", Name, Value);
        Container.SetValue(Name, Value);
      }
    }
  private:
    Parameters::StringType GetValue() const
    {
      Parameters::StringType value;
      Container.FindValue(Name, value);
      return value;
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const Parameters::StringType Value;
  };

  class IntegerValueImpl : public IntegerValue
  {
  public:
    IntegerValueImpl(QComboBox& parent, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
      : IntegerValue(parent)
      , Container(ctr)
      , Name(name)
      , DefValue(defValue)
    {
      parent.setCurrentIndex(GetValue());
      this->connect(&parent, SIGNAL(currentIndexChanged(int)), SLOT(SetValue(int)));
    }

    template<class Holder>
    IntegerValueImpl(Holder& parent, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
      : IntegerValue(parent)
      , Container(ctr)
      , Name(name)
      , DefValue(defValue)
    {
      parent.setValue(GetValue());
      this->connect(&parent, SIGNAL(valueChanged(int)), SLOT(SetValue(int)));
    }

    virtual void SetValue(int value)
    {
      Log::Debug("Parameters::Helper", "%1%=%2%", Name, value);
      Container.SetValue(Name, value);
    }
  private:
    int GetValue() const
    {
      Parameters::IntType value = DefValue;
      Container.FindValue(Name, value);
      return value;
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const int DefValue;
  };
  
  class BigIntegerValueImpl : public BigIntegerValue
  {
  public:
    BigIntegerValueImpl(QLineEdit& parent, Parameters::Container& ctr, const Parameters::NameType& name, Parameters::IntType defValue)
      : BigIntegerValue(parent)
      , Container(ctr)
      , Name(name)
    {
      Parameters::IntType value = defValue;
      Container.FindValue(Name, value);
      parent.setText(QString::number(value));
      this->connect(&parent, SIGNAL(textChanged(const QString&)), SLOT(SetValue(const QString&)));
    }


    virtual void SetValue(const QString& value)
    {
      const Parameters::IntType val = value.toLongLong();
      Log::Debug("Parameters::Helper", "%1%=%2%", Name, val);
      Container.SetValue(Name, val);
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType Name;
  };

  class StringValueImpl : public StringValue
  {
  public:
    StringValueImpl(QLineEdit& parent, Parameters::Container& ctr, const Parameters::NameType& name, const Parameters::StringType& defValue)
      : StringValue(parent)
      , Container(ctr)
      , Name(name)
    {
      Parameters::StringType value = defValue;
      Container.FindValue(Name, value);
      parent.setText(ToQString(value));
      this->connect(&parent, SIGNAL(textChanged(const QString&)), SLOT(SetValue(const QString&)));
    }


    virtual void SetValue(const QString& value)
    {
      const Parameters::StringType& val = FromQString(value);
      Log::Debug("Parameters::Helper", "%1%=%2%", Name, val);
      Container.SetValue(Name, val);
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType Name;
  };
}

namespace Parameters
{
  BooleanValue::BooleanValue(QObject& parent) : QObject(&parent)
  {
  }

  ExclusiveValue::ExclusiveValue(QObject& parent) : QObject(&parent)
  {
  }

  IntegerValue::IntegerValue(QObject& parent) : QObject(&parent)
  {
  }

  BigIntegerValue::BigIntegerValue(QObject &parent) : QObject(&parent)
  {
  }

  StringValue::StringValue(QObject &parent) : QObject(&parent)
  {
  }

  void BooleanValue::Bind(QAction& action, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue)
  {
    new BooleanValueImpl(action, ctr, name, defValue, 1);
  }

  void BooleanValue::Bind(QAbstractButton& button, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue, const Parameters::IntType& oneValue)
  {
    new BooleanValueImpl(button, ctr, name, defValue, oneValue);
  }

  void BooleanValue::Bind(QGroupBox& box, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue, const Parameters::IntType& oneValue)
  {
    new BooleanValueImpl(box, ctr, name, defValue, oneValue);
  }

  void ExclusiveValue::Bind(QAbstractButton& button, Parameters::Container& ctr, const Parameters::NameType& name, const Parameters::StringType& value)
  {
    new StringSetValue(button, ctr, name, value);
  }

  void IntegerValue::Bind(QComboBox& combo, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
  {
    new IntegerValueImpl(combo, ctr, name, defValue);
  }

  void IntegerValue::Bind(QSlider& slider, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
  {
    new IntegerValueImpl(slider, ctr, name, defValue);
  }

  void IntegerValue::Bind(QSpinBox& spinbox, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
  {
    new IntegerValueImpl(spinbox, ctr, name, defValue);
  }

  void BigIntegerValue::Bind(QLineEdit& edit, Parameters::Container& ctr, const Parameters::NameType& name, Parameters::IntType defValue)
  {
    new BigIntegerValueImpl(edit, ctr, name, defValue);
  }

  void StringValue::Bind(QLineEdit& edit, Parameters::Container& ctr, const Parameters::NameType& name, const Parameters::StringType& defValue)
  {
    new StringValueImpl(edit, ctr, name, defValue);
  }
}
