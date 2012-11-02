/*
Abstract:
  RSS feed implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "rss.h"
#include "apps/zxtune-qt/ui/utils.h"
//common includes
#include <debug_log.h>
//qt includes
#include <QtCore/QXmlStreamReader>

namespace
{
  const Debug::Stream Dbg("UpdateCheck");
}

namespace RSS
{
  const QLatin1String FEED("feed");
  const QLatin1String UPDATED("updated");
  const QLatin1String ID("id");
  const QLatin1String ENTRY("entry");
  const QLatin1String LINK("link");
  const QLatin1String CONTENT("content");
  const QLatin1String HREF("href");
  const QLatin1String REL("rel");
  const QLatin1String ALTERNATE("alternate");
  const QLatin1String TYPE("type");
  const QLatin1String HTML("html");
}

namespace
{
  RSS::Entry ParseEntry(QXmlStreamReader& xml)
  {
    RSS::Entry res;
    while (xml.readNextStartElement())
    {
      const QStringRef tag = xml.name();
      if (tag == RSS::ID)
      {
        res.Id = xml.readElementText();
      }
      else if (tag == RSS::UPDATED)
      {
        res.Updated = QDate::fromString(xml.readElementText(), Qt::ISODate);
      }
      else if (tag == RSS::LINK)
      {
        const QXmlStreamAttributes attributes = xml.attributes();
        if (attributes.value(RSS::REL) == RSS::ALTERNATE)
        {
          res.AlternateLink = attributes.value(RSS::HREF).toString();
        }
        xml.readElementText();
      }
      else if (tag == RSS::CONTENT)
      {
        const QXmlStreamAttributes attributes = xml.attributes();
        const QString val = xml.readElementText();
        if (attributes.value(RSS::TYPE) == RSS::HTML)
        {
          res.HtmlContent = val;
        }
      }
      else
      {
        xml.skipCurrentElement();
      }
    }
    return res;
  }
}

namespace RSS
{
  bool Parse(const QByteArray& rss, Visitor& visitor)
  {
    Dbg("Parsing rss feed in %1% bytes", rss.size());
    QXmlStreamReader xml(rss);
    if (!xml.readNextStartElement() ||
         xml.name() != RSS::FEED)
    {
      Dbg("Invalid rss content");
      return false;
    }
    while (xml.readNextStartElement())
    {
      const QStringRef tag = xml.name();
      if (tag == RSS::ENTRY)
      {
        const Entry entry = ParseEntry(xml);
        visitor.OnEntry(entry);
      }
      else
      {
        xml.skipCurrentElement();
      }
    }
    return !xml.error();
  }
}
