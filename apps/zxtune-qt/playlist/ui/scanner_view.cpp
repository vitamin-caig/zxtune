/**
* 
* @file
*
* @brief Scanner view implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "scanner_view.h"
#include "scanner_view.ui.h"
#include "playlist/supp/scanner.h"
//common includes
#include <contract.h>
//library includes
#include <debug/log.h>

namespace
{
  const Debug::Stream Dbg("Playlist::UI::ScannerView");

  class ScannerViewImpl : public Playlist::UI::ScannerView
                        , private Playlist::UI::Ui_ScannerView
  {
  public:
    ScannerViewImpl(QWidget& parent, Playlist::Scanner::Ptr scanner)
      : Playlist::UI::ScannerView(parent)
      , Scanner(scanner)
    {
      //setup self
      setupUi(this);
      hide();
      //make connections with scanner
      Require(connect(Scanner, SIGNAL(ScanStarted(Playlist::ScanStatus::Ptr)), this, SLOT(ScanStart(Playlist::ScanStatus::Ptr))));
      Require(connect(Scanner, SIGNAL(ScanStopped()), this, SLOT(ScanStop())));
      Require(connect(Scanner, SIGNAL(ScanProgressChanged(unsigned)), SLOT(ShowProgress(unsigned))));
      Require(connect(Scanner, SIGNAL(ScanMessageChanged(const QString&)), SLOT(ShowProgressMessage(const QString&))));
      Require(Scanner->connect(scanCancel, SIGNAL(clicked()), SLOT(Stop())));
      Require(Scanner->connect(scanPause, SIGNAL(toggled(bool)), SLOT(Pause(bool))));

      Dbg("Created at %1%", this);
    }

    virtual ~ScannerViewImpl()
    {
      Dbg("Destroyed at %1%", this);
    }

    virtual void ScanStart(Playlist::ScanStatus::Ptr status)
    {
      Dbg("Scan started for %1%", this);
      Status = status;
      show();
    }

    virtual void ScanStop()
    {
      Dbg("Scan stopped for %1%", this);
      hide();
      scanPause->setChecked(false);
    }

    virtual void ShowProgress(unsigned progress)
    {
      const QString itemsProgressText = QString::fromAscii("%1/%2%3").arg(Status->DoneFiles()).arg(Status->FoundFiles()).arg(Status->SearchFinished() ? ' ' : '+');
      itemsProgress->setText(itemsProgressText);
      itemsProgress->setToolTip(Status->CurrentFile());
      scanProgress->setValue(progress);
      CheckedShow();
    }

    virtual void ShowProgressMessage(const QString& message)
    {
      scanProgress->setToolTip(message);
    }
  private:
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
}

namespace Playlist
{
  namespace UI
  {
    ScannerView::ScannerView(QWidget& parent) : QWidget(&parent)
    {
    }

    ScannerView* ScannerView::Create(QWidget& parent, Playlist::Scanner::Ptr scanner)
    {
      return new ScannerViewImpl(parent, scanner);
    }
  }
}
