/**
* 
* @file
*
* @brief Parameters helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <debug/log.h>
#include <math/numeric.h>
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

  const Debug::Stream Dbg("Parameters::Helper");

  template<class Holder>
  class BooleanValueImpl : public BooleanValue
  {
  public:
    BooleanValueImpl(Holder& parent, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue, const Parameters::IntType& oneValue)
      : BooleanValue(parent)
      , Parent(parent)
      , Container(ctr)
      , Name(name)
      , Default(defValue)
      , OneValue(oneValue)
    {
      BooleanValueImpl<Holder>::Reload();
      Require(connect(&parent, SIGNAL(toggled(bool)), SLOT(Set(bool))));
    }

    virtual void Set(bool value)
    {
      const Parameters::IntType val = value ? OneValue : 0;
      Dbg("%1%=%2%", Name.FullPath(), val);
      Container.SetValue(Name, val);
    }

    virtual void Reset()
    {
      const AutoBlockSignal block(Parent);
      Container.RemoveValue(Name);
      Reload();
    }

    virtual void Reload()
    {
      Parent.setChecked(GetValue());
    }
  private:
    bool GetValue() const
    {
      Parameters::IntType val = Default ? OneValue : 0;
      Container.FindValue(Name, val);
      return val != 0;
    }
  private:
    Holder& Parent;
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const bool Default;
    const Parameters::IntType OneValue;
  };

  class StringSetValue : public ExclusiveValue
  {
  public:
    StringSetValue(QAbstractButton& parent, Parameters::Container& ctr, const Parameters::NameType& name, const Parameters::StringType& value)
      : ExclusiveValue(parent)
      , Parent(parent)
      , Container(ctr)
      , Name(name)
      , Value(value)
    {
      StringSetValue::Reload();
      Require(connect(&parent, SIGNAL(toggled(bool)), SLOT(Set(bool))));
    }

    virtual void Set(bool value)
    {
      if (value)
      {
        Dbg("%1%=%2%", Name.FullPath(), Value);
        Container.SetValue(Name, Value);
      }
    }

    virtual void Reset()
    {
      const AutoBlockSignal block(Parent);
      Container.RemoveValue(Name);
      Reload();
    }

    virtual void Reload()
    {
      Parent.setChecked(GetValue() == Value);
    }
  private:
    Parameters::StringType GetValue() const
    {
      Parameters::StringType value;
      Container.FindValue(Name, value);
      return value;
    }
  private:
    QAbstractButton& Parent;
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const Parameters::StringType Value;
  };

  template<class Holder>
  void SetWidgetValue(Holder& holder, int val)
  {
    holder.setValue(val);
  }

  template<class Holder>
  void ConnectChanges(Holder& holder, IntegerValue& val)
  {
    Require(val.connect(&holder, SIGNAL(valueChanged(int)), SLOT(Set(int))));
  }

  void SetWidgetValue(QComboBox& holder, int val)
  {
    holder.setCurrentIndex(val);
  }

  void ConnectChanges(QComboBox& holder, IntegerValue& val)
  {
    Require(val.connect(&holder, SIGNAL(currentIndexChanged(int)), SLOT(Set(int))));
  }

  template<class Holder>
  class IntegerValueImpl : public IntegerValue
  {
  public:
    IntegerValueImpl(Holder& parent, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
      : IntegerValue(parent)
      , Parent(parent)
      , Container(ctr)
      , Name(name)
      , Default(defValue)
    {
      IntegerValueImpl<Holder>::Reload();
      ConnectChanges(Parent, *this);
    }

    virtual void Set(int value)
    {
      Dbg("%1%=%2%", Name.FullPath(), value);
      Container.SetValue(Name, value);
    }

    virtual void Reset()
    {
      const AutoBlockSignal block(Parent);
      Container.RemoveValue(Name);
      Reload();
    }

    virtual void Reload()
    {
      SetWidgetValue(Parent, GetValue());
    }
  private:
    int GetValue() const
    {
      Parameters::IntType value = Default;
      Container.FindValue(Name, value);
      return value;
    }
  private:
    Holder& Parent;
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const int Default;
  };

  template<class Holder>
  class IntegerValueControl : public IntegerValue
  {
  public:
    IntegerValueControl(Holder& parent, Integer::Ptr val)
      : IntegerValue(parent)
      , Parent(parent)
      , Value(val)
    {
      IntegerValueControl<Holder>::Reload();
      ConnectChanges(Parent, *this);
    }

    virtual void Set(int value)
    {
      Value->Set(value);
    }

    virtual void Reset()
    {
      const AutoBlockSignal block(Parent);
      Value->Reset();
      Reload();
    }

    virtual void Reload()
    {
      SetWidgetValue(Parent, Value->Get());
    }
  private:
    Holder& Parent;
    const Integer::Ptr Value;
  };

  class BigIntegerValueImpl : public BigIntegerValue
  {
  public:
    BigIntegerValueImpl(QLineEdit& parent, Parameters::Container& ctr, const IntegerTraits& traits)
      : BigIntegerValue(parent)
      , Parent(parent)
      , Container(ctr)
      , Traits(traits)
    {
      BigIntegerValueImpl::Reload();
      Require(connect(&parent, SIGNAL(textChanged(const QString&)), SLOT(Set(const QString&))));
      Require(connect(&parent, SIGNAL(editingFinished()), SLOT(Reload())));
    }


    virtual void Set(const QString& value)
    {
      const Parameters::IntType val = value.toLongLong();
      if (Math::InRange(val, Traits.Min, Traits.Max))
      {
        Dbg("%1%=%2%", Traits.Name.FullPath(), val);
        Container.SetValue(Traits.Name, val);
      }
    }

    virtual void Reset()
    {
      const AutoBlockSignal block(Parent);
      Container.RemoveValue(Traits.Name);
      Reload();
    }

    virtual void Reload()
    {
      const Parameters::IntType val = GetValue();
      if (Math::InRange(val, Traits.Min, Traits.Max))
      {
        Parent.setText(QString::number(val));
      }
      else
      {
        Parent.clear();
      }
    }
  private:
    Parameters::IntType GetValue() const
    {
      Parameters::IntType value = Traits.Default;
      Container.FindValue(Traits.Name, value);
      return value;
    }
  private:
    QLineEdit& Parent;
    Parameters::Container& Container;
    const Parameters::IntegerTraits Traits;
  };

  class StringValueImpl : public StringValue
  {
  public:
    StringValueImpl(QLineEdit& parent, Parameters::Container& ctr, const Parameters::NameType& name, const Parameters::StringType& defValue)
      : StringValue(parent)
      , Parent(parent)
      , Container(ctr)
      , Name(name)
      , Default(defValue)
    {
      StringValueImpl::Reload();
      Require(connect(&parent, SIGNAL(textChanged(const QString&)), SLOT(Set(const QString&))));
    }

    virtual void Set(const QString& value)
    {
      const Parameters::StringType& val = FromQString(value);
      Dbg("%1%=%2%", Name.FullPath(), val);
      Container.SetValue(Name, val);
    }

    virtual void Reset()
    {
      const AutoBlockSignal block(Parent);
      Container.RemoveValue(Name);
      Reload();
    }

    virtual void Reload()
    {
      Parent.setText(ToQString(GetValue()));
    }
  private:
    Parameters::StringType GetValue() const
    {
      Parameters::StringType value = Default;
      Container.FindValue(Name, value);
      return value;
    }
  private:
    QLineEdit& Parent;
    Parameters::Container& Container;
    const Parameters::NameType Name;
    const Parameters::StringType Default;
  };
}

namespace Parameters
{
  Value::Value(QObject& parent) : QObject(&parent)
  {
  }

  BooleanValue::BooleanValue(QObject& parent) : Value(parent)
  {
  }

  ExclusiveValue::ExclusiveValue(QObject& parent) : Value(parent)
  {
  }

  IntegerValue::IntegerValue(QObject& parent) : Value(parent)
  {
  }

  BigIntegerValue::BigIntegerValue(QObject &parent) : Value(parent)
  {
  }

  StringValue::StringValue(QObject &parent) : Value(parent)
  {
  }

  Value* BooleanValue::Bind(QAction& action, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue)
  {
    return new BooleanValueImpl<QAction>(action, ctr, name, defValue, 1);
  }

  Value* BooleanValue::Bind(QAbstractButton& button, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue, const Parameters::IntType& oneValue)
  {
    return new BooleanValueImpl<QAbstractButton>(button, ctr, name, defValue, oneValue);
  }

  Value* BooleanValue::Bind(QGroupBox& box, Parameters::Container& ctr, const Parameters::NameType& name, bool defValue, const Parameters::IntType& oneValue)
  {
    return new BooleanValueImpl<QGroupBox>(box, ctr, name, defValue, oneValue);
  }

  Value* ExclusiveValue::Bind(QAbstractButton& button, Parameters::Container& ctr, const Parameters::NameType& name, const Parameters::StringType& value)
  {
    return new StringSetValue(button, ctr, name, value);
  }

  Value* IntegerValue::Bind(QComboBox& combo, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
  {
    return new IntegerValueImpl<QComboBox>(combo, ctr, name, defValue);
  }

  Value* IntegerValue::Bind(QSlider& slider, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
  {
    return new IntegerValueImpl<QSlider>(slider, ctr, name, defValue);
  }

  Value* IntegerValue::Bind(QSpinBox& spinbox, Parameters::Container& ctr, const Parameters::NameType& name, int defValue)
  {
    return new IntegerValueImpl<QSpinBox>(spinbox, ctr, name, defValue);
  }

  Value* IntegerValue::Bind(QComboBox& combo, Integer::Ptr val)
  {
    return new IntegerValueControl<QComboBox>(combo, val);
  }

  Value* BigIntegerValue::Bind(QLineEdit& edit, Parameters::Container& ctr, const IntegerTraits& traits)
  {
    return new BigIntegerValueImpl(edit, ctr, traits);
  }

  Value* StringValue::Bind(QLineEdit& edit, Parameters::Container& ctr, const Parameters::NameType& name, const Parameters::StringType& defValue)
  {
    return new StringValueImpl(edit, ctr, name, defValue);
  }
}
