/*
Abstract:
  Playlist operations factories

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_OPERATIONS_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_OPERATIONS_H_DEFINED

//local includes
#include "controller.h"
#include "model.h"
//qt includes
#include <QtCore/QObject>

namespace Playlist
{
  namespace Item
  {
    class SelectionOperation : public QObject
                             , public Playlist::Item::StorageAccessOperation
    {
      Q_OBJECT
    protected:
      explicit SelectionOperation(QObject& parent);
    public:
      typedef boost::shared_ptr<SelectionOperation> Ptr;
    signals:
      void ResultAcquired(Playlist::Model::IndexSetPtr);
    };

    class TextResultOperation : public QObject
                              , public Playlist::Item::StorageAccessOperation
    {
      Q_OBJECT
    protected:
      explicit TextResultOperation(QObject& parent);
    public:
      typedef boost::shared_ptr<TextResultOperation> Ptr;
    signals:
      void ResultAcquired(Playlist::TextNotification::Ptr);
    };

    //rip-offs
    SelectionOperation::Ptr CreateSelectAllRipOffsOperation(QObject& parent);
    SelectionOperation::Ptr CreateSelectRipOffsOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items);
    SelectionOperation::Ptr CreateSelectRipOffsInSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items);
    //duplicates
    SelectionOperation::Ptr CreateSelectAllDuplicatesOperation(QObject& parent);
    SelectionOperation::Ptr CreateSelectDuplicatesOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items);
    SelectionOperation::Ptr CreateSelectDuplicatesInSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items);
    //other
    SelectionOperation::Ptr CreateSelectTypesOfSelectedOperation(QObject& parent, Playlist::Model::IndexSetPtr items);
    //statistic
    TextResultOperation::Ptr CreateCollectStatisticOperation(QObject& parent);
    TextResultOperation::Ptr CreateCollectStatisticOperation(QObject& parent, Playlist::Model::IndexSetPtr items);
    //export
    TextResultOperation::Ptr CreateExportOperation(QObject& parent, const String& nameTemplate);
    TextResultOperation::Ptr CreateExportOperation(QObject& parent, Playlist::Model::IndexSetPtr items, const String& nameTemplate);
    //convert
    TextResultOperation::Ptr CreateConvertOperation(QObject& parent, Playlist::Model::IndexSetPtr items,
      const String& type, Parameters::Accessor::Ptr params);
    //other
    TextResultOperation::Ptr CreateCollectPathsOperation(QObject& parent, Playlist::Model::IndexSetPtr items);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_OPERATIONS_H_DEFINED
