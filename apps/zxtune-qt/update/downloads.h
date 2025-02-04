/**
 *
 * @file
 *
 * @brief Downloads visitor adapter factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/update/product.h"
#include "apps/zxtune-qt/update/rss.h"

#include <memory>

namespace Downloads
{
  class Visitor
  {
  public:
    virtual ~Visitor() = default;

    virtual void OnDownload(Product::Update::Ptr update) = 0;
  };

  std::unique_ptr<RSS::Visitor> CreateFeedVisitor(const QString& project, Visitor& delegate);
}  // namespace Downloads
