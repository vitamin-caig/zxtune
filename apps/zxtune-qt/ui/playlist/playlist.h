/*
Abstract:
  Playlist widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//qt includes
#include <QtCore/QModelIndex>
#include <QtGui/QWidget>

class Playlist : public QWidget
{
  Q_OBJECT
public:
  //creator
  static Playlist* Create(class QMainWindow* parent);

public slots:
  //items operating
  virtual void AddItems(const QStringList& itemsPath) = 0;
  virtual void NextItem() = 0;
  virtual void PrevItem() = 0;
  virtual void PlayItem() = 0;
  virtual void PauseItem() = 0;
  virtual void StopItem() = 0;
  //playlist operating
  virtual void AddFiles() = 0;
  virtual void Clear() = 0;
  virtual void Random(bool isRandom) = 0;
  virtual void Loop(bool isLooped) = 0;
signals:
  void OnItemSet(const Playitem&);
};

#endif //ZXTUNE_QT_PLAYLIST_H_DEFINED
