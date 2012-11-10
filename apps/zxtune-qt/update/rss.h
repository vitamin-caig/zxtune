/*
Abstract:
  RSS feed interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UPDATE_RSS_H_DEFINED
#define ZXTUNE_QT_UPDATE_RSS_H_DEFINED

//qt includes
#include <QtCore/QByteArray>
#include <QtCore/QDate>
#include <QtCore/QString>
#include <QtCore/QUrl>

namespace RSS
{
  struct Entry
  {
    QDate Updated;
    QString Title;
    QUrl AlternateLink;
    QString HtmlContent;
  };

  class Visitor
  {
  public:
    virtual ~Visitor() {}

    virtual void OnEntry(const Entry& e) = 0;
  };

  bool Parse(const QByteArray& rss, Visitor& visitor);
}

#endif //ZXTUNE_QT_UPDATE_RSS_H_DEFINED
