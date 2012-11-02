/*
Abstract:
  Downloads adapter implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#include "downloads.h"
#include "product.h"
#include "apps/zxtune-qt/ui/utils.h"
//common includes
#include <debug_log.h>
//qt includes
#include <QtCore/QRegExp>

namespace
{
  const Debug::Stream Dbg("UpdateCheck");
}

namespace
{
  const QLatin1String CONTENT_FORMAT(
    "([\\w\\d]+)\\s+revision\\s+(\\d+)"
    ".*OpSys-([\\w\\d]+)\\b"
    ".*Platform-([\\w\\d]+)\\b"
    ".*Compiler-([\\w\\d]+)\\b"
    ".*href=\"(http.*)\">"
  )
  ;

  class FeedVisitor : public RSS::Visitor
  {
  public:
    FeedVisitor(const QString& project, Downloads::Visitor& delegate)
      : Project(project)
      , Delegate(delegate)
      , ContentMatch(CONTENT_FORMAT)
    {
      ContentMatch.setMinimal(true);
    }

    virtual void OnUpdated(const QDate& /*date*/)
    {
    }

    virtual void OnEntry(const RSS::Entry& e)
    {
      Dbg("Feed entry '%1%'", FromQString(e.Id));
      if (-1 == ContentMatch.indexIn(e.HtmlContent))
      {
        Dbg("Failed to parse html content");
        return;
      }
      const QString project = ContentMatch.cap(1);
      if (project != Project)
      {
        Dbg("Not supported project");
        return;
      }
      const QString revision = ContentMatch.cap(2);
      const QString os = ContentMatch.cap(3).toLower();
      const QString arch = ContentMatch.cap(4);
      const QString compiler = ContentMatch.cap(5);
      const QString file = ContentMatch.cap(6);
      const QString platform = os == "windows" && compiler == "gcc" ? "mingw" : os;
      Dbg(" download ver %1% %2%_%3% (%4%) %5%", FromQString(revision), FromQString(os), FromQString(arch), FromQString(compiler), FromQString(file));
      const Product::Version vers(revision, e.Updated);
      const Product::Platform plat(arch, platform);
      const Product::Download download(e.AlternateLink, QUrl(file));
      Delegate.OnDownload(vers, plat, download);
    }
  private:
    const QString Project;
    Downloads::Visitor& Delegate;
    QRegExp ContentMatch;
  };
}

namespace Downloads
{
  std::auto_ptr<RSS::Visitor> CreateFeedVisitor(const QString& project, Visitor& delegate)
  {
    return std::auto_ptr<RSS::Visitor>(new FeedVisitor(project, delegate));
  }
}
