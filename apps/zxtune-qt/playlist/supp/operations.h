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
#include "model.h"

namespace Playlist
{
  namespace Item
  {
    typedef boost::shared_ptr<const Playlist::Model::IndexSet> SelectionPtr;

    class PromisedSelectionOperation : public Playlist::Item::StorageAccessOperation
    {
    public:
      typedef boost::shared_ptr<PromisedSelectionOperation> Ptr;

      virtual SelectionPtr GetResult() const = 0;
    };

    class TextOperationResult
    {
    public:
      typedef boost::shared_ptr<const TextOperationResult> Ptr;
      virtual ~TextOperationResult() {}

      virtual String GetBasicResult() const = 0;
      virtual String GetDetailedResult() const = 0;
    };

    class PromisedTextResultOperation : public Playlist::Item::StorageAccessOperation
    {
    public:
      typedef boost::shared_ptr<PromisedTextResultOperation> Ptr;

      virtual TextOperationResult::Ptr GetResult() const = 0;
    };

    PromisedSelectionOperation::Ptr CreateSelectAllRipOffsOperation();
    PromisedSelectionOperation::Ptr CreateSelectRipOffsOfSelectedOperation(const Playlist::Model::IndexSet& items);
    PromisedSelectionOperation::Ptr CreateSelectRipOffsInSelectedOperation(const Playlist::Model::IndexSet& items);
    PromisedSelectionOperation::Ptr CreateSelectTypesOfSelectedOperation(const Playlist::Model::IndexSet& items);
    PromisedSelectionOperation::Ptr CreateSelectAllDuplicatesOperation();
    PromisedSelectionOperation::Ptr CreateSelectDuplicatesOfSelectedOperation(const Playlist::Model::IndexSet& items);
    PromisedSelectionOperation::Ptr CreateSelectDuplicatesInSelectedOperation(const Playlist::Model::IndexSet& items);
    PromisedTextResultOperation::Ptr CreateCollectPathsOperation(const Playlist::Model::IndexSet& items);
    PromisedTextResultOperation::Ptr CreateCollectStatisticOperation();
    PromisedTextResultOperation::Ptr CreateCollectStatisticOperation(const Playlist::Model::IndexSet& items);
    PromisedTextResultOperation::Ptr CreateExportOperation(const String& nameTemplate);
    PromisedTextResultOperation::Ptr CreateExportOperation(const Playlist::Model::IndexSet& items, const String& nameTemplate);
    PromisedTextResultOperation::Ptr CreateConvertOperation(const Playlist::Model::IndexSet& items, const String& nameTemplate);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_OPERATIONS_H_DEFINED
