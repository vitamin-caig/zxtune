/*
Abstract:
  Playlist scanner view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_SCANNER_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_SCANNER_VIEW_H_DEFINED

//local includes
#include "playlist/supp/scanner.h"
//qt includes
#include <QtGui/QWidget>

namespace Playlist
{
  namespace UI
  {
    class ScannerView : public QWidget
    {
      Q_OBJECT
    protected:
      explicit ScannerView(QWidget& parent);
    public:
      static ScannerView* Create(QWidget& parent, Playlist::Scanner::Ptr scanner);

    private slots:
      virtual void ScanStart(Playlist::ScanStatus::Ptr) = 0;
      virtual void ScanCancel() = 0;
      virtual void ScanStop() = 0;
      virtual void ShowProgress(unsigned) = 0;
      virtual void ShowProgressMessage(const QString&) = 0;
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_SCANNER_VIEW_H_DEFINED
