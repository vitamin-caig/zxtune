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

#include "apps/zxtune-qt/playlist/supp/model.h"
#include "apps/zxtune-qt/playlist/supp/operations.h"

#include <QtCore/QString>
#include <QtWidgets/QDialog>  // IWYU pragma: export

#include <memory>

class QWidget;

namespace Playlist::Item::Search
{
  struct Data;
}  // namespace Playlist::Item::Search

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
