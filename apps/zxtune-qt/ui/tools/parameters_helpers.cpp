/**
 *
 * @file
 *
 * @brief Parameters helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"

#include "debug/log.h"
#include "math/numeric.h"

#include "contract.h"
#include "string_view.h"

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QAction>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>

#include <algorithm>
#include <optional>
#include <utility>

namespace
{
  using namespace Parameters;
  using namespace UI;

  const Debug::Stream Dbg("Parameters::Helper");

  class LoggingContainer : public Container
  {
  public:
    explicit LoggingContainer(Container& delegate)
      : Delegate(delegate)
    {}

    uint_t Version() const override
    {
      return Delegate.Version();
    }

    std::optional<IntType> FindInteger(Identifier name) const override
    {
      return Delegate.FindInteger(name);
    }

    std::optional<StringType> FindString(Identifier name) const override
    {
      return Delegate.FindString(name);
    }

    Binary::Data::Ptr FindData(Identifier name) const override
    {
      return Delegate.FindData(name);
    }

    void Process(Visitor& visitor) const override
    {
      Delegate.Process(visitor);
    }

    void SetValue(Identifier name, IntType val) override
    {
      Log(name, val);
      Delegate.SetValue(name, val);
    }

    void SetValue(Identifier name, StringView val) override
    {
      Log(name, val);
      Delegate.SetValue(name, val);
    }

    void SetValue(Identifier name, Binary::View val) override
    {
      Delegate.SetValue(name, val);
    }

    void RemoveValue(Identifier name) override
    {
      Delegate.RemoveValue(name);
    }

  private:
    template<class T>
    static void Log(Identifier name, const T& val)
    {
      Dbg("{}={}", static_cast<StringView>(name), val);
    }

  private:
    Container& Delegate;
  };

  class BaseValue : public Value
  {
  public:
    BaseValue(QObject& parent, Container& ctr, Identifier name)
      : Value(parent)
      , Storage(ctr)
      , Name(std::move(name))
    {}

    void Reset() override
    {
      const AutoBlockSignal block(*parent());
      Storage.RemoveValue(Name);
      Reload();
    }

  protected:
    // Value is a QObject child of the bound widget
    template<class Holder>
    Holder& Parent() const
    {
      return *static_cast<Holder*>(parent());
    }

    LoggingContainer Storage;
    const Identifier Name;
  };

  // Radio groups share one parameter: an unset falseValue makes an unchecked button
  // write nothing, so it cannot clobber the newly selected one
  template<class Holder, class T>
  class BooleanValue : public BaseValue
  {
  public:
    BooleanValue(Holder& parent, Container& ctr, Identifier name, T trueValue, std::optional<T> falseValue,
                 bool defValue = false)
      : BaseValue(parent, ctr, std::move(name))
      , TrueValue(std::move(trueValue))
      , FalseValue(std::move(falseValue))
      , Default(defValue)
    {
      BooleanValue<Holder, T>::Reload();
      Require(connect(&parent, &Holder::toggled, this, &BooleanValue::Set));
    }

    void Reload() override
    {
      Parent<Holder>().setChecked(GetValue());
    }

  private:
    void Set(bool checked)
    {
      if (checked)
      {
        Storage.SetValue(Name, TrueValue);
      }
      else if (FalseValue)
      {
        Storage.SetValue(Name, *FalseValue);
      }
    }

    bool GetValue() const
    {
      T stored{};
      if (Parameters::FindValue(Storage, Name, stored))
      {
        return stored == TrueValue;
      }
      return Default;
    }

  private:
    const T TrueValue;
    const std::optional<T> FalseValue;
    const bool Default;
  };

  template<class Holder>
  void SetWidgetValue(Holder& holder, int val)
  {
    holder.setValue(val);
  }

  template<class Holder, class Target>
  void ConnectChanges(Holder& holder, Target& val)
  {
    Require(QObject::connect(&holder, qOverload<int>(&Holder::valueChanged), &val, &Target::Set));
  }

  void SetWidgetValue(QComboBox& holder, int val)
  {
    holder.setCurrentIndex(val);
  }

  template<class Target>
  void ConnectChanges(QComboBox& holder, Target& val)
  {
    Require(QObject::connect(&holder, qOverload<int>(&QComboBox::currentIndexChanged), &val, &Target::Set));
  }

  template<class Holder>
  class IntegerValue : public BaseValue
  {
  public:
    IntegerValue(Holder& parent, Container& ctr, Identifier name, int defValue)
      : BaseValue(parent, ctr, std::move(name))
      , Default(defValue)
    {
      IntegerValue<Holder>::Reload();
      ConnectChanges(Parent<Holder>(), *this);
    }

    void Set(int value)
    {
      Storage.SetValue(Name, value);
    }

    void Reload() override
    {
      SetWidgetValue(Parent<Holder>(), GetValue());
    }

  private:
    int GetValue() const
    {
      return Parameters::GetInteger<int>(Storage, Name, Default);
    }

  private:
    const int Default;
  };

  StringType LoadValue(const Accessor& ac, Identifier name, StringView def)
  {
    return Parameters::GetString(ac, name, def);
  }

  IntType LoadValue(const Accessor& ac, Identifier name, IntType def)
  {
    return Parameters::GetInteger<IntType>(ac, name, def);
  }

  template<class T>
  class IndexedValue : public BaseValue
  {
  public:
    IndexedValue(QComboBox& parent, Container& ctr, Identifier name, std::vector<T> values, T defValue)
      : BaseValue(parent, ctr, std::move(name))
      , Values(std::move(values))
      , Default(std::move(defValue))
    {
      Require(parent.count() == static_cast<int>(Values.size()));
      Reload();
      ConnectChanges(parent, *this);
    }

    void Set(int index)
    {
      if (index >= 0 && index < static_cast<int>(Values.size()))
      {
        Storage.SetValue(Name, Values[index]);
      }
    }

    void Reload() override
    {
      Parent<QComboBox>().setCurrentIndex(GetIndex());
    }

  private:
    int GetIndex() const
    {
      const T val = LoadValue(Storage, Name, Default);
      const auto it = std::find(Values.begin(), Values.end(), val);
      return it != Values.end() ? static_cast<int>(it - Values.begin()) : -1;
    }

  private:
    const std::vector<T> Values;
    const T Default;
  };

  class BigIntegerValue : public BaseValue
  {
  public:
    BigIntegerValue(QLineEdit& parent, Container& ctr, IntegerTraits traits)
      : BaseValue(parent, ctr, traits.Name)
      , Traits(traits)
    {
      BigIntegerValue::Reload();
      Require(connect(&parent, &QLineEdit::textChanged, this, &BigIntegerValue::Set));
      Require(connect(&parent, &QLineEdit::editingFinished, this, &UI::Value::Reload));
    }

    void Reload() override
    {
      const IntType val = GetValue();
      auto& parent = Parent<QLineEdit>();
      if (Math::InRange(val, Traits.Min, Traits.Max))
      {
        parent.setText(QString::number(val));
      }
      else
      {
        parent.clear();
      }
    }

  private:
    void Set(const QString& value)
    {
      const IntType val = value.toLongLong();
      if (Math::InRange(val, Traits.Min, Traits.Max))
      {
        Storage.SetValue(Name, val);
      }
    }

    IntType GetValue() const
    {
      return Parameters::GetInteger<IntType>(Storage, Name, Traits.Default);
    }

  private:
    const IntegerTraits Traits;
  };

  class StringValue : public BaseValue
  {
  public:
    StringValue(QLineEdit& parent, Container& ctr, Identifier name, StringView defValue)
      : BaseValue(parent, ctr, std::move(name))
      , Default(defValue)
    {
      StringValue::Reload();
      Require(connect(&parent, &QLineEdit::textChanged, this, &StringValue::Set));
    }

    void Reload() override
    {
      Parent<QLineEdit>().setText(ToQString(GetValue()));
    }

  private:
    void Set(const QString& value)
    {
      Storage.SetValue(Name, FromQString(value));
    }

    StringType GetValue() const
    {
      return Parameters::GetString(Storage, Name, Default);
    }

  private:
    const StringType Default;
  };
}  // namespace

namespace UI
{
  using ::Parameters::Container;
  using ::Parameters::Identifier;
  using ::Parameters::IntType;

  Value::Value(QObject& parent)
    : QObject(&parent)
  {}

  Value* Value::Bind(QAction& action, Container& ctr, Identifier name, bool defValue)
  {
    return new BooleanValue<QAction, IntType>(action, ctr, name, 1, IntType(0), defValue);
  }

  Value* Value::Bind(QAbstractButton& button, Container& ctr, Identifier name, bool defValue, IntType trueValue)
  {
    return new BooleanValue<QAbstractButton, IntType>(button, ctr, name, trueValue, IntType(0), defValue);
  }

  Value* Value::BindExclusive(QAbstractButton& button, Container& ctr, Identifier name, StringView trueValue)
  {
    return new BooleanValue<QAbstractButton, StringType>(button, ctr, name, String(trueValue), std::nullopt);
  }

  Value* Value::Bind(QGroupBox& box, Container& ctr, Identifier name, bool defValue, IntType trueValue)
  {
    return new BooleanValue<QGroupBox, IntType>(box, ctr, name, trueValue, IntType(0), defValue);
  }

  Value* Value::Bind(QComboBox& combo, Container& ctr, Identifier name, int defValue)
  {
    return new IntegerValue<QComboBox>(combo, ctr, name, defValue);
  }

  Value* Value::Bind(QSlider& slider, Container& ctr, Identifier name, int defValue)
  {
    return new IntegerValue<QSlider>(slider, ctr, name, defValue);
  }

  Value* Value::Bind(QSpinBox& spinbox, Container& ctr, Identifier name, int defValue)
  {
    return new IntegerValue<QSpinBox>(spinbox, ctr, name, defValue);
  }

  Value* Value::Bind(QComboBox& combo, Container& ctr, Identifier name, const Strings::Array& values,
                     StringView defValue)
  {
    return new IndexedValue<StringType>(combo, ctr, name, values, String(defValue));
  }

  Value* Value::Bind(QComboBox& combo, Container& ctr, Identifier name, const std::vector<IntType>& values,
                     IntType defValue)
  {
    return new IndexedValue<IntType>(combo, ctr, name, values, defValue);
  }

  Value* Value::Bind(QLineEdit& edit, Container& ctr, const IntegerTraits& traits)
  {
    return new BigIntegerValue(edit, ctr, traits);
  }

  Value* Value::Bind(QLineEdit& edit, Container& ctr, Identifier name, StringView defValue)
  {
    return new StringValue(edit, ctr, name, defValue);
  }
}  // namespace UI
