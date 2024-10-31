/**
 *
 * @file
 *
 * @brief Scanner view implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/playlist/ui/scanner_view.h"
#include "apps/zxtune-qt/playlist/supp/scanner.h"
#include "scanner_view.ui.h"
// common includes
#include <contract.h>
// library includes
#include <debug/log.h>

namespace
{
  const Debug::Stream Dbg("Playlist::UI::ScannerView");

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
      // make connections with scanner
      Require(connect(Scanner, &Playlist::Scanner::ScanStarted, this, &ScannerViewImpl::ScanStart));
      Require(connect(Scanner, &Playlist::Scanner::ScanStopped, this, &ScannerViewImpl::ScanStop));
      Require(connect(Scanner, &Playlist::Scanner::ScanProgressChanged, this, &ScannerViewImpl::ShowProgress));
      Require(connect(Scanner, &Playlist::Scanner::ScanMessageChanged, scanProgress, &QProgressBar::setToolTip));
      Require(connect(scanCancel, &QToolButton::clicked, Scanner, &Playlist::Scanner::Stop));
      Require(connect(scanPause, &QToolButton::toggled, Scanner, &Playlist::Scanner::Pause));

      Dbg("Created at {}", Self());
    }

    ~ScannerViewImpl() override
    {
      Dbg("Destroyed at {}", Self());
    }

  private:
    void ScanStart(Playlist::ScanStatus::Ptr status)
    {
      Dbg("Scan started for {}", Self());
      Status = status;
      show();
    }

    void ScanStop()
    {
      Dbg("Scan stopped for {}", Self());
      hide();
      scanPause->setChecked(false);
    }

    void ShowProgress(unsigned progress)
    {
      // new file started
      if (progress == 0)
      {
        const QString itemsProgressText = QString::fromLatin1("%1/%2%3")
                                              .arg(Status->DoneFiles())
                                              .arg(Status->FoundFiles())
                                              .arg(Status->SearchFinished() ? ' ' : '+');
        itemsProgress->setText(itemsProgressText);
        itemsProgress->setToolTip(Status->CurrentFile());
      }
      scanProgress->setValue(progress);
      CheckedShow();
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
    Playlist::ScanStatus::Ptr Status;
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
