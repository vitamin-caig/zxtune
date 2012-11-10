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
#include "apps/zxtune-qt/ui/utils.h"
//common includes
#include <debug_log.h>
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QRegExp>

namespace
{
  const Debug::Stream Dbg("UpdateCheck::Downloads");
}

namespace
{
  const QLatin1String CONTENT_FORMAT(
    "([\\w\\d]+)\\s+revision\\s+(\\d+)"
    ".*href=\"(http.*)\">"
  )
  ;

  const QLatin1String OPSYS_WINDOWS("OpSys-Windows");
  const QLatin1String OPSYS_LINUX("OpSys-Linux");
  const QLatin1String OPSYS_DINGUX("OpSys-Dingux");
  const QLatin1String OPSYS_ARCHLINUX("OpSys-Archlinux");
  const QLatin1String OPSYS_UBUNTU("OpSys-Ubuntu");
  const QLatin1String OPSYS_REDHAT("OpSys-Redhat");
  const QLatin1String COMPILER_GCC("Compiler-gcc");

  const QLatin1String PLATFORM_X86("Platform-x86");
  const QLatin1String PLATFORM_X86_64("Platform-x86_64");
  const QLatin1String PLATFORM_ARM("Platform-arm");
  const QLatin1String PLATFORM_MIPSEL("Platform-mipsel");

  const QLatin1String TYPE_ZIP(".zip");
  const QLatin1String TYPE_TARGZ(".tar.gz");
  const QLatin1String TYPE_TARXZ(".tar.xz");
  const QLatin1String TYPE_DEB(".deb");
  const QLatin1String TYPE_RPM(".rpm");

  class UpdateDownload : public Product::Update
  {
  public:
    UpdateDownload(const RSS::Entry& entry, const QString& version, const QString& file)
      : Entry(entry)
      , VersionValue(version)
      , File(file)
    {
    }

    virtual Product::Release::PlatformTag Platform() const
    {
      if (Entry.HtmlContent.contains(OPSYS_WINDOWS))
      {
        return Entry.HtmlContent.contains(COMPILER_GCC)
          ? Product::Release::MINGW
          : Product::Release::WINDOWS
        ;
      }
      else if (Entry.HtmlContent.contains(OPSYS_LINUX) ||
               Entry.HtmlContent.contains(OPSYS_ARCHLINUX) ||
               Entry.HtmlContent.contains(OPSYS_UBUNTU) ||
               Entry.HtmlContent.contains(OPSYS_REDHAT))
      {
        return Product::Release::LINUX;
      }
      else if (Entry.HtmlContent.contains(OPSYS_DINGUX))
      {
        return Product::Release::DINGUX;
      }
      else
      {
        return Product::Release::UNKNOWN_PLATFORM;
      }
    }

    virtual Product::Release::ArchitectureTag Architecture() const
    {
      //check x86_64 first
      if (Entry.HtmlContent.contains(PLATFORM_X86_64))
      {
        return Product::Release::X86_64;
      }
      else if (Entry.HtmlContent.contains(PLATFORM_X86))
      {
        return Product::Release::X86;
      }
      else if (Entry.HtmlContent.contains(PLATFORM_ARM))
      {
        return Product::Release::ARM;
      }
      else if (Entry.HtmlContent.contains(PLATFORM_MIPSEL))
      {
        return Product::Release::MIPSEL;
      }
      else
      {
        return Product::Release::UNKNOWN_ARCHITECTURE;
      }
    }

    virtual QString Version() const
    {
      return VersionValue;
    }

    virtual QDate Date() const
    {
      return Entry.Updated;
    }

    virtual Product::Update::PackagingTag Packaging() const
    {
      if (File.endsWith(TYPE_ZIP))
      {
        return Product::Update::ZIP;
      }
      else if (File.endsWith(TYPE_TARGZ))
      {
        return Product::Update::TARGZ;
      }
      else if (File.endsWith(TYPE_TARXZ))
      {
        return Product::Update::TARXZ;
      }
      else if (File.endsWith(TYPE_DEB))
      {
        return Product::Update::DEB;
      }
      else if (File.endsWith(TYPE_RPM))
      {
        return Product::Update::RPM;
      }
      else
      {
        return Product::Update::UNKNOWN_PACKAGING;
      }
    }

    virtual QString Title() const
    {
      return Entry.Title;
    }

    virtual QUrl Description() const
    {
      return Entry.AlternateLink;
    }

    virtual QUrl Package() const
    {
      return File;
    }
  private:
    const RSS::Entry Entry;
    const QString VersionValue;
    const QString File;
  };

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

    virtual void OnEntry(const RSS::Entry& e)
    {
      Dbg("Feed entry '%1%'", FromQString(e.Title));
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
      const QString file = ContentMatch.cap(3);
      const Product::Update::Ptr update = boost::make_shared<UpdateDownload>(e, revision, file);
      Delegate.OnDownload(update);
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
