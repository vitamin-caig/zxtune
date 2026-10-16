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
#include "strings/array.h"

#include "string_view.h"

#include <QtCore/QObject>

#include <vector>

class QAction;
class QAbstractButton;
class QComboBox;
class QGroupBox;
class QLineEdit;
class QSlider;
class QSpinBox;

namespace UI
{
  struct IntegerTraits
  {
    const Parameters::Identifier Name;
    const Parameters::IntType Default;
    const Parameters::IntType Min;
    const Parameters::IntType Max;
  };

  class Value : public QObject
  {
  protected:
    explicit Value(QObject& parent);

  public:
    virtual void Reset() = 0;
    virtual void Reload() = 0;

    static Value* Bind(QAction& action, Parameters::Container& ctr, Parameters::Identifier name, bool defValue);
    static Value* Bind(QAbstractButton& button, Parameters::Container& ctr, Parameters::Identifier name, bool defValue,
                       Parameters::IntType trueValue = 1);
    static Value* Bind(QGroupBox& box, Parameters::Container& ctr, Parameters::Identifier name, bool defValue,
                       Parameters::IntType trueValue = 1);
    // Stores trueValue when checked; unchecking leaves the value untouched
    static Value* BindExclusive(QAbstractButton& button, Parameters::Container& ctr, Parameters::Identifier name,
                                StringView trueValue);
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, Parameters::Identifier name, int defValue);
    static Value* Bind(QSlider& slider, Parameters::Container& ctr, Parameters::Identifier name, int defValue);
    static Value* Bind(QSpinBox& spinbox, Parameters::Container& ctr, Parameters::Identifier name, int defValue);
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, Parameters::Identifier name,
                       const Strings::Array& values, StringView defValue);
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, Parameters::Identifier name,
                       const std::vector<Parameters::IntType>& values, Parameters::IntType defValue);
    static Value* Bind(QLineEdit& edit, Parameters::Container& ctr, const IntegerTraits& traits);
    static Value* Bind(QLineEdit& edit, Parameters::Container& ctr, Parameters::Identifier name, StringView defValue);
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
}  // namespace UI
