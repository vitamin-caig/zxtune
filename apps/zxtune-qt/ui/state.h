/**
 *
 * @file
 *
 * @brief UI state helper interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "string_view.h"
#include "types.h"

#include <memory>

class QWidget;
namespace UI
{
  class State
  {
  public:
    using Ptr = std::unique_ptr<State>;
    virtual ~State() = default;

    virtual void AddWidget(QWidget& w) = 0;

    // main interface
    virtual void Load() const = 0;
    virtual void Save() const = 0;

    static Ptr Create(StringView category);
    static Ptr Create(QWidget& root);
  };
}  // namespace UI
