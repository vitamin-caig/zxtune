/**
 *
 * @file
 *
 * @brief Playlist operations helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/supp/operations_helpers.h"

#include "apps/zxtune-qt/playlist/supp/data.h"
#include "apps/zxtune-qt/playlist/supp/storage.h"

#include "tools/progress_callback.h"
#include "tools/progress_callback_helpers.h"

#include "types.h"

#include <memory>

namespace
{
  class ProgressModelVisitor : public Playlist::Item::Visitor
  {
  public:
    ProgressModelVisitor(Playlist::Item::Visitor& delegate, Log::ProgressCallback& cb)
      : Delegate(delegate)
      , Callback(cb)
    {}

    void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data) override
    {
      Delegate.OnItem(index, data);
      Callback.OnProgress(++Done);
    }

  private:
    Playlist::Item::Visitor& Delegate;
    Log::ProgressCallback& Callback;
    uint_t Done = 0;
  };
}  // namespace

namespace Playlist::Item
{
  void ExecuteOperation(const Storage& stor, const Model::IndexSet::Ptr& selectedItems, Visitor& visitor,
                        Log::ProgressCallback& cb)
  {
    const std::size_t totalItems = selectedItems ? selectedItems->size() : stor.CountItems();
    Log::PercentProgressCallback progress(static_cast<uint_t>(totalItems), cb);
    ProgressModelVisitor progressed(visitor, progress);
    if (selectedItems)
    {
      stor.ForSpecifiedItems(*selectedItems, progressed);
    }
    else
    {
      stor.ForAllItems(progressed);
    }
  }
}  // namespace Playlist::Item
