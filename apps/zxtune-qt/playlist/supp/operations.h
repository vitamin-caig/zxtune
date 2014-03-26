/**
* 
* @file
*
* @brief Playlist common operations interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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
    public:
      typedef boost::shared_ptr<SelectionOperation> Ptr;
    signals:
      void ResultAcquired(Playlist::Model::IndexSetPtr);
    };

    class TextResultOperation : public QObject
                              , public Playlist::Item::StorageAccessOperation
    {
      Q_OBJECT
    public:
      typedef boost::shared_ptr<TextResultOperation> Ptr;
    signals:
      void ResultAcquired(Playlist::TextNotification::Ptr);
    };

    //rip-offs
    SelectionOperation::Ptr CreateSelectAllRipOffsOperation();
    SelectionOperation::Ptr CreateSelectRipOffsOfSelectedOperation(Playlist::Model::IndexSetPtr items);
    SelectionOperation::Ptr CreateSelectRipOffsInSelectedOperation(Playlist::Model::IndexSetPtr items);
    //duplicates
    SelectionOperation::Ptr CreateSelectAllDuplicatesOperation();
    SelectionOperation::Ptr CreateSelectDuplicatesOfSelectedOperation(Playlist::Model::IndexSetPtr items);
    SelectionOperation::Ptr CreateSelectDuplicatesInSelectedOperation(Playlist::Model::IndexSetPtr items);
    //other
    SelectionOperation::Ptr CreateSelectTypesOfSelectedOperation(Playlist::Model::IndexSetPtr items);
    SelectionOperation::Ptr CreateSelectAllUnavailableOperation();
    SelectionOperation::Ptr CreateSelectUnavailableInSelectedOperation(Playlist::Model::IndexSetPtr items);
  }
}
