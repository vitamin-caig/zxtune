/**
 *
 * @file
 *
 * @brief Playlist table view interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/model.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QItemDelegate>  // IWYU pragma: export
#include <QtWidgets/QTableView>     // IWYU pragma: export

class QWidget;

namespace Playlist
{
  namespace Item
  {
    class StateCallback;
  }  // namespace Item

  namespace UI
  {
    // table view
    class TableViewItem : public QItemDelegate
    {
      Q_OBJECT
    protected:
      explicit TableViewItem(QWidget& parent);

    public:
      static TableViewItem* Create(QWidget& parent, const Item::StateCallback& callback);
    };

    class TableView : public QTableView
    {
      Q_OBJECT
    protected:
      explicit TableView(QWidget& parent);

    public:
      // creator
      static TableView* Create(QWidget& parent, const Item::StateCallback& callback, QAbstractItemModel& model);

      virtual Model::IndexSet::Ptr GetSelectedItems() const = 0;
      virtual void SelectItems(Playlist::Model::IndexSet::Ptr indices) = 0;
      virtual void MoveToTableRow(unsigned index) = 0;
    signals:
      void TableRowActivated(unsigned index);
    };
  }  // namespace UI
}  // namespace Playlist
