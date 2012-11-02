/*
Abstract:
  Update checking implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "check.h"
#include "async_download.h"
#include "downloads.h"
#include "product.h"
#include "apps/zxtune-qt/text/text.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "apps/zxtune-qt/ui/tools/errordialog.h"
//common includes
#include <debug_log.h>
#include <error.h>
//library includes
#include <io/api.h>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressDialog>

#define FILE_TAG 3AFAD500

namespace
{
  const Debug::Stream Dbg("UpdateCheck");
}

namespace
{
  const Error CANCELED(THIS_LINE, String());

  class DownloadCallback : public Async::DownloadCallback
  {
  public:
    explicit DownloadCallback(QProgressDialog& dlg)
      : Progress(dlg)
    {
    }

    virtual void OnProgress(uint_t current)
    {
      if (Progress.wasCanceled())
      {
        Dbg("Cancel download");
        throw CANCELED;
      }
      Progress.setValue(current);
    }

    virtual void OnProgress(uint_t current, const String& /*message*/)
    {
      OnProgress(current);
    }

    virtual void Complete(Binary::Data::Ptr data)
    {
      Dbg("Download complete");
      Result = data;
      Progress.accept();
    }

    virtual void Failed()
    {
      Dbg("Download failed");
      Progress.reject();
    }

    Binary::Data::Ptr GetResult() const
    {
      return Result;
    }
  private:
    QProgressDialog& Progress;
    Binary::Data::Ptr Result;
  };

  Binary::Data::Ptr Download(QWidget& parent, const QUrl& url, const char* progressMsg)
  {
    QProgressDialog dialog(&parent, Qt::Dialog);
    dialog.setLabelText(QApplication::translate("UpdateCheck", progressMsg));
    dialog.setWindowModality(Qt::WindowModal);
    DownloadCallback cb(dialog);
    const Async::Activity::Ptr act = Async::CreateDownloadActivity(url, cb);
    dialog.exec();
    ThrowIfError(act->Wait());
    return cb.GetResult();
  }

  void DownloadAndSave(QWidget& parent, const QUrl& url)
  {
    QString filename = url.toString();
    //do not use UI::SaveFileDialog
    QFileDialog dialog(&parent, QString(), filename, QLatin1String("*"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    dialog.setOption(QFileDialog::HideNameFilterDetails, true);
    if (QDialog::Accepted == dialog.exec())
    {
      filename = dialog.selectedFiles().front();
      QFile file(filename);
      if (!file.open(QIODevice::WriteOnly))
      {
        return;
      }
      const Binary::Data::Ptr content = Download(parent, url, QT_TRANSLATE_NOOP("UpdateCheck", "Downloading file"));
      file.write(static_cast<const char*>(content->Start()), content->Size());
    }
  }

  struct AvailableUpdate
  {
    Product::Download Reference;
    Product::Version Version;
    Product::Platform Platform;

    AvailableUpdate(const Product::Download& ref, const Product::Version& ver, const Product::Platform& plat)
      : Reference(ref)
      , Version(ver)
      , Platform(plat)
    {
    }
  };

  class UpdateState : public Downloads::Visitor
  {
  public:
    UpdateState()
      : CurrentVersion(Product::CurrentBuildVersion())
      , CurrentPlatform(Product::CurrentBuildPlatform())
    {
      Dbg("Current version %1% from %2%", FromQString(CurrentVersion.Index), FromQString(CurrentVersion.ReleaseDate.toString()));
      Dbg("Current platform %1%_%2%", FromQString(CurrentPlatform.OperatingSystem), FromQString(CurrentPlatform.Architecture));
    }

    virtual void OnDownload(const Product::Version& version, const Product::Platform& platform, const Product::Download& download)
    {
      if (CurrentPlatform == platform)
      {
        Dbg("Update for current platform: ver=%1%", FromQString(version.Index));
        //direct updates
        if (Latest.get())
        {
          if (version.IsNewerThan(Latest->Version))
          {
            Dbg(" Latest now");
            Latest->Version = version;
          }
        }
        else
        {
          Dbg(" Latest now");
          Latest.reset(new AvailableUpdate(download, version, platform));
        }
      }
      else if (CurrentPlatform.IsReplaceableWith(platform))
      {
        Dbg("Compatible platform download: ver=%1%, platform=%2% %3%",
          FromQString(version.Index), FromQString(platform.OperatingSystem), FromQString(platform.Architecture));
        //compatible updates
        if (LatestCompatible.get())
        {
          if (LatestCompatible->Version == version)
          {
            if (!LatestCompatible->Platform.IsReplaceableWith(platform))
            {
              Dbg(" Latest compatible now");
              LatestCompatible->Platform = platform;
              LatestCompatible->Reference = download;
            }
          }
          else if (version.IsNewerThan(LatestCompatible->Version))
          {
            Dbg(" Latest compatible now");
            LatestCompatible.reset(new AvailableUpdate(download, version, platform));
          }
        }
        else
        {
          Dbg(" Latest compatible now");
          LatestCompatible.reset(new AvailableUpdate(download, version, platform));
        }
      }
    }

    bool IsLatestVersionUsed() const
    {
      return Latest.get() && (CurrentVersion == Latest->Version || CurrentVersion.IsNewerThan(Latest->Version));
    }

    const AvailableUpdate* GetLatest() const
    {
      return Latest.get();
    }

    const AvailableUpdate* GetLatestCompatible() const
    {
      return LatestCompatible.get();
    }
  private:
    const Product::Version CurrentVersion;
    const Product::Platform CurrentPlatform;
    std::auto_ptr<AvailableUpdate> Latest;
    std::auto_ptr<AvailableUpdate> LatestCompatible;
  };

  void Download(QWidget& parent, const AvailableUpdate& upd, const char* msg)
  {
    const QString text = QApplication::translate("UpdateCheck", msg)
      .arg(upd.Version.Index).arg(upd.Version.ReleaseDate.toString())
      .arg(upd.Reference.Description.toString());
    if (QMessageBox::Save == QMessageBox::question(&parent, QString(),
      text, QMessageBox::Save | QMessageBox::Cancel))
    {
      const QUrl& download = upd.Reference.Package;
      DownloadAndSave(parent, download);
    }
  }
}

namespace Update
{
  bool IsCheckingAvailable()
  {
    try
    {
      const IO::Identifier::Ptr id = IO::ResolveUri(Text::DOWNLOADS_FEED_URL);
      return id;
    }
    catch (const Error&)
    {
      return false;
    }
  }

  void Check(QWidget& parent)
  {
    try
    {
      const QUrl feedUrl(Text::DOWNLOADS_FEED_URL);
      const Binary::Data::Ptr feedData = Download(parent, feedUrl,
        QT_TRANSLATE_NOOP("UpdateCheck", "Getting list of currently available downloads"));
      UpdateState state;
      const std::auto_ptr<RSS::Visitor> rss = Downloads::CreateFeedVisitor("zxtune", state);
      RSS::Parse(QByteArray(static_cast<const char*>(feedData->Start()), feedData->Size()), *rss);
      if (state.IsLatestVersionUsed())
      {
        QMessageBox::information(&parent, QString(),
          QApplication::translate("UpdateCheck", QT_TRANSLATE_NOOP("UpdateCheck", "The current version is the latest one")));
      }
      else if (const AvailableUpdate* avail = state.GetLatest())
      {
        Download(parent, *avail, QT_TRANSLATE_NOOP("UpdateCheck", "New version %1 from %2 found.<br/><a href=\"%3\">Download it manually</a>"));
      }
      else if(const AvailableUpdate* compatible = state.GetLatestCompatible())
      {
        Download(parent, *compatible, QT_TRANSLATE_NOOP("UpdateCheck", "New compatible version %1 from %2 found.<br/><a href=\"%3\">Download it manually</a>"));
      }
    }
    catch (const Error& e)
    {
      if (e.GetSuberror().GetLocation() != CANCELED.GetLocation())
      {
        ShowErrorMessage(QApplication::translate("UpdateCheck", QT_TRANSLATE_NOOP("UpdateCheck", "Failed to check updates")), e);
      }
    }
  }
};
