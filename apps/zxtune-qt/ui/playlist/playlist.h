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
#include "../../playitems_provider.h"
//common includes
#include <types.h>
//qt includes
#include <QtGui/QWidget>

namespace Log
{
  struct MessageData;
}

class Playlist : public QWidget
{
  Q_OBJECT
public:
  //creator
  static Playlist* Create(class QMainWindow* parent);

public slots:
  //items operating
  virtual void AddItemByPath(const String& itemPath) = 0;
  virtual void NextItem() = 0;
  virtual void PrevItem() = 0;
  virtual void PlayItem() = 0;
  virtual void PauseItem() = 0;
  virtual void StopItem() = 0;
  //playlist operating
  virtual void AddFiles() = 0;
  virtual void Clear() = 0;
  virtual void Sort() = 0;
  virtual void Random(bool isRandom) = 0;
  virtual void Loop(bool isLooped) = 0;
private slots:
  virtual void AddItem(Playitem::Ptr item) = 0;
  virtual void SetItem(class QListWidgetItem*) = 0;
  virtual void SelectItem(class QListWidgetItem*) = 0;
  virtual void ClearSelected() = 0;
  virtual void ShowProgress(const Log::MessageData&) = 0;
signals:
  void OnItemSet(const Playitem&);
  void OnItemSelected(const Playitem&);
};

#endif //ZXTUNE_QT_PLAYLIST_H_DEFINED
