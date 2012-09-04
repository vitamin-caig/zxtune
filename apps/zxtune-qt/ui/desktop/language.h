/*
Abstract:
  UI i18n support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_LANGUAGE_H_DEFINED
#define ZXTUNE_QT_UI_LANGUAGE_H_DEFINED

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
    virtual void Set(const QString& lang) = 0;

    static Ptr Create();
  };
}

#endif //ZXTUNE_QT_UI_LANGUAGE_H_DEFINED
