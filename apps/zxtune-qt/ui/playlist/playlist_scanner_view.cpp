/*
Abstract:
  Playlist scanner view implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist_scanner.h"
#include "playlist_scanner_view.h"
#include "playlist_scanner_view_ui.h"
#include "playlist_scanner_view_moc.h"

namespace
{
  class PlaylistScannerViewImpl : public PlaylistScannerView
                                , private Ui::PlaylistScannerView
  {
  public:
    PlaylistScannerViewImpl(QWidget* parent, PlaylistScanner* scanner)
      : Scanner(scanner)
    {
      //setup self
      setParent(parent);
      setupUi(this);
      hide();
      //make connections with scanner
      this->connect(Scanner, SIGNAL(OnScanStart()), this, SLOT(ScanStart()));
      this->connect(Scanner, SIGNAL(OnScanStop()), this, SLOT(ScanStop()));
      this->connect(Scanner, SIGNAL(OnProgress(unsigned, unsigned, unsigned)), SLOT(ShowProgress(unsigned)));
      this->connect(Scanner, SIGNAL(OnProgressMessage(const QString&, const QString&)), SLOT(ShowProgressMessage(const QString&)));
      this->connect(scanCancel, SIGNAL(clicked()), SLOT(ScanCancel()));
    }

    virtual void ScanStart()
    {
      scanCancel->setEnabled(true);
      show();
    }

    virtual void ScanCancel()
    {
      scanCancel->setEnabled(false);
      Scanner->Cancel();
    }

    virtual void ScanStop()
    {
      hide();
    }

    virtual void ShowProgress(unsigned progress)
    {
      scanProgress->setValue(progress);
    }

    virtual void ShowProgressMessage(const QString& message)
    {
      scanProgress->setToolTip(message);
    }
  private:
    PlaylistScanner* Scanner;
  };
}

PlaylistScannerView* PlaylistScannerView::Create(QWidget* parent, PlaylistScanner* scanner)
{
  return new PlaylistScannerViewImpl(parent, scanner);
}
