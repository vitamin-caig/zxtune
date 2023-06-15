/**
 *
 * @file
 *
 * @brief i18n support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <memory>
// qt includes
#include <QtCore/QStringList>

namespace UI
{
  class Language
  {
  public:
    using Ptr = std::shared_ptr<Language>;
    virtual ~Language() = default;

    virtual QStringList GetAvailable() const = 0;
    virtual QString GetSystem() const = 0;
    virtual void Set(const QString& lang) = 0;

    static Ptr Create();
  };
}  // namespace UI
