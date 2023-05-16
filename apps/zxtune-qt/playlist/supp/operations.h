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

// local includes
#include "controller.h"
#include "model.h"
// qt includes
#include <QtCore/QObject>

namespace Playlist::Item
{
  class SelectionOperation
    : public QObject
    , public Playlist::Item::StorageAccessOperation
  {
    Q_OBJECT
  public:
    typedef std::shared_ptr<SelectionOperation> Ptr;
  signals:
    void ResultAcquired(Playlist::Model::IndexSet::Ptr);
  };

  class TextResultOperation
    : public QObject
    , public Playlist::Item::StorageAccessOperation
  {
    Q_OBJECT
  public:
    typedef std::shared_ptr<TextResultOperation> Ptr;
  signals:
    void ResultAcquired(Playlist::TextNotification::Ptr);
  };

  // rip-offs
  SelectionOperation::Ptr CreateSelectAllRipOffsOperation();
  SelectionOperation::Ptr CreateSelectRipOffsOfSelectedOperation(Playlist::Model::IndexSet::Ptr items);
  SelectionOperation::Ptr CreateSelectRipOffsInSelectedOperation(Playlist::Model::IndexSet::Ptr items);
  // duplicates
  SelectionOperation::Ptr CreateSelectAllDuplicatesOperation();
  SelectionOperation::Ptr CreateSelectDuplicatesOfSelectedOperation(Playlist::Model::IndexSet::Ptr items);
  SelectionOperation::Ptr CreateSelectDuplicatesInSelectedOperation(Playlist::Model::IndexSet::Ptr items);
  // other
  SelectionOperation::Ptr CreateSelectTypesOfSelectedOperation(Playlist::Model::IndexSet::Ptr items);
  SelectionOperation::Ptr CreateSelectFilesOfSelectedOperation(Playlist::Model::IndexSet::Ptr items);
  SelectionOperation::Ptr CreateSelectAllUnavailableOperation();
  SelectionOperation::Ptr CreateSelectUnavailableInSelectedOperation(Playlist::Model::IndexSet::Ptr items);
}  // namespace Playlist::Item
