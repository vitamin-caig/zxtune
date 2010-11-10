/*
Abstract:
  Playlist scanner view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_SCANNER_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SCANNER_VIEW_H_DEFINED

//qt includes
#include <QtGui/QWidget>

class PlaylistScannerView : public QWidget
{
  Q_OBJECT
protected:
  explicit PlaylistScannerView(QWidget& parent);
public:
  static PlaylistScannerView* Create(QWidget& parent, class PlaylistScanner& scanner);

public slots:
  virtual void ScanStart() = 0;
  virtual void ScanCancel() = 0;
  virtual void ScanStop() = 0;
  virtual void ShowProgress(unsigned, unsigned, unsigned) = 0;
  virtual void ShowProgressMessage(const QString&) = 0;
  virtual void ShowResolving() = 0;
  virtual void HideResolving() = 0;
};

#endif //ZXTUNE_QT_PLAYLIST_SCANNER_VIEW_H_DEFINED
