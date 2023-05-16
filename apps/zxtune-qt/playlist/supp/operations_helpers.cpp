/**
 *
 * @file
 *
 * @brief Playlist operations helpers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "operations_helpers.h"
#include "storage.h"
// library includes
#include <tools/progress_callback_helpers.h>

namespace
{
  class ProgressModelVisitor : public Playlist::Item::Visitor
  {
  public:
    ProgressModelVisitor(Playlist::Item::Visitor& delegate, Log::ProgressCallback& cb)
      : Delegate(delegate)
      , Callback(cb)
      , Done(0)
    {}

    void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data) override
    {
      Delegate.OnItem(index, data);
      Callback.OnProgress(++Done);
    }

  private:
    Playlist::Item::Visitor& Delegate;
    Log::ProgressCallback& Callback;
    uint_t Done;
  };
}  // namespace

namespace Playlist::Item
{
  void ExecuteOperation(const Storage& stor, Model::IndexSet::Ptr selectedItems, Visitor& visitor,
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
