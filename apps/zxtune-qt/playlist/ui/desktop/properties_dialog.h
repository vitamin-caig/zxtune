/*
Abstract:
  Playlist items properties dialog for desktop

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_DESKTOP_PROPERTIES_DIALOG_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_DESKTOP_PROPERTIES_DIALOG_H_DEFINED

//local includes
#include "playlist/supp/model.h"
#include "playlist/supp/operations_search.h"
//qt includes
#include <QtGui/QDialog>

class QAbstractButton;
namespace Playlist
{
  namespace UI
  {
    class PropertiesDialog : public QDialog
    {
      Q_OBJECT
    protected:
      explicit PropertiesDialog(QWidget& parent);
    public:
      typedef boost::shared_ptr<PropertiesDialog> Ptr;

      static Ptr Create(QWidget& parent, Item::Data::Ptr item);
    private slots:
      virtual void ButtonClicked(QAbstractButton* button) = 0;
    signals:
      void ResetToDefaults();
    };

    void ExecutePropertiesDialog(class TableView& view, Playlist::Model::IndexSetPtr scope, Controller& controller);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_DESKTOP_PROPERTIES_DIALOG_H_DEFINED
