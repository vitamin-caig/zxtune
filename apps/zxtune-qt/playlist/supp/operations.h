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

    //statistic
    class StatisticTextNotification : public Playlist::TextNotification
    {
    public:
      typedef boost::shared_ptr<StatisticTextNotification> Ptr;

      virtual void AddInvalid() = 0;
      virtual void AddValid(const String& type, const Time::MillisecondsDuration& duration, std::size_t size) = 0;
    };

    TextResultOperation::Ptr CreateCollectStatisticOperation(StatisticTextNotification::Ptr result);
    TextResultOperation::Ptr CreateCollectStatisticOperation(Playlist::Model::IndexSetPtr items, StatisticTextNotification::Ptr result);

    //export
    class ConversionResultNotification : public Playlist::TextNotification
    {
    public:
      typedef boost::shared_ptr<ConversionResultNotification> Ptr;

      virtual void AddSucceed() = 0;
      virtual void AddFailedToOpen(const String& path) = 0;
      virtual void AddFailedToConvert(const String& path, const Error& err) = 0;
    };
    TextResultOperation::Ptr CreateExportOperation(const String& nameTemplate,
      Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result);
    TextResultOperation::Ptr CreateExportOperation(Playlist::Model::IndexSetPtr items,
      const String& nameTemplate, Parameters::Accessor::Ptr params, ConversionResultNotification::Ptr result);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_OPERATIONS_H_DEFINED
