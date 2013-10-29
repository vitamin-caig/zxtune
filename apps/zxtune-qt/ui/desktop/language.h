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

//boost includes
#include <boost/shared_ptr.hpp>
//qt includes
#include <QtCore/QStringList>

namespace UI
{
  class Language
  {
  public:
    typedef boost::shared_ptr<Language> Ptr;
    virtual ~Language() {}

    virtual QStringList GetAvailable() const = 0;
    virtual QString GetSystem() const = 0;
    virtual void Set(const QString& lang) = 0;

    static Ptr Create();
  };
}
