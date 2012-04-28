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
//common includes
#include <logging.h>
//qt includes
#include <QtGui/QAbstractButton>
#include <QtGui/QAction>
#include <QtGui/QComboBox>
#include <QtGui/QSlider>

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
      Container.FindIntValue(Name, val);
      parent.setChecked(val != 0);
      this->connect(&parent, SIGNAL(toggled(bool)), SLOT(SetValue(bool)));
    }

    virtual void SetValue(bool value)
    {
      const Parameters::IntType val = value ? OneValue : 0;
      Log::Debug("Parameters::Helper", "%1%=%2%", Name, val);
      Container.SetIntValue(Name, val);
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
        Container.SetStringValue(Name, Value);
      }
    }
  private:
    Parameters::StringType GetValue() const
    {
      Parameters::StringType value;
      Container.FindStringValue(Name, value);
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

    IntegerValueImpl(QSlider& parent, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
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
      Container.SetIntValue(Name, value);
    }
  private:
    int GetValue() const
    {
      Parameters::IntType value = DefValue;
      Container.FindIntValue(Name, value);
      return value;
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const int DefValue;
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

  void BooleanValue::Bind(QAction& action, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue)
  {
    new BooleanValueImpl(action, ctr, name, defValue, 1);
  }

  void BooleanValue::Bind(QAbstractButton& button, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue, const Parameters::IntType& oneValue)
  {
    new BooleanValueImpl(button, ctr, name, defValue, oneValue);
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
}
