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

namespace
{
  using namespace Parameters;

  class BooleanValueImpl : public BooleanValue
  {
  public:
    BooleanValueImpl(QAction& parent, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue)
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
}

namespace Parameters
{
  BooleanValue::BooleanValue(QObject& parent) : QObject(&parent)
  {
  }

  void BooleanValue::Bind(QAction& action, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue)
  {
    new BooleanValueImpl(action, ctr, name, defValue);
  }
}
