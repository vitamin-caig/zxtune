/**
 *
 * @file
 *
 * @brief RSS feed parser interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

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
    QUrl DirectLink;
    QUrl AlternateLink;
    QString HtmlContent;
  };

  class Visitor
  {
  public:
    virtual ~Visitor() = default;

    virtual void OnEntry(const Entry& e) = 0;
  };

  bool Parse(const QByteArray& rss, Visitor& visitor);
}  // namespace RSS
