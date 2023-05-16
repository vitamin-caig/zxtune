/**
 *
 * @file
 *
 * @brief Update checking implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "check.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "apps/zxtune-qt/urls.h"
#include "downloads.h"
#include "parameters.h"
#include "product.h"
// common includes
#include <error.h>
#include <progress_callback.h>
// library includes
#include <debug/log.h>
#include <io/api.h>
#include <io/providers_parameters.h>
#include <platform/version/fields.h>
// std includes
#include <ctime>
#include <utility>
// qt includes
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressDialog>

namespace
{
  const Debug::Stream Dbg("UpdateCheck");
}

namespace
{
  class IOParameters : public Parameters::Accessor
  {
  public:
    explicit IOParameters(String userAgent)
      : UserAgent(std::move(userAgent))
    {}

    IOParameters() {}

    uint_t Version() const override
    {
      return 1;
    }

    bool FindValue(Parameters::Identifier name, Parameters::IntType& val) const override
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

    bool FindValue(Parameters::Identifier name, Parameters::StringType& val) const override
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

    bool FindValue(Parameters::Identifier /*name*/, Parameters::DataType& /*val*/) const override
    {
      return false;
    }

    void Process(Parameters::Visitor& visitor) const override
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

  class Canceled
  {};

  class DialogProgressCallback : public Log::ProgressCallback
  {
  public:
    explicit DialogProgressCallback(QWidget& parent, const QString& text)
      : Progress(&parent, Qt::Dialog)
    {
      Progress.setMinimumDuration(1);
      Progress.setLabelText(text);
      Progress.setWindowModality(Qt::WindowModal);
      Progress.setValue(0);
    }

    void OnProgress(uint_t current) override
    {
      if (Progress.wasCanceled())
      {
        Dbg("Cancel download");
        throw Canceled();
      }
      Progress.setValue(current);
    }

    void OnProgress(uint_t current, StringView /*message*/) override
    {
      OnProgress(current);
    }

  private:
    QProgressDialog Progress;
  };

  String GetUserAgent()
  {
    const std::unique_ptr<Strings::FieldsSource> fields = Platform::Version::CreateVersionFieldsSource();
    return Strings::Template::Instantiate("[Program]/[Version] ([Platform]-[Arch])", *fields);
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
    return asDate.isValid() ? asDate : fallback;
  }

  unsigned short VersionToRevision(const QString& str)
  {
    static const QLatin1String REV_FORMAT("(\?:r(\?:ev)\?)\?(\\d{4,5}).*");
    QRegExp expr(REV_FORMAT);
    if (expr.exactMatch(str))
    {
      const QString rev = expr.cap(1);
      bool ok = false;
      const unsigned short val = rev.toShort(&ok);
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
    {}

    explicit Version(const QString& ver, const QDate& date)
      : AsDate(VersionToDate(ver, date))
      , AsRev(VersionToRevision(ver))
    {
      Dbg("Version({}, {}) = (date={}, rev={})", FromQString(ver), FromQString(date.toString()),
          FromQString(AsDate.toString()), AsRev);
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
          // prefer much more valid
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
    unsigned short AsRev = 0;
  };

  class UpdateState : public Downloads::Visitor
  {
  public:
    UpdateState()
      : CurVersion(Product::ThisRelease().Version(), Product::ThisRelease().Date())
      , CurTypes(Product::SupportedUpdateTypes())
      , UpdateRank(~std::size_t(0))
    {
      Dbg("Supported update types: {} items", CurTypes.size());
      for (auto type : CurTypes)
      {
        Dbg(" {}", int(type));
      }
    }

    void OnDownload(Product::Update::Ptr update) override
    {
      const Product::Update::TypeTag type =
          Product::GetUpdateType(update->Platform(), update->Architecture(), update->Packaging());
      Dbg("Update {}, type {}", FromQString(update->Title()), int(type));
      const auto it = std::find(CurTypes.begin(), CurTypes.end(), type);
      if (CurTypes.end() == it)
      {
        Dbg(" unsupported");
        return;
      }
      const Version version(update->Version(), update->Date());
      if (version.IsMoreRecentThan(CurVersion))
      {
        const std::size_t rank = std::distance(CurTypes.begin(), it);
        if (!Update || version.IsMoreRecentThan(UpdateVersion)
            || (version.EqualsTo(UpdateVersion) && rank < UpdateRank))
        {
          Update = update;
          UpdateRank = rank;
          UpdateVersion = version;
          Dbg(" using: rank={}", rank);
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
    {}

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
        Dbg("Remove failed saving: {}", res);
      }
    }

  private:
    const QFileInfo Info;
    Binary::OutputStream::Ptr Object;
  };

  const unsigned CHECK_UPDATE_DELAY = 60;

  class UpdateParameters
  {
  public:
    explicit UpdateParameters(Parameters::Container::Ptr params)
      : Params(std::move(params))
    {}

    String GetFeedUrl() const
    {
      Parameters::StringType url = Urls::DownloadsFeed();
      Params->FindValue(Parameters::ZXTuneQT::Update::FEED, url);
      return url;
    }

    unsigned GetCheckPeriod() const
    {
      Parameters::IntType period = Parameters::ZXTuneQT::Update::CHECK_PERIOD_DEFAULT;
      Params->FindValue(Parameters::ZXTuneQT::Update::CHECK_PERIOD, period);
      return static_cast<unsigned>(period);
    }

    std::time_t GetLastCheckTime() const
    {
      Parameters::IntType lastCheck = 0;
      Params->FindValue(Parameters::ZXTuneQT::Update::LAST_CHECK, lastCheck);
      return static_cast<std::time_t>(lastCheck);
    }

    void SetLastCheckTime(std::time_t time)
    {
      Params->SetValue(Parameters::ZXTuneQT::Update::LAST_CHECK, time);
    }

  private:
    const Parameters::Container::Ptr Params;
  };

  class UpdateCheckOperation : public Update::CheckOperation
  {
  public:
    UpdateCheckOperation(QWidget& parent, UpdateParameters params)
      : Parent(parent)
      , Params(std::move(params))
    {
      setParent(&parent);
      QTimer::singleShot(CHECK_UPDATE_DELAY * 1000, this, SLOT(ExecuteBackground()));
    }

    void Execute() override
    {
      try
      {
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
      {}
      catch (const Error& e)
      {
        emit ErrorOccurred(e);
      }
    }

    void ExecuteBackground() override
    {
      try
      {
        // If check was performed before
        if (!CheckPeriodExpired())
        {
          return;
        }
        if (const Product::Update::Ptr update = GetAvailableUpdateSilent())
        {
          if (QMessageBox::Save == ShowUpdateDialog(*update))
          {
            ApplyUpdate(*update);
          }
        }
      }
      catch (const Canceled&)
      {}
      catch (const Error&)
      {
        // Do not bother with implicit check errors
      }
    }

  private:
    Product::Update::Ptr GetAvailableUpdate() const
    {
      DialogProgressCallback cb(Parent, Update::CheckOperation::tr("Getting list of available updates"));
      return GetAvailableUpdate(cb);
    }

    Product::Update::Ptr GetAvailableUpdateSilent() const
    {
      return GetAvailableUpdate(Log::ProgressCallback::Stub());
    }

    Product::Update::Ptr GetAvailableUpdate(Log::ProgressCallback& cb) const
    {
      const QUrl feedUrl(ToQString(Params.GetFeedUrl()));
      const Binary::Data::Ptr feedData = Download(feedUrl, cb);
      UpdateState state;
      const std::unique_ptr<RSS::Visitor> rss = Downloads::CreateFeedVisitor("ZXTune", state);
      RSS::Parse(QByteArray(static_cast<const char*>(feedData->Start()), feedData->Size()), *rss);
      StoreLastCheckTime();
      return state.GetUpdate();
    }

    Binary::Data::Ptr DownloadWithProgress(const QUrl& url, const QString& text) const
    {
      DialogProgressCallback cb(Parent, text);
      return Download(url, cb);
    }

    QMessageBox::StandardButton ShowUpdateDialog(const Product::Update& update) const
    {
      const QString title = Update::CheckOperation::tr("Available version %1").arg(update.Version());
      QStringList msg;
      msg.append(update.Title());
      if (const int ageInDays = update.Date().daysTo(QDate::currentDate()))
      {
        msg.append(Update::CheckOperation::tr("%1 (%n day(s) ago)", "", ageInDays)
                       .arg(update.Date().toString(Qt::DefaultLocaleLongDate)));
      }
      msg.append(QString("<a href=\"%1\">%2</a>")
                     .arg(update.Description().toString())
                     .arg(Update::CheckOperation::tr("Download manually")));
      return QMessageBox::question(&Parent, title, msg.join("<br/>"), QMessageBox::Save | QMessageBox::Cancel);
    }

    void ApplyUpdate(const Product::Update& update) const
    {
      const QUrl packageUrl = update.Package();
      // do not use UI::SaveFileDialog
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

    void StoreLastCheckTime() const
    {
      Params.SetLastCheckTime(std::time(nullptr));
    }

    bool CheckPeriodExpired() const
    {
      if (const unsigned period = Params.GetCheckPeriod())
      {
        const std::time_t lastCheck = Params.GetLastCheckTime();
        const std::time_t now = std::time(nullptr);
        return now > lastCheck + period;
      }
      else
      {
        return false;
      }
    }

  private:
    QWidget& Parent;
    mutable UpdateParameters Params;
  };
}  // namespace

namespace Update
{
  CheckOperation* CheckOperation::Create(QWidget& parent)
  {
    try
    {
      UpdateParameters params(GlobalOptions::Instance().Get());
      if (IO::ResolveUri(params.GetFeedUrl()))
      {
        std::unique_ptr<CheckOperation> res(new UpdateCheckOperation(parent, params));
        return res.release();
      }
    }
    catch (const Error&)
    {}
    return nullptr;
  }
};  // namespace Update
