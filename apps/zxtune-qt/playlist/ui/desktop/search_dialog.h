/**
 *
 * @file
 *
 * @brief Playlist search dialog interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "apps/zxtune-qt/playlist/supp/model.h"
#include "apps/zxtune-qt/playlist/supp/operations_search.h"
#include "apps/zxtune-qt/playlist/ui/search.h"
// qt includes
#include <QtWidgets/QDialog>

namespace Playlist::UI
{
  class SearchDialog : public QDialog
  {
    Q_OBJECT
  protected:
    explicit SearchDialog(QWidget& parent);

  public:
    using Ptr = std::shared_ptr<SearchDialog>;
    static Ptr Create(QWidget& parent);

    virtual bool Execute(Playlist::Item::Search::Data& res) = 0;
  };

  Playlist::Item::SelectionOperation::Ptr ExecuteSearchDialog(QWidget& parent,
                                                              const Playlist::Model::IndexSet::Ptr& scope);
}  // namespace Playlist::UI
