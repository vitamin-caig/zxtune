/*
Abstract:
  Async download helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UPDATE_ASYNC_DOWNLOAD_H_DEFINED
#define ZXTUNE_QT_UPDATE_ASYNC_DOWNLOAD_H_DEFINED

//common includes
#include <progress_callback.h>
//library includes
#include <async/activity.h>
#include <binary/data.h>
//qt includes
#include <QtCore/QUrl>

namespace Async
{
  class DownloadCallback : public Log::ProgressCallback
  {
  public:
    virtual void Complete(Binary::Data::Ptr data) = 0;
    virtual void Failed() = 0;
  };

  Activity::Ptr CreateDownloadActivity(const QUrl& url, DownloadCallback& cb);
}

#endif //ZXTUNE_QT_UPDATE_ASYNC_DOWNLOAD_H_DEFINED
