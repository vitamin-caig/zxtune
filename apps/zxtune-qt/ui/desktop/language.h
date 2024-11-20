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

#include <QtCore/QStringList>

#include <memory>

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
