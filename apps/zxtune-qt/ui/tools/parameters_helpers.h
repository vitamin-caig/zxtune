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
#include "parameters/value.h"
#include "strings/array.h"

#include "string_view.h"

#include <QtCore/QObject>
#include <QtGui/QIntValidator>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>

#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

class QAction;
class QAbstractButton;
class QComboBox;
class QGroupBox;

namespace UI
{
  //! @brief Numeric parameters are always bound to int-operand widgets (QSpinBox/QSlider/QIntValidator)
  template<class P>
  inline constexpr bool RangeFitsInt = static_cast<Parameters::IntType>(P::Min) >= std::numeric_limits<int>::min()
                                       && static_cast<Parameters::IntType>(P::Max) <= std::numeric_limits<int>::max();

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
    // Integer-valued widgets; an unset default means "no override" (properties view)
    static Value* Bind(QSlider& slider, Parameters::Container& ctr, Parameters::Identifier name,
                       std::optional<Parameters::IntType> defValue);
    static Value* Bind(QSpinBox& spinbox, Parameters::Container& ctr, Parameters::Identifier name,
                       std::optional<Parameters::IntType> defValue);
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, Parameters::Identifier name,
                       std::optional<Parameters::IntType> defValue);
    static Value* Bind(QLineEdit& edit, Parameters::Container& ctr, Parameters::Identifier name,
                       std::optional<Parameters::IntType> defValue);
    static Value* Bind(QLineEdit& edit, Parameters::Container& ctr, Parameters::Identifier name,
                       std::optional<Parameters::StringType> defValue);
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, Parameters::Identifier name,
                       const Strings::Array& values, StringView defValue);
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, Parameters::Identifier name,
                       const std::vector<Parameters::IntType>& values, Parameters::IntType defValue);

    template<Parameters::Defaulted P>
    static Value* Bind(QLineEdit& edit, Parameters::Container& ctr)
    {
      return BindText<P>(edit, ctr, std::optional<typename P::Type>(P::Default));
    }

    template<Parameters::Defaulted P>
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr)
    {
      return Bind(combo, ctr, P::Key, std::optional<Parameters::IntType>(static_cast<Parameters::IntType>(P::Default)));
    }

    template<Parameters::Defaulted P>
    static Value* Bind(QSlider& slider, Parameters::Container& ctr)
    {
      static_assert(RangeFitsInt<P>, "Parameter range must fit into int");
      slider.setRange(static_cast<int>(P::Min), static_cast<int>(P::Max));
      return Bind(slider, ctr, P::Key,
                  std::optional<Parameters::IntType>(static_cast<Parameters::IntType>(P::Default)));
    }

    template<Parameters::Defaulted P>
    static Value* Bind(QSpinBox& spinbox, Parameters::Container& ctr)
    {
      static_assert(RangeFitsInt<P>, "Parameter range must fit into int");
      spinbox.setRange(static_cast<int>(P::Min), static_cast<int>(P::Max));
      return Bind(spinbox, ctr, P::Key,
                  std::optional<Parameters::IntType>(static_cast<Parameters::IntType>(P::Default)));
    }

    template<Parameters::Defaulted P>
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, const std::vector<Parameters::IntType>& values)
    {
      return Bind(combo, ctr, P::Key, values, static_cast<Parameters::IntType>(P::Default));
    }

    template<Parameters::Defaulted P>
    static Value* Bind(QAction& action, Parameters::Container& ctr)
    {
      return Bind(action, ctr, P::Key, static_cast<bool>(P::Default));
    }

    template<Parameters::Defaulted P>
    static Value* Bind(QAbstractButton& button, Parameters::Container& ctr, Parameters::IntType trueValue = 1)
    {
      return Bind(button, ctr, P::Key, static_cast<bool>(P::Default), trueValue);
    }

    template<Parameters::Defaulted P>
    static Value* Bind(QGroupBox& box, Parameters::Container& ctr, Parameters::IntType trueValue = 1)
    {
      return Bind(box, ctr, P::Key, static_cast<bool>(P::Default), trueValue);
    }

    template<Parameters::Parameter P>
    static Value* BindExplicit(QLineEdit& edit, Parameters::Container& ctr)
    {
      return BindText<P>(edit, ctr, std::nullopt);
    }

    template<Parameters::Parameter P>
    static Value* BindExplicit(QComboBox& combo, Parameters::Container& ctr)
    {
      return Bind(combo, ctr, P::Key, std::optional<Parameters::IntType>{});
    }

    // Explicit (no-override) bindings for dynamic property names
    static Value* BindExplicit(QLineEdit& edit, Parameters::Container& ctr, Parameters::Identifier name)
    {
      return Bind(edit, ctr, name, std::optional<Parameters::StringType>{});
    }

    static Value* BindExplicit(QComboBox& combo, Parameters::Container& ctr, Parameters::Identifier name)
    {
      return Bind(combo, ctr, name, std::optional<Parameters::IntType>{});
    }

    // Free-text combo box bound to a string parameter
    template<Parameters::Parameter P>
    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, StringView defValue)
    {
      return Bind(combo, ctr, P::Key, defValue);
    }

    static Value* Bind(QComboBox& combo, Parameters::Container& ctr, Parameters::Identifier name, StringView defValue);

  private:
    //! @brief Numeric or textual field; an unset default means "no override"
    template<Parameters::Parameter P>
    static Value* BindText(QLineEdit& edit, Parameters::Container& ctr, std::optional<typename P::Type> def)
    {
      if constexpr (std::is_same_v<typename P::Type, Parameters::StringType>)
      {
        return Bind(edit, ctr, P::Key, std::move(def));
      }
      else
      {
        static_assert(RangeFitsInt<P>, "Parameter range must fit into int");
        edit.setValidator(new QIntValidator(static_cast<int>(P::Min), static_cast<int>(P::Max), &edit));
        const auto intDef = def ? std::optional<Parameters::IntType>(static_cast<Parameters::IntType>(*def))
                                : std::optional<Parameters::IntType>{};
        return Bind(edit, ctr, P::Key, intDef);
      }
    }
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
