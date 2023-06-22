/**
 *
 * @file
 *
 * @brief Downloads visitor adapter implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "downloads.h"
#include "apps/zxtune-qt/ui/utils.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
// std includes
#include <utility>
// qt includes
#include <QtCore/QRegularExpression>

namespace
{
  const Debug::Stream Dbg("UpdateCheck::Downloads");
}

namespace
{
  const QLatin1String CONTENT_FORMAT(R"(([\w\d]+)\s+(revision|version)\s+(\d+))");

  enum
  {
    CONTENT_PROJECT = 1,
    CONTENT_REVISION = 3,
  };

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
  const QLatin1String PLATFORM_ARMHF("Platform-armhf");
  const QLatin1String PLATFORM_MIPSEL("Platform-mipsel");

  const QLatin1String TYPE_ZIP(".zip");
  const QLatin1String TYPE_TARGZ(".tar.gz");
  const QLatin1String TYPE_TARXZ(".tar.xz");
  const QLatin1String TYPE_DEB(".deb");
  const QLatin1String TYPE_RPM(".rpm");

  class UpdateDownload : public Product::Update
  {
  public:
    UpdateDownload(RSS::Entry entry, QString version)
      : Entry(std::move(entry))
      , VersionValue(std::move(version))
    {}

    Product::Release::PlatformTag Platform() const override
    {
      if (Entry.HtmlContent.contains(OPSYS_WINDOWS))
      {
        return Entry.HtmlContent.contains(COMPILER_GCC) ? Product::Release::MINGW : Product::Release::WINDOWS;
      }
      else if (Entry.HtmlContent.contains(OPSYS_LINUX) || Entry.HtmlContent.contains(OPSYS_ARCHLINUX)
               || Entry.HtmlContent.contains(OPSYS_UBUNTU) || Entry.HtmlContent.contains(OPSYS_REDHAT))
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

    Product::Release::ArchitectureTag Architecture() const override
    {
      // check x86_64 first
      if (Entry.HtmlContent.contains(PLATFORM_X86_64))
      {
        return Product::Release::X86_64;
      }
      else if (Entry.HtmlContent.contains(PLATFORM_X86))
      {
        return Product::Release::X86;
      }
      // check armhf first
      else if (Entry.HtmlContent.contains(PLATFORM_ARMHF))
      {
        return Product::Release::ARMHF;
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

    QString Version() const override
    {
      return VersionValue;
    }

    QDate Date() const override
    {
      return Entry.Updated;
    }

    Product::Update::PackagingTag Packaging() const override
    {
      const QString file = Entry.DirectLink.toString();
      if (file.endsWith(TYPE_ZIP))
      {
        return Product::Update::ZIP;
      }
      else if (file.endsWith(TYPE_TARGZ))
      {
        return Product::Update::TARGZ;
      }
      else if (file.endsWith(TYPE_TARXZ))
      {
        return Product::Update::TARXZ;
      }
      else if (file.endsWith(TYPE_DEB))
      {
        return Product::Update::DEB;
      }
      else if (file.endsWith(TYPE_RPM))
      {
        return Product::Update::RPM;
      }
      else
      {
        return Product::Update::UNKNOWN_PACKAGING;
      }
    }

    QString Title() const override
    {
      return Entry.Title;
    }

    QUrl Description() const override
    {
      return Entry.AlternateLink;
    }

    QUrl Package() const override
    {
      return Entry.DirectLink;
    }

  private:
    const RSS::Entry Entry;
    const QString VersionValue;
  };

  class FeedVisitor : public RSS::Visitor
  {
  public:
    FeedVisitor(QString project, Downloads::Visitor& delegate)
      : Project(std::move(project))
      , Delegate(delegate)
      , ContentMatch(CONTENT_FORMAT)
    {}

    void OnEntry(const RSS::Entry& e) override
    {
      Dbg("Feed entry '{}'", FromQString(e.Title));
      const auto match = ContentMatch.match(e.HtmlContent);
      if (!match.hasMatch())
      {
        Dbg("Failed to parse html content");
        return;
      }
      const auto project = match.capturedView(CONTENT_PROJECT);
      if (project != Project)
      {
        Dbg("Not supported project");
        return;
      }
      const auto revision = match.captured(CONTENT_REVISION);
      const Product::Update::Ptr update = MakePtr<UpdateDownload>(e, revision);
      Delegate.OnDownload(update);
    }

  private:
    const QString Project;
    Downloads::Visitor& Delegate;
    QRegularExpression ContentMatch;
  };
}  // namespace

namespace Downloads
{
  std::unique_ptr<RSS::Visitor> CreateFeedVisitor(const QString& project, Visitor& delegate)
  {
    return std::unique_ptr<RSS::Visitor>(new FeedVisitor(project, delegate));
  }
}  // namespace Downloads
