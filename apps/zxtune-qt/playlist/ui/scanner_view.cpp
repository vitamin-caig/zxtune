/**
 *
 * @file
 *
 * @brief Scanner view implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/ui/scanner_view.h"

#include "apps/zxtune-qt/playlist/supp/scanner.h"
#include "scanner_view.ui.h"

#include "debug/log.h"

#include "contract.h"

#include <QtCore/QTimer>

namespace
{
  const Debug::Stream Dbg("Playlist::UI::ScannerView");
  const auto UPDATE_PERIOD = 500;

  class ScannerViewImpl
    : public Playlist::UI::ScannerView
    , private Playlist::UI::Ui_ScannerView
  {
  public:
    ScannerViewImpl(QWidget& parent, Playlist::Scanner::Ptr scanner)
      : Playlist::UI::ScannerView(parent)
      , Scanner(scanner)
    {
      // setup self
      setupUi(this);
      hide();
      UpdateTimer.setInterval(UPDATE_PERIOD);
      // make connections with scanner
      Require(connect(Scanner, &Playlist::Scanner::ScanStarted, this, &ScannerViewImpl::ScanStart));
      Require(connect(Scanner, &Playlist::Scanner::ScanStopped, this, &ScannerViewImpl::ScanStop));
      Require(connect(&UpdateTimer, &QTimer::timeout, this, &ScannerViewImpl::UpdateStatus));
      Require(connect(scanCancel, &QToolButton::clicked, Scanner, &Playlist::Scanner::Stop));
      Require(connect(scanPause, &QToolButton::toggled, Scanner, &Playlist::Scanner::Pause));

      Dbg("Created at {}", Self());
    }

    ~ScannerViewImpl() override
    {
      Dbg("Destroyed at {}", Self());
    }

  private:
    void ScanStart()
    {
      Dbg("Scan started for {}", Self());
      UpdateStatus();
      UpdateTimer.start();
    }

    void ScanStop()
    {
      Dbg("Scan stopped for {}", Self());
      UpdateTimer.stop();
      hide();
      scanPause->setChecked(false);
    }

    void UpdateStatus()
    {
      if (const auto& status = Scanner->GetStatus())
      {
        itemsProgress->setText(QString::fromLatin1("%1/%2%3")
                                   .arg(status->DoneFiles)
                                   .arg(status->FoundFiles)
                                   .arg(status->SearchFinished ? ' ' : '+'));
        itemsProgress->setToolTip(status->CurrentFile);
        scanProgress->setValue(status->Progress);
        scanProgress->setToolTip(status->Message);
        CheckedShow();
      }
    }

    const void* Self() const
    {
      return this;
    }

    void CheckedShow()
    {
      if (!isVisible())
      {
        show();
      }
    }

  private:
    const Playlist::Scanner::Ptr Scanner;
    QTimer UpdateTimer;
  };
}  // namespace

namespace Playlist::UI
{
  ScannerView::ScannerView(QWidget& parent)
    : QWidget(&parent)
  {}

  ScannerView* ScannerView::Create(QWidget& parent, Playlist::Scanner::Ptr scanner)
  {
    return new ScannerViewImpl(parent, scanner);
  }
}  // namespace Playlist::UI
