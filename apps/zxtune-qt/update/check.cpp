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
#include "downloads.h"
#include "product.h"
#include "apps/version/fields.h"
#include "apps/zxtune-qt/text/text.h"
#include "apps/zxtune-qt/ui/utils.h"
//common includes
#include <error.h>
#include <parameters.h>
#include <progress_callback.h>
//library includes
#include <debug/log.h>
#include <io/api.h>
#include <io/providers_parameters.h>
//std includes
#include <ctime>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressDialog>

namespace
{
  const Debug::Stream Dbg("UpdateCheck");
}

namespace
{
  class IOParameters : public Parameters::Accessor
  {
  public:
    explicit IOParameters(const String& userAgent)
      : UserAgent(userAgent)
    {
    }

    IOParameters()
    {
    }

    virtual uint_t Version() const
    {
      return 1;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      if (name == Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING)
      {
        val = 1;
        return true;
      }
      else
      {
        return false;
      }
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      if (!UserAgent.empty() && name == Parameters::ZXTune::IO::Providers::Network::Http::USERAGENT)
      {
        val = UserAgent;
        return true;
      }
      else
      {
        return false;
      }
    }

    virtual bool FindValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const
    {
      return false;
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      visitor.SetValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, 1);
      if (!UserAgent.empty())
      {
        visitor.SetValue(Parameters::ZXTune::IO::Providers::Network::Http::USERAGENT, UserAgent);
      }
    }
  private:
    const String UserAgent;
  };

  class Canceled {};

  class DownloadCallback : public Log::ProgressCallback
  {
  public:
    explicit DownloadCallback(QProgressDialog& dlg)
      : Progress(dlg)
    {
      Progress.setValue(0);
    }

    virtual void OnProgress(uint_t current)
    {
      if (Progress.wasCanceled())
      {
        Dbg("Cancel download");
        throw Canceled();
      }
      Progress.setValue(current);
    }

    virtual void OnProgress(uint_t current, const String& /*message*/)
    {
      OnProgress(current);
    }
  private:
    QProgressDialog& Progress;
  };

  String GetUserAgent()
  {
    const std::auto_ptr<Strings::FieldsSource> fields = CreateVersionFieldsSource();
    return Strings::Template::Instantiate(Text::HTTP_USERAGENT, *fields);
  }

  Binary::Data::Ptr Download(const QUrl& url, Log::ProgressCallback& cb)
  {
    const String path = FromQString(url.toString());
    const IOParameters params(GetUserAgent());
    return IO::OpenData(path, params, cb);
  }

  Binary::OutputStream::Ptr CreateStream(const QString& filename)
  {
    const String path = FromQString(filename);
    const IOParameters params;
    return IO::CreateStream(path, params, Log::ProgressCallback::Stub());
  }

  QDate VersionToDate(const QString& str, const QDate& fallback)
  {
    const QDate asDate = QDate::fromString(str, "yyyyMMdd");
    return asDate.isValid()
      ? asDate
      : fallback
    ;
  }

  unsigned short VersionToRevision(const QString& str)
  {
    static const QLatin1String REV_FORMAT("(\?:r(\?:ev)\?)?(\\d{4,5})");
    QRegExp expr(REV_FORMAT);
    if (expr.exactMatch(str))
    {
      bool ok = false;
      const unsigned short val = expr.cap(1).toShort(&ok);
      if (ok)
      {
        return val;
      }
    }
    return 0;
  }

  class Version
  {
  public:
    Version()
      : AsDate()
      , AsRev()
    {
    }

    explicit Version(const QString& ver, const QDate& date)
      : AsDate(VersionToDate(ver, date))
      , AsRev(VersionToRevision(ver))
    {
      Dbg("Version(%1%, %2%) = (date=%3%, rev=%4%)", FromQString(ver), FromQString(date.toString()), FromQString(AsDate.toString()), AsRev);
    }

    bool IsMoreRecentThan(const Version& rh) const
    {
      if (AsRev && rh.AsRev)
      {
        return AsRev > rh.AsRev;
      }
      else if (AsDate.isValid() && rh.AsDate.isValid())
      {
        return AsDate > rh.AsDate;
      }
      else
      {
        const bool thisValid = AsRev || AsDate.isValid();
        const bool rhValid = rh.AsRev || rh.AsDate.isValid();
        if (thisValid && !rhValid)
        {
          //prefer much more valid
          return true;
        }
        else
        {
          return false;
        }
      }
    }

    bool EqualsTo(const Version& rh) const
    {
      if (AsRev && rh.AsRev)
      {
        return AsRev == rh.AsRev;
      }
      else
      {
        return AsDate == rh.AsDate;
      }
    }
  private:
    QDate AsDate;
    unsigned short AsRev;
  };

  class UpdateState : public Downloads::Visitor
  {
  public:
    UpdateState()
      : CurVersion(Product::ThisRelease().Version(), Product::ThisRelease().Date())
      , CurTypes(Product::SupportedUpdateTypes())
      , UpdateRank(~std::size_t(0))
    {
      Dbg("Supported update types: %1% items", CurTypes.size());
      for (std::vector<Product::Update::TypeTag>::const_iterator it = CurTypes.begin(), lim = CurTypes.end(); it != lim; ++it)
      {
        Dbg(" %1%", *it);
      }
    }

    virtual void OnDownload(Product::Update::Ptr update)
    {
      const Product::Update::TypeTag type = Product::GetUpdateType(update->Platform(), update->Architecture(), update->Packaging());
      Dbg("Update %1%, type %2%", FromQString(update->Title()), type);
      const std::vector<Product::Update::TypeTag>::const_iterator it = std::find(CurTypes.begin(), CurTypes.end(), type);
      if (CurTypes.end() == it)
      {
        Dbg(" unsupported");
        return;
      }
      const Version version(update->Version(), update->Date());
      if (version.IsMoreRecentThan(CurVersion))
      {
        const std::size_t rank = std::distance(CurTypes.begin(), it);
        if (!Update
         || version.IsMoreRecentThan(UpdateVersion)
         || (version.EqualsTo(UpdateVersion) && rank < UpdateRank))
        {
          Update = update;
          UpdateRank = rank;
          UpdateVersion = version;
          Dbg(" using: rank=%1%", rank);
        }
        else
        {
          Dbg(" ignored");
        }
      }
      else
      {
        Dbg(" outdated");
      }
    }

    Product::Update::Ptr GetUpdate()
    {
      return Update;
    }
  private:
    const Version CurVersion;
    const std::vector<Product::Update::TypeTag> CurTypes;
    Product::Update::Ptr Update;
    std::size_t UpdateRank;
    Version UpdateVersion;
  };

  class FileTransaction
  {
  public:
    explicit FileTransaction(const QString& name)
      : Info(name)
    {
    }

    ~FileTransaction()
    {
      Rollback();
    }

    Binary::OutputStream::Ptr Begin()
    {
      Rollback();
      return Object = CreateStream(Info.absoluteFilePath());
    }

    void Commit()
    {
      Object = Binary::OutputStream::Ptr();
    }

    void Rollback()
    {
      if (Object)
      {
        Object = Binary::OutputStream::Ptr();
        const bool res = Info.absoluteDir().remove(Info.fileName());
        Dbg("Remove failed saving: %1%", res);
      }
    }
  private:
    const QFileInfo Info;
    Binary::OutputStream::Ptr Object;
  };

  class UpdateCheckOperation : public Update::CheckOperation
  {
  public:
    explicit UpdateCheckOperation(QWidget& parent)
      : Parent(parent)
      , LastLandingPageVisit()
    {
      setParent(&parent);
    }

    virtual void Execute()
    {
      const int_t LANDING_PERIOD = 86400;
      try
      {
        VisitLandingPage(LANDING_PERIOD);
        if (const Product::Update::Ptr update = GetAvailableUpdate())
        {
          if (QMessageBox::Save == ShowUpdateDialog(*update))
          {
            ApplyUpdate(*update);
          }
        }
        else
        {
          QMessageBox::information(&Parent, QString(), Update::CheckOperation::tr("No new updates found"));
        }
      }
      catch (const Canceled&)
      {
      }
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }
  private:
    void VisitLandingPage(int_t period) const
    {
      const std::time_t nowTime = std::time(0);
      if (LastLandingPageVisit + period > nowTime)
      {
        return;
      }
      const QUrl landingUrl(Text::HOMEPAGE_URL);
      DownloadWithProgress(landingUrl, Update::CheckOperation::tr("Checking connection..."));
      LastLandingPageVisit = nowTime;
    }

    Product::Update::Ptr GetAvailableUpdate() const
    {
      const QUrl feedUrl(Text::DOWNLOADS_FEED_URL);
      const Binary::Data::Ptr feedData = DownloadWithProgress(feedUrl, Update::CheckOperation::tr("Getting list of available updates"));
      UpdateState state;
      const std::auto_ptr<RSS::Visitor> rss = Downloads::CreateFeedVisitor(Text::DOWNLOADS_PROJECT_NAME, state);
      RSS::Parse(QByteArray(static_cast<const char*>(feedData->Start()), feedData->Size()), *rss);
      return state.GetUpdate();
    }

    Binary::Data::Ptr DownloadWithProgress(const QUrl& url, const QString& text) const
    {
      QProgressDialog dialog(&Parent, Qt::Dialog);
      dialog.setMinimumDuration(1);
      dialog.setLabelText(text);
      dialog.setWindowModality(Qt::WindowModal);
      DownloadCallback cb(dialog);
      return Download(url, cb);
    }

    QMessageBox::StandardButton ShowUpdateDialog(const Product::Update& update) const
    {
      const QString title = Update::CheckOperation::tr("Available version %1").arg(update.Version());
      QStringList msg;
      msg.append(update.Title());
      if (const int ageInDays = update.Date().daysTo(QDate::currentDate()))
      {
        msg.append(Update::CheckOperation::tr("%1 (%n day(s) ago)", 0, ageInDays).arg(update.Date().toString(Qt::DefaultLocaleLongDate)));
      }
      msg.append(Update::CheckOperation::tr("<a href=\"%1\">Download manually</a>").arg(update.Description().toString()));
      return QMessageBox::question(&Parent, title, msg.join("<br/>"), QMessageBox::Save | QMessageBox::Cancel);
    }

    void ApplyUpdate(const Product::Update& update) const
    {
      const QUrl packageUrl = update.Package();
      //do not use UI::SaveFileDialog
      QFileDialog dialog(&Parent, QString(), packageUrl.toString(), QLatin1String("*"));
      dialog.setOption(QFileDialog::DontUseNativeDialog, true);
      dialog.setOption(QFileDialog::HideNameFilterDetails, true);
      if (QDialog::Accepted == dialog.exec())
      {
        DownloadAndSave(packageUrl, dialog.selectedFiles().front());
      }
    }

    void DownloadAndSave(const QUrl& url, const QString& filename) const
    {
      FileTransaction transaction(filename);
      const Binary::OutputStream::Ptr target = transaction.Begin();
      const Binary::Data::Ptr content = DownloadWithProgress(url, Update::CheckOperation::tr("Downloading file"));
      target->ApplyData(*content);
      target->Flush();
      transaction.Commit();
    }
  private:
    QWidget& Parent;
    mutable std::time_t LastLandingPageVisit;
  };
}

namespace Update
{
  CheckOperation* CheckOperation::Create(QWidget& parent)
  {
    try
    {
      if (IO::ResolveUri(Text::DOWNLOADS_FEED_URL))
      {
        std::auto_ptr<CheckOperation> res(new UpdateCheckOperation(parent));
        res->setParent(&parent);
        return res.release();
      }
    }
    catch (const Error&)
    {
    }
    return 0;
  }
};
