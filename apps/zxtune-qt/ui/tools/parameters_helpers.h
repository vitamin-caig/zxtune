/**
 *
 * @file
 *
 * @brief Parameters helpers interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/ui/utils.h"

#include "parameters/container.h"

#include "string_view.h"

#include <QtCore/QObject>

class QAction;
class QAbstractButton;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QSlider;
class QSpinBox;

namespace Parameters
{
  class Value : public QObject
  {
    Q_OBJECT
  protected:
    explicit Value(QObject& parent);

  public:
    virtual void Reset() = 0;
    virtual void Reload() = 0;
  };

  struct IntegerTraits
  {
    Identifier Name = ""_id;
    IntType Default = 0;
    IntType Min = 0;
    IntType Max = 0;

    IntegerTraits() = default;

    IntegerTraits(Identifier name, IntType def, IntType min, IntType max)
      : Name(std::move(name))
      , Default(def)
      , Min(min)
      , Max(max)
    {}
  };

  class BooleanValue : public Value
  {
    Q_OBJECT
  protected:
    explicit BooleanValue(QObject& parent);

  public:
    static Value* Bind(QAction& action, Container& ctr, Identifier name, bool defValue);
    static Value* Bind(QAbstractButton& button, Container& ctr, Identifier name, bool defValue, IntType oneValue = 1);
    static Value* Bind(QGroupBox& box, Container& ctr, Identifier name, bool defValue, IntType oneValue = 1);
  };

  class ExclusiveValue : public Value
  {
    Q_OBJECT
  protected:
    explicit ExclusiveValue(QObject& parent);

  public:
    static Value* Bind(QAbstractButton& button, Container& ctr, Identifier name, StringView value);
  };

  class Integer
  {
  public:
    using Ptr = std::shared_ptr<Integer>;
    virtual ~Integer() = default;

    virtual int Get() const = 0;
    virtual void Set(int val) = 0;
    virtual void Reset() = 0;
  };

  class IntegerValue : public Value
  {
    Q_OBJECT
  protected:
    explicit IntegerValue(QObject& parent);

  public:
    static Value* Bind(QComboBox& combo, Container& ctr, Identifier name, int defValue);
    static Value* Bind(QSlider& slider, Container& ctr, Identifier name, int defValue);
    static Value* Bind(QSpinBox& spinbox, Container& ctr, Identifier name, int defValue);
    static Value* Bind(QComboBox& combo, Integer::Ptr val);

    virtual void Set(int value) = 0;
  };

  class BigIntegerValue : public Value
  {
    Q_OBJECT
  protected:
    explicit BigIntegerValue(QObject& parent);

  public:
    static Value* Bind(QLineEdit& edit, Container& ctr, const IntegerTraits& traits);
  };

  class StringValue : public Value
  {
    Q_OBJECT
  protected:
    explicit StringValue(QObject& parent);

  public:
    static Value* Bind(QLineEdit& edit, Container& ctr, Identifier name, StringView defValue);
  };

  /*
    Keeping l10n-dependent controls in proper state while language change
  */
  class ValueSnapshot
  {
  public:
    explicit ValueSnapshot(Value& val)
      : Val(val)
      , Blocker(*val.parent())
    {}

    ~ValueSnapshot()
    {
      Val.Reload();
    }

  private:
    Value& Val;
    const AutoBlockSignal Blocker;
  };
}  // namespace Parameters
