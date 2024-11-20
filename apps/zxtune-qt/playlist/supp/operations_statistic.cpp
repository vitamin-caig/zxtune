/**
 *
 * @file
 *
 * @brief Playlist statistic operations implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/supp/operations_statistic.h"

#include "apps/zxtune-qt/playlist/supp/data.h"
#include "apps/zxtune-qt/playlist/supp/operations_helpers.h"
#include "apps/zxtune-qt/playlist/supp/storage.h"

#include "make_ptr.h"
#include "string_type.h"

#include <cassert>
#include <utility>

namespace Log
{
  class ProgressCallback;
}  // namespace Log

namespace
{
  class CollectStatisticOperation
    : public Playlist::Item::TextResultOperation
    , private Playlist::Item::Visitor
  {
  public:
    explicit CollectStatisticOperation(Playlist::Item::StatisticTextNotification::Ptr result)
      : SelectedItems()
      , Result(std::move(result))
    {}

    CollectStatisticOperation(Playlist::Model::IndexSet::Ptr items,
                              Playlist::Item::StatisticTextNotification::Ptr result)
      : SelectedItems(std::move(items))
      , Result(std::move(result))
    {}

    void Execute(const Playlist::Item::Storage& stor, Log::ProgressCallback& cb) override
    {
      ExecuteOperation(stor, SelectedItems, *this, cb);
      emit ResultAcquired(Result);
    }

  private:
    void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data) override
    {
      data->GetModule();
      // check for the data first to define is data valid or not
      const String type = data->GetType();
      if (data->GetState().GetIfError())
      {
        Result->AddInvalid();
      }
      else
      {
        assert(!type.empty());
        Result->AddValid();
        Result->AddType(type);
        Result->AddDuration(data->GetDuration());
        Result->AddSize(data->GetSize());
        Result->AddPath(data->GetFilePath());
      }
    }

  private:
    const Playlist::Model::IndexSet::Ptr SelectedItems;
    const Playlist::Item::StatisticTextNotification::Ptr Result;
  };
}  // namespace

namespace Playlist::Item
{
  TextResultOperation::Ptr CreateCollectStatisticOperation(StatisticTextNotification::Ptr result)
  {
    return MakePtr<CollectStatisticOperation>(std::move(result));
  }

  TextResultOperation::Ptr CreateCollectStatisticOperation(Playlist::Model::IndexSet::Ptr items,
                                                           StatisticTextNotification::Ptr result)
  {
    return MakePtr<CollectStatisticOperation>(std::move(items), std::move(result));
  }
}  // namespace Playlist::Item
