/**
 *
 * @file
 *
 * @brief Update checking implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/update/check.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/tools/errordialog.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "apps/zxtune-qt/update/downloads.h"
#include "apps/zxtune-qt/update/parameters.h"
#include "apps/zxtune-qt/update/product.h"
#include "apps/zxtune-qt/urls.h"

#include "debug/log.h"
#include "platform/version/fields.h"
#include "time/duration.h"
#include "time/instant.h"

#include "contract.h"
#include "error.h"
#include "make_ptr.h"

#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressDialog>

#include <ctime>
#include <utility>

namespace
{
  const Debug::Stream Dbg("UpdateCheck");
}

namespace
{
  class UICallback
  {
  public:
    using Ptr = std::unique_ptr<UICallback>;
    virtual ~UICallback() = default;

    // @return false if cancelled
    virtual bool UpdateProgress(uint_t value) = 0;
    virtual void ShowError(const Error& err) = 0;
  };

  class DialogUICallback : public UICallback
  {
  public:
    DialogUICallback(QWidget& parent, const QString& text)
      : Progress(&parent, Qt::Dialog)
    {
      Progress.setMinimumDuration(1);
      Progress.setLabelText(text);
      Progress.setWindowModality(Qt::WindowModal);
      Progress.setValue(0);
    }

    bool UpdateProgress(uint_t current) override
    {
      if (Progress.wasCanceled())
      {
        return false;
      }
      Progress.setValue(current);
      return true;
    }

    void ShowError(const Error& err) override
    {
      ShowErrorMessage({}, err);
    }

  private:
    QProgressDialog Progress;
  };

  class SilentUICallback : public UICallback
  {
  public:
    bool UpdateProgress(uint_t /*current*/) override
    {
      return true;
    }

    void ShowError(const Error& /*err*/) override {}
  };

  String GetUserAgent()
  {
    const std::unique_ptr<Strings::FieldsSource> fields = Platform::Version::CreateVersionFieldsSource();
    return Strings::Template::Instantiate("[Program]/[Version] ([Platform]-[Arch])", *fields);
  }

  QNetworkRequest MakeRequest(const QUrl& url)
  {
    QNetworkRequest result(url);
    result.setHeader(QNetworkRequest::UserAgentHeader, ToQString(GetUserAgent()));
    result.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    return result;
  }

  void Download(QNetworkAccessManager& mgr, const QUrl& url, UICallback::Ptr cb,
                std::function<void(QByteArray)> onResult)
  {
    Dbg("Downloading {}", FromQString(url.toString()));
    auto* reply = mgr.get(MakeRequest(url));
    Require(
        QObject::connect(reply, &QNetworkReply::downloadProgress, [reply, ui = cb.get()](qint64 done, qint64 total) {
          Dbg("Downloaded {}/{} err={}", done, total, uint_t(reply->error()));
          if (!reply->error() && !ui->UpdateProgress(total > 0 ? uint_t(done * 100 / total) : 0))
          {
            Dbg("Aborted");
            reply->abort();
          }
        }));
    Require(QObject::connect(reply, &QNetworkReply::errorOccurred,
                             [reply, ui = std::move(cb)](QNetworkReply::NetworkError err) {
                               const auto& errStr = FromQString(reply->errorString());
                               Dbg("Network error {}: {}", uint_t(err), errStr);
                               if (QNetworkReply::OperationCanceledError != err)
                               {
                                 ui->ShowError(Error(THIS_LINE, errStr));
                               }
                             }));
    Require(QObject::connect(reply, &QNetworkReply::finished, [reply, cb = std::move(onResult)] {
      Dbg("Finished");
      if (!reply->error())
      {
        cb(reply->readAll());
      }
      reply->deleteLater();
    }));
  }

  QDate VersionToDate(const QString& str, const QDate& fallback)
  {
    const QDate asDate = QDate::fromString(str, "yyyyMMdd");
    return asDate.isValid() ? asDate : fallback;
  }

  unsigned short VersionToRevision(const QString& str)
  {
    static const QLatin1String REV_FORMAT(R"((?:r(?:ev)?)?(\d{4,5}).*)");
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
    Version() = default;

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
        // prefer much more valid
        return thisValid && !rhValid;
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
    {}

    void OnDownload(Product::Update::Ptr update) override
    {
      Dbg("Update {}:", FromQString(update->Title()));
      const auto type = Product::GetUpdateType(update->Platform(), update->Architecture(), update->Packaging());
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

    FileTransaction(const FileTransaction&) = delete;

    ~FileTransaction()
    {
      Rollback();
    }

    std::unique_ptr<QIODevice> Begin()
    {
      Rollback();
      auto obj = std::make_unique<QFile>(Info.absoluteFilePath());
      Require(obj->open(QIODevice::WriteOnly));
      Object = obj.get();
      return obj;
    }

    void Commit()
    {
      Object = {};
    }

    void Rollback()
    {
      if (Object)
      {
        const auto result = Object->remove();
        Dbg("Remove failed saving: {}", result);
        Object = {};
      }
    }

  private:
    const QFileInfo Info;
    QPointer<QFile> Object;
  };

  const auto CHECK_UPDATE_DELAY = Time::Seconds(60);

  // TODO: move to Time
  using AtSecond = Time::Instant<Time::Second>;

  auto Now()
  {
    return AtSecond(std::time(nullptr));
  }

  class UpdateParameters
  {
  public:
    explicit UpdateParameters(Parameters::Container::Ptr params)
      : Params(std::move(params))
    {}

    QUrl GetFeedUrl() const
    {
      using namespace Parameters;
      const auto url = GetString(*Params, ZXTuneQT::Update::FEED, Urls::DownloadsFeed());
      return ToQString(url);
    }

    std::optional<Time::Seconds> GetCheckPeriod() const
    {
      using namespace Parameters::ZXTuneQT::Update;
      if (const auto val =
              Parameters::GetInteger<Time::Seconds::ValueType>(*Params, CHECK_PERIOD, CHECK_PERIOD_DEFAULT))
      {
        return Time::Seconds(val);
      }
      else
      {
        return std::nullopt;
      }
    }

    AtSecond GetLastCheckTime() const
    {
      using namespace Parameters;
      return AtSecond{GetInteger<AtSecond::ValueType>(*Params, ZXTuneQT::Update::LAST_CHECK)};
    }

    void SetLastCheckTime(AtSecond time)
    {
      Params->SetValue(Parameters::ZXTuneQT::Update::LAST_CHECK, time.Get());
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
      , Network(this)
    {
      setParent(&parent);
      ScheduleBackgroundCheck(CHECK_UPDATE_DELAY);
    }

    void Execute() override
    {
      auto cb = MakePtr<DialogUICallback>(Parent, Update::CheckOperation::tr("Getting list of available updates"));
      GetAvailableUpdate(std::move(cb), &UpdateCheckOperation::ProcessUpdate);
    }

  private:
    using OnUpdateReady = void (UpdateCheckOperation::*)(Product::Update::Ptr);

    void GetAvailableUpdate(UICallback::Ptr cb, OnUpdateReady onResult)
    {
      Download(Network, Params.GetFeedUrl(), std::move(cb), [this, cb = std::move(onResult)](QByteArray feedData) {
        UpdateState state;
        const auto rss = Downloads::CreateFeedVisitor("ZXTune", state);
        RSS::Parse(feedData, *rss);
        StoreLastCheckTime();
        (this->*cb)(state.GetUpdate());
      });
    }

    void ProcessUpdate(Product::Update::Ptr update)
    {
      if (update)
      {
        ProcessUpdateSilent(std::move(update));
      }
      else
      {
        QMessageBox::information(&Parent, QString(), Update::CheckOperation::tr("No new updates found"));
      }
    }

    void ProcessUpdateSilent(Product::Update::Ptr update)
    {
      if (!update)
      {
        return;
      }
      if (QMessageBox::Save == ShowUpdateDialog(*update))
      {
        ApplyUpdate(*update);
      }
    }

    QMessageBox::StandardButton ShowUpdateDialog(const Product::Update& update) const
    {
      const QString title = Update::CheckOperation::tr("Available version %1").arg(update.Version());
      QStringList msg;
      msg.append(update.Title());
      if (const int ageInDays = update.Date().daysTo(QDate::currentDate()))
      {
        msg.append(Update::CheckOperation::tr("%1 (%n day(s) ago)", "", ageInDays).arg(update.Date().toString()));
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
      auto cb = MakePtr<DialogUICallback>(Parent, Update::CheckOperation::tr("Downloading file"));
      Download(Network, url, std::move(cb), [filename](QByteArray content) {
        FileTransaction transaction(filename);
        if (const auto target = transaction.Begin())
        {
          target->write(content);
        }
        transaction.Commit();
      });
    }

    void StoreLastCheckTime() const
    {
      Params.SetLastCheckTime(AtSecond(std::time(nullptr)));
    }

    void ScheduleBackgroundCheck(Time::Milliseconds delay)
    {
      Dbg("Schedule verify after {}ms", delay.Get());
      QTimer::singleShot(delay.Get(), this, &UpdateCheckOperation::ExecuteBackground);
    }

    std::optional<Time::Seconds> GetCheckDelay() const
    {
      if (const auto period = Params.GetCheckPeriod())
      {
        const auto nextCheck = Params.GetLastCheckTime() + *period;
        const auto now = Now();
        return now < nextCheck ? nextCheck - now : Time::Seconds(0);
      }
      else
      {
        return std::nullopt;
      }
    }

    void ExecuteBackground()
    {
      // Update check is performed only if enabled, but operation executes periodically in order to cover settings
      // change
      auto after = Time::Seconds(3600);
      if (!GetCheckDelay().value_or(after))
      {
        Dbg("Check updates now");
        GetAvailableUpdate(MakePtr<SilentUICallback>(), &UpdateCheckOperation::ProcessUpdateSilent);
      }
      if (const auto delay = GetCheckDelay().value_or(Time::Seconds(0)))
      {
        // Ignore zero delays - here means failed update check
        after = delay;
      }
      ScheduleBackgroundCheck(after);
    }

  private:
    QWidget& Parent;
    mutable UpdateParameters Params;
    mutable QNetworkAccessManager Network;
  };
}  // namespace

namespace Update
{
  CheckOperation* CheckOperation::Create(QWidget& parent)
  {
    UpdateParameters params(GlobalOptions::Instance().Get());
    return new UpdateCheckOperation(parent, std::move(params));
  }
};  // namespace Update
