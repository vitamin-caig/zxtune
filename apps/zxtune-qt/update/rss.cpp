/**
 *
 * @file
 *
 * @brief RSS feed parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/update/rss.h"

#include "apps/zxtune-qt/ui/utils.h"

#include "debug/log.h"

#include <QtCore/QXmlStreamReader>

namespace
{
  const Debug::Stream Dbg("UpdateCheck::RSS");
}

namespace RSS
{
  const QLatin1String FEED("feed");
  const QLatin1String UPDATED("updated");
  const QLatin1String ENTRY("entry");
  const QLatin1String LINK("link");
  const QLatin1String TITLE("title");
  const QLatin1String CONTENT("content");
  const QLatin1String HREF("href");
  const QLatin1String REL("rel");
  const QLatin1String DIRECT("direct");
  const QLatin1String ALTERNATE("alternate");
  const QLatin1String TYPE("type");
  const QLatin1String HTML("html");
}  // namespace RSS

namespace
{
  RSS::Entry ParseEntry(QXmlStreamReader& xml)
  {
    RSS::Entry res;
    while (xml.readNextStartElement())
    {
      const QStringRef tag = xml.name();
      if (tag == RSS::UPDATED)
      {
        res.Updated = QDate::fromString(xml.readElementText(), Qt::ISODate);
      }
      else if (tag == RSS::LINK)
      {
        const QXmlStreamAttributes attributes = xml.attributes();
        const QStringRef rel = attributes.value(RSS::REL);
        const QStringRef ref = attributes.value(RSS::HREF);
        if (rel == RSS::DIRECT)
        {
          res.DirectLink = ref.toString();
        }
        else if (rel == RSS::ALTERNATE)
        {
          res.AlternateLink = ref.toString();
        }
        xml.readElementText();
      }
      else if (tag == RSS::TITLE)
      {
        res.Title = xml.readElementText().trimmed();
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
}  // namespace

namespace RSS
{
  bool Parse(const QByteArray& rss, Visitor& visitor)
  {
    Dbg("Parsing rss feed in {} bytes", rss.size());
    QXmlStreamReader xml(rss);
    if (!xml.readNextStartElement() || xml.name() != RSS::FEED)
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
}  // namespace RSS
