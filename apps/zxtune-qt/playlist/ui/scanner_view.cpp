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
#include "scanner_view.h"
#include "scanner_view.ui.h"
#include "playlist/supp/scanner.h"
//common includes
#include <contract.h>
#include <debug_log.h>

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
      Require(connect(Scanner, SIGNAL(OnScanStart()), this, SLOT(ScanStart())));
      Require(connect(Scanner, SIGNAL(OnScanStop()), this, SLOT(ScanStop())));
      Require(connect(Scanner, SIGNAL(OnProgressStatus(unsigned, unsigned, unsigned)), SLOT(ShowProgress(unsigned, unsigned, unsigned))));
      Require(connect(Scanner, SIGNAL(OnProgressMessage(const QString&, const QString&)), SLOT(ShowProgressMessage(const QString&))));
      Require(connect(Scanner, SIGNAL(OnResolvingStart()), this, SLOT(ShowResolving())));
      Require(connect(Scanner, SIGNAL(OnResolvingStop()), this, SLOT(HideResolving())));
      Require(connect(scanCancel, SIGNAL(clicked()), SLOT(ScanCancel())));

      Dbg("Created at %1%", this);
    }

    virtual ~ScannerViewImpl()
    {
      Dbg("Destroyed at %1%", this);
    }

    virtual void ScanStart()
    {
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
      scanCancel->setEnabled(true);
    }

    virtual void ShowProgress(unsigned progress, unsigned itemsDone, unsigned totalItems)
    {
      const QString itemsProgressText = QString::fromUtf8("%1/%2").arg(itemsDone).arg(totalItems);
      itemsProgress->setText(itemsProgressText);
      scanProgress->setValue(progress);
      CheckedShow();
    }

    virtual void ShowProgressMessage(const QString& message)
    {
      scanProgress->setToolTip(message);
    }

    virtual void ShowResolving()
    {
      itemsProgress->setText(Playlist::UI::ScannerView::tr("Searching..."));
      CheckedShow();
    }

    virtual void HideResolving()
    {
      itemsProgress->setText(QString());
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
