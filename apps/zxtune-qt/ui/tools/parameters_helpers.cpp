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

namespace
{
  using namespace Parameters;

  class BooleanValueImpl : public BooleanValue
  {
  public:
    template<class Holder>
    BooleanValueImpl(Holder& parent, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue)
      : BooleanValue(parent)
      , Container(ctr)
      , Name(name)
      , DefValue(defValue)
    {
      parent.setChecked(GetValue());
      this->connect(&parent, SIGNAL(toggled(bool)), SLOT(SetValue(bool)));
    }

    virtual void SetValue(bool value)
    {
      Log::Debug("Parameters::Helper", "%1%=%2%", Name, value ? "true" : "false");
      Container.SetIntValue(Name, value);
    }
  private:
    bool GetValue() const
    {
      Parameters::IntType value = 0;
      if (Container.FindIntValue(Name, value))
      {
        return value != 0;
      }
      return DefValue;
    }
  private:
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const bool DefValue;
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

    virtual void SetValue(int value)
    {
      Log::Debug("Parameters::Helper", "%1%=%2%", Name, value);
      Container.SetIntValue(Name, value);
    }
  private:
    int GetValue() const
    {
      Parameters::IntType value = 0;
      if (Container.FindIntValue(Name, value))
      {
        return value != 0;
      }
      return DefValue;
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

  IntegerValue::IntegerValue(QObject& parent) : QObject(&parent)
  {
  }

  void BooleanValue::Bind(QAction& action, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue)
  {
    new BooleanValueImpl(action, ctr, name, defValue);
  }

  void BooleanValue::Bind(QAbstractButton& button, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue)
  {
    new BooleanValueImpl(button, ctr, name, defValue);
  }

  void IntegerValue::Bind(QComboBox& combo, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
  {
    new IntegerValueImpl(combo, ctr, name, defValue);
  }
}
