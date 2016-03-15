/**
* 
* @file
*
* @brief Playlist operations helpers implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "operations_helpers.h"
#include "storage.h"

namespace
{
  class ProgressModelVisitor : public Playlist::Item::Visitor
  {
  public:
    ProgressModelVisitor(Playlist::Item::Visitor& delegate, Log::ProgressCallback& cb)
      : Delegate(delegate)
      , Callback(cb)
      , Done(0)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data)
    {
      Delegate.OnItem(index, data);
      Callback.OnProgress(++Done);
    }
  private:
    Playlist::Item::Visitor& Delegate;
    Log::ProgressCallback& Callback;
    uint_t Done;
  };
}

namespace Playlist
{
  namespace Item
  {
    void ExecuteOperation(const Storage& stor, Model::IndexSet::Ptr selectedItems, Visitor& visitor, Log::ProgressCallback& cb)
    {
      const std::size_t totalItems = selectedItems ? selectedItems->size() : stor.CountItems();
      const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(static_cast<uint_t>(totalItems), cb);
      ProgressModelVisitor progressed(visitor, *progress);
      if (selectedItems)
      {
        stor.ForSpecifiedItems(*selectedItems, progressed);
      }
      else
      {
        stor.ForAllItems(progressed);
      }
    }
  }
}
