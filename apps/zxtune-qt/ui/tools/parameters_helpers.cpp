/**
 *
 * @file
 *
 * @brief Parameters helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "parameters_helpers.h"
// common includes
#include <contract.h>
#include <string_view.h>
// library includes
#include <debug/log.h>
#include <math/numeric.h>
// std includes
#include <utility>
// qt includes
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QAction>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>

namespace
{
  using namespace Parameters;

  const Debug::Stream Dbg("Parameters::Helper");

  template<class Holder>
  class BooleanValueImpl : public BooleanValue
  {
  public:
    BooleanValueImpl(Holder& parent, Container& ctr, Identifier name, bool defValue, IntType oneValue)
      : BooleanValue(parent)
      , Parent(parent)
      , Storage(ctr)
      , Name(std::move(name))
      , Default(defValue)
      , OneValue(oneValue)
    {
      BooleanValueImpl<Holder>::Reload();
      Require(connect(&parent, &Holder::toggled, this, &BooleanValueImpl::Set));
    }

    void Reset() override
    {
      const AutoBlockSignal block(Parent);
      Storage.RemoveValue(Name);
      Reload();
    }

    void Reload() override
    {
      Parent.setChecked(GetValue());
    }

  private:
    void Set(bool value)
    {
      const IntType val = value ? OneValue : 0;
      Dbg("{}={}", static_cast<StringView>(Name), val);
      Storage.SetValue(Name, val);
    }

    bool GetValue() const
    {
      if (const auto val = Storage.FindInteger(Name))
      {
        return *val != 0;
      }
      return Default ? OneValue : 0;
    }

  private:
    Holder& Parent;
    Container& Storage;
    const Identifier Name;
    const bool Default;
    const IntType OneValue;
  };

  class StringSetValue : public ExclusiveValue
  {
  public:
    StringSetValue(QAbstractButton& parent, Container& ctr, Identifier name, StringView value)
      : ExclusiveValue(parent)
      , Parent(parent)
      , Storage(ctr)
      , Name(std::move(name))
      , Value(value)
    {
      StringSetValue::Reload();
      Require(connect(&parent, &QAbstractButton::toggled, this, &StringSetValue::Set));
    }

    void Reset() override
    {
      const AutoBlockSignal block(Parent);
      Storage.RemoveValue(Name);
      Reload();
    }

    void Reload() override
    {
      Parent.setChecked(GetValue() == Value);
    }

  private:
    void Set(bool value)
    {
      if (value)
      {
        Dbg("{}={}", static_cast<StringView>(Name), Value);
        Storage.SetValue(Name, Value);
      }
    }

    StringType GetValue() const
    {
      return Parameters::GetString(Storage, Name);
    }

  private:
    QAbstractButton& Parent;
    Container& Storage;
    const Identifier Name;
    const StringType Value;
  };

  template<class Holder>
  void SetWidgetValue(Holder& holder, int val)
  {
    holder.setValue(val);
  }

  template<class Holder>
  void ConnectChanges(Holder& holder, IntegerValue& val)
  {
    Require(QObject::connect(&holder, qOverload<int>(&Holder::valueChanged), &val, &IntegerValue::Set));
  }

  void SetWidgetValue(QComboBox& holder, int val)
  {
    holder.setCurrentIndex(val);
  }

  void ConnectChanges(QComboBox& holder, IntegerValue& val)
  {
    Require(QObject::connect(&holder, qOverload<int>(&QComboBox::currentIndexChanged), &val, &IntegerValue::Set));
  }

  template<class Holder>
  class IntegerValueImpl : public IntegerValue
  {
  public:
    IntegerValueImpl(Holder& parent, Container& ctr, Identifier name, int defValue)
      : IntegerValue(parent)
      , Parent(parent)
      , Storage(ctr)
      , Name(std::move(name))
      , Default(defValue)
    {
      IntegerValueImpl<Holder>::Reload();
      ConnectChanges(Parent, *this);
    }

    void Set(int value) override
    {
      Dbg("{}={}", static_cast<StringView>(Name), value);
      Storage.SetValue(Name, value);
    }

    void Reset() override
    {
      const AutoBlockSignal block(Parent);
      Storage.RemoveValue(Name);
      Reload();
    }

    void Reload() override
    {
      SetWidgetValue(Parent, GetValue());
    }

  private:
    int GetValue() const
    {
      return Parameters::GetInteger<int>(Storage, Name, Default);
    }

  private:
    Holder& Parent;
    Container& Storage;
    const Identifier Name;
    const int Default;
  };

  template<class Holder>
  class IntegerValueControl : public IntegerValue
  {
  public:
    IntegerValueControl(Holder& parent, Integer::Ptr val)
      : IntegerValue(parent)
      , Parent(parent)
      , Value(std::move(val))
    {
      IntegerValueControl<Holder>::Reload();
      ConnectChanges(Parent, *this);
    }

    void Set(int value) override
    {
      Value->Set(value);
    }

    void Reset() override
    {
      const AutoBlockSignal block(Parent);
      Value->Reset();
      Reload();
    }

    void Reload() override
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
    BigIntegerValueImpl(QLineEdit& parent, Container& ctr, IntegerTraits traits)
      : BigIntegerValue(parent)
      , Parent(parent)
      , Storage(ctr)
      , Traits(traits)
    {
      BigIntegerValueImpl::Reload();
      Require(connect(&parent, &QLineEdit::textChanged, this, &BigIntegerValueImpl::Set));
      Require(connect(&parent, &QLineEdit::editingFinished, this, &Value::Reload));
    }

    void Reset() override
    {
      const AutoBlockSignal block(Parent);
      Storage.RemoveValue(Traits.Name);
      Reload();
    }

    void Reload() override
    {
      const IntType val = GetValue();
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
    void Set(const QString& value)
    {
      const IntType val = value.toLongLong();
      if (Math::InRange(val, Traits.Min, Traits.Max))
      {
        Dbg("{}={}", static_cast<StringView>(Traits.Name), val);
        Storage.SetValue(Traits.Name, val);
      }
    }

    IntType GetValue() const
    {
      return Parameters::GetInteger<IntType>(Storage, Traits.Name, Traits.Default);
    }

  private:
    QLineEdit& Parent;
    Container& Storage;
    const IntegerTraits Traits;
  };

  class StringValueImpl : public StringValue
  {
  public:
    StringValueImpl(QLineEdit& parent, Container& ctr, Identifier name, StringView defValue)
      : StringValue(parent)
      , Parent(parent)
      , Storage(ctr)
      , Name(std::move(name))
      , Default(defValue)
    {
      StringValueImpl::Reload();
      Require(connect(&parent, &QLineEdit::textChanged, this, &StringValueImpl::Set));
    }

    void Reset() override
    {
      const AutoBlockSignal block(Parent);
      Storage.RemoveValue(Name);
      Reload();
    }

    void Reload() override
    {
      Parent.setText(ToQString(GetValue()));
    }

  private:
    void Set(const QString& value)
    {
      const auto val = FromQString(value);
      Dbg("{}={}", static_cast<StringView>(Name), val);
      Storage.SetValue(Name, val);
    }

    StringType GetValue() const
    {
      return Parameters::GetString(Storage, Name, Default);
    }

  private:
    QLineEdit& Parent;
    Container& Storage;
    const Identifier Name;
    const StringType Default;
  };
}  // namespace

namespace Parameters
{
  Value::Value(QObject& parent)
    : QObject(&parent)
  {}

  BooleanValue::BooleanValue(QObject& parent)
    : Value(parent)
  {}

  ExclusiveValue::ExclusiveValue(QObject& parent)
    : Value(parent)
  {}

  IntegerValue::IntegerValue(QObject& parent)
    : Value(parent)
  {}

  BigIntegerValue::BigIntegerValue(QObject& parent)
    : Value(parent)
  {}

  StringValue::StringValue(QObject& parent)
    : Value(parent)
  {}

  Value* BooleanValue::Bind(QAction& action, Container& ctr, Identifier name, bool defValue)
  {
    return new BooleanValueImpl<QAction>(action, ctr, name, defValue, 1);
  }

  Value* BooleanValue::Bind(QAbstractButton& button, Container& ctr, Identifier name, bool defValue, IntType oneValue)
  {
    return new BooleanValueImpl<QAbstractButton>(button, ctr, name, defValue, oneValue);
  }

  Value* BooleanValue::Bind(QGroupBox& box, Container& ctr, Identifier name, bool defValue, IntType oneValue)
  {
    return new BooleanValueImpl<QGroupBox>(box, ctr, name, defValue, oneValue);
  }

  Value* ExclusiveValue::Bind(QAbstractButton& button, Container& ctr, Identifier name, StringView value)
  {
    return new StringSetValue(button, ctr, name, value);
  }

  Value* IntegerValue::Bind(QComboBox& combo, Container& ctr, Identifier name, int defValue)
  {
    return new IntegerValueImpl<QComboBox>(combo, ctr, name, defValue);
  }

  Value* IntegerValue::Bind(QSlider& slider, Container& ctr, Identifier name, int defValue)
  {
    return new IntegerValueImpl<QSlider>(slider, ctr, name, defValue);
  }

  Value* IntegerValue::Bind(QSpinBox& spinbox, Container& ctr, Identifier name, int defValue)
  {
    return new IntegerValueImpl<QSpinBox>(spinbox, ctr, name, defValue);
  }

  Value* IntegerValue::Bind(QComboBox& combo, Integer::Ptr val)
  {
    return new IntegerValueControl<QComboBox>(combo, std::move(val));
  }

  Value* BigIntegerValue::Bind(QLineEdit& edit, Container& ctr, const IntegerTraits& traits)
  {
    return new BigIntegerValueImpl(edit, ctr, traits);
  }

  Value* StringValue::Bind(QLineEdit& edit, Container& ctr, Identifier name, StringView defValue)
  {
    return new StringValueImpl(edit, ctr, name, defValue);
  }
}  // namespace Parameters
