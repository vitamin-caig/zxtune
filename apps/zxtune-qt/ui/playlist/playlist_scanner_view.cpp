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
#include "playlist_scanner_view.h"
#include "playlist_scanner_view.ui.h"
#include "supp/playlist/playlist_scanner.h"

namespace
{
  class PlaylistScannerViewImpl : public PlaylistScannerView
                                , private Ui::PlaylistScannerView
  {
  public:
    PlaylistScannerViewImpl(QWidget* parent, PlaylistScanner& scanner)
      : Scanner(scanner)
    {
      //setup self
      setParent(parent);
      setupUi(this);
      hide();
      //make connections with scanner
      this->connect(&Scanner, SIGNAL(OnScanStart()), this, SLOT(ScanStart()));
      this->connect(&Scanner, SIGNAL(OnScanStop()), this, SLOT(ScanStop()));
      this->connect(&Scanner, SIGNAL(OnProgressStatus(unsigned, unsigned, unsigned)), SLOT(ShowProgress(unsigned, unsigned, unsigned)));
      this->connect(&Scanner, SIGNAL(OnProgressMessage(const QString&, const QString&)), SLOT(ShowProgressMessage(const QString&)));
      this->connect(&Scanner, SIGNAL(OnResolvingStart()), this, SLOT(ShowResolving()));
      this->connect(&Scanner, SIGNAL(OnResolvingStop()), this, SLOT(HideResolving()));
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
      Scanner.Cancel();
    }

    virtual void ScanStop()
    {
      hide();
    }

    virtual void ShowProgress(unsigned progress, unsigned itemsDone, unsigned totalItems)
    {
      const QString itemsProgressText = QString("%1/%2").arg(itemsDone).arg(totalItems);
      itemsProgress->setText(itemsProgressText);
      scanProgress->setValue(progress);
    }

    virtual void ShowProgressMessage(const QString& message)
    {
      scanProgress->setToolTip(message);
    }

    virtual void ShowResolving()
    {
      itemsProgress->setText(tr("Searching..."));
    }

    virtual void HideResolving()
    {
      itemsProgress->setText(QString());
    }
  private:
    PlaylistScanner& Scanner;
  };
}

PlaylistScannerView* PlaylistScannerView::Create(QWidget* parent, PlaylistScanner& scanner)
{
  return new PlaylistScannerViewImpl(parent, scanner);
}
