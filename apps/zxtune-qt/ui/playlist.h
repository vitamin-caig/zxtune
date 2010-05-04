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
  static Playlist* Create(QWidget* parent);

public slots:
  virtual void AddItemByPath(const String& itemPath) = 0;
  virtual void NextItem() = 0;
private slots:
  virtual void AddItem(const struct ModuleItem& item) = 0;
  virtual void SelectItem(class QListWidgetItem*) = 0;
  virtual void ShowProgress(const Log::MessageData&) = 0;
signals:
  void OnItemSelected(const ModuleItem&);
};

#endif //ZXTUNE_QT_PLAYLIST_H_DEFINED
