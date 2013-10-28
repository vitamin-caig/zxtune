/*
Abstract:
  Downloads visitor adapter

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UPDATE_DOWNLOADS_H_DEFINED
#define ZXTUNE_QT_UPDATE_DOWNLOADS_H_DEFINED

//local includes
#include "product.h"
#include "rss.h"
//std includes
#include <memory>

namespace Downloads
{
  class Visitor
  {
  public:
    virtual ~Visitor() {}

    virtual void OnDownload(Product::Update::Ptr update) = 0;
  };

  std::auto_ptr<RSS::Visitor> CreateFeedVisitor(const QString& project, Visitor& delegate);
}

#endif //ZXTUNE_QT_UPDATE_DOWNLOADS_H_DEFINED
