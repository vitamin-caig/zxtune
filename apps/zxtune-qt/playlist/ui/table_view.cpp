/**
 *
 * @file
 *
 * @brief Playlist table view implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/ui/table_view.h"

#include "apps/zxtune-qt/playlist/supp/controller.h"
#include "apps/zxtune-qt/playlist/supp/model.h"
#include "apps/zxtune-qt/ui/utils.h"

#include "debug/log.h"

#include "contract.h"
#include "make_ptr.h"
#include "string_view.h"
#include "types.h"

#include <QtCore/QFlags>
#include <QtCore/QItemSelection>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QModelIndex>
#include <QtCore/QModelIndexList>
#include <QtCore/QVariant>
#include <QtCore/QtCore>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QFontMetrics>
#include <QtGui/QPalette>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QAction>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionViewItem>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>
#include <memory>

class QBrush;
class QPainter;

namespace
{
  const Debug::Stream Dbg("Playlist::UI::TableView");

  // Options
  const QLatin1String TYPE_TEXT("WWWW");
  const int_t DISPLAYNAME_WIDTH = 320;
  const QLatin1String DURATION_TEXT("77:77.77");
  const int_t AUTHOR_WIDTH = 160;
  const int_t TITLE_WIDTH = 160;
  const int_t COMMENT_WIDTH = 160;
  const int_t PATH_WIDTH = 320;
  const QLatin1String SIZE_TEXT("7777777");
  const QLatin1String CRC_TEXT("AAAAAAAA");

  // WARNING!!! change object name when adding new column
  class TableHeader : public QHeaderView
  {
  public:
    explicit TableHeader(QWidget& parent)
      : QHeaderView(Qt::Horizontal, &parent)
    {
      setObjectName(QLatin1String("Columns_v2"));
      setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      setHighlightSections(false);
      setTextElideMode(Qt::ElideRight);
      setSectionsMovable(true);
      setSectionsClickable(true);
      const QFontMetrics fontMetrics(font());
      resizeSection(Playlist::Model::COLUMN_TYPE, fontMetrics.horizontalAdvance(TYPE_TEXT));
      resizeSection(Playlist::Model::COLUMN_DISPLAY_NAME, DISPLAYNAME_WIDTH);
      resizeSection(Playlist::Model::COLUMN_DURATION, fontMetrics.horizontalAdvance(DURATION_TEXT));
      resizeSection(Playlist::Model::COLUMN_AUTHOR, AUTHOR_WIDTH);
      resizeSection(Playlist::Model::COLUMN_TITLE, TITLE_WIDTH);
      resizeSection(Playlist::Model::COLUMN_COMMENT, COMMENT_WIDTH);
      resizeSection(Playlist::Model::COLUMN_PATH, PATH_WIDTH);
      resizeSection(Playlist::Model::COLUMN_SIZE, fontMetrics.horizontalAdvance(SIZE_TEXT));
      resizeSection(Playlist::Model::COLUMN_CRC, fontMetrics.horizontalAdvance(CRC_TEXT));
      resizeSection(Playlist::Model::COLUMN_FIXEDCRC, fontMetrics.horizontalAdvance(CRC_TEXT));

      // default view
      for (int idx = Playlist::Model::COLUMN_AUTHOR; idx != Playlist::Model::COLUMNS_COUNT; ++idx)
      {
        hideSection(idx);
      }
    }

    // QWidget's virtuals
    void contextMenuEvent(QContextMenuEvent* event) override
    {
      const std::unique_ptr<QMenu> menu = CreateMenu();
      if (QAction* res = menu->exec(event->globalPos()))
      {
        const QVariant data = res->data();
        const int section = data.toInt();
        setSectionHidden(section, !isSectionHidden(section));
      }
      event->accept();
    }

  private:
    std::unique_ptr<QMenu> CreateMenu()
    {
      std::unique_ptr<QMenu> result(new QMenu(this));
      QAbstractItemModel* const md = model();
      for (int idx = 0, lim = count(); idx != lim; ++idx)
      {
        const QVariant text = md->headerData(idx, Qt::Horizontal);
        QAction* const action = result->addAction(text.toString());
        action->setCheckable(true);
        action->setChecked(!isSectionHidden(idx));
        action->setData(QVariant(idx));
      }
      return result;
    }
  };

  class TableViewImpl : public Playlist::UI::TableView
  {
  public:
    TableViewImpl(QWidget& parent, const Playlist::Item::StateCallback& callback, QAbstractItemModel& model)
      : Playlist::UI::TableView(parent)
    {
      // setup self
      setSortingEnabled(true);
      setItemDelegate(Playlist::UI::TableViewItem::Create(*this, callback));
      setMinimumSize(256, 128);
      // setup ui
      setAcceptDrops(true);
      setDragEnabled(true);
      setDragDropMode(QAbstractItemView::InternalMove);
      setDropIndicatorShown(true);
      setEditTriggers(QAbstractItemView::NoEditTriggers);
      setSelectionMode(QAbstractItemView::ExtendedSelection);
      setSelectionBehavior(QAbstractItemView::SelectRows);
      setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
      setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
      setShowGrid(false);
      setGridStyle(Qt::NoPen);
      setWordWrap(false);
      setCornerButtonEnabled(false);

      // setup dynamic ui
      setHorizontalHeader(new TableHeader(*this));
      if (QHeaderView* const verHeader = verticalHeader())
      {
        verHeader->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);
        verHeader->setSectionResizeMode(QHeaderView::Fixed);
      }
      setModel(&model);

      // signals
      Require(connect(this, &QTableView::activated, this, &TableViewImpl::ActivateItem));

      Dbg("Created at {}", Self());
    }

    ~TableViewImpl() override
    {
      Dbg("Destroyed at {}", Self());
    }

    Playlist::Model::IndexSet::Ptr GetSelectedItems() const override
    {
      const QItemSelectionModel* const selection = selectionModel();
      const QModelIndexList& items = selection->selectedRows();
      const Playlist::Model::IndexSet::RWPtr result = MakeRWPtr<Playlist::Model::IndexSet>();
      for (const auto& item : items)
      {
        result->insert(item.row());
      }
      return result;
    }

    void SelectItems(Playlist::Model::IndexSet::Ptr indices) override
    {
      setEnabled(true);
      QAbstractItemModel* const curModel = model();
      QItemSelectionModel* const selectModel = selectionModel();
      QItemSelection selection;
      for (auto index : *indices)
      {
        const QModelIndex left = curModel->index(index, 0);
        const QModelIndex right = curModel->index(index, Playlist::Model::COLUMNS_COUNT - 1);
        const QItemSelection sel(left, right);
        selection.merge(sel, QItemSelectionModel::Select);
      }
      selectModel->select(selection, QItemSelectionModel::ClearAndSelect);
      if (!indices->empty())
      {
        MoveToTableRow(*indices->rbegin());
      }
    }

    void MoveToTableRow(unsigned index) override
    {
      QAbstractItemModel* const curModel = model();
      const QModelIndex idx = curModel->index(index, 0);
      scrollTo(idx, QAbstractItemView::EnsureVisible);
    }

    void ActivateItem(const QModelIndex& index)
    {
      if (index.isValid())
      {
        const unsigned number = index.row();
        emit TableRowActivated(number);
      }
    }

    // Qt natives
    void keyboardSearch(const QString& search) override
    {
      QAbstractItemView::keyboardSearch(search);
      const QItemSelectionModel* const selection = selectionModel();
      if (selection->hasSelection())
      {
        // selection->currentIndex() does not work
        // items are orderen by selection, not by position
        QModelIndexList items = selection->selectedRows();
        scrollTo(*std::min_element(items.begin(), items.end()), QAbstractItemView::EnsureVisible);
      }
    }

  private:
    const void* Self() const
    {
      return this;
    }
  };

  class TableViewItemImpl : public Playlist::UI::TableViewItem
  {
  public:
    TableViewItemImpl(QWidget& parent, const Playlist::Item::StateCallback& callback)
      : Playlist::UI::TableViewItem(parent)
      , Callback(callback)
    {}

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
      QStyleOptionViewItem fixedOption(option);
      FillItemStyle(index, fixedOption);
      Playlist::UI::TableViewItem::paint(painter, fixedOption, index);
    }

  private:
    void FillItemStyle(const QModelIndex& index, QStyleOptionViewItem& style) const
    {
      // disable focus to remove per-cell dotted border
      style.state &= ~QStyle::State_HasFocus;
      const Playlist::Item::State state = Callback.GetState(index);
      if (state == Playlist::Item::STOPPED)
      {
        return;
      }
      const QBrush& textColor = Palette.text();
      const QBrush& baseColor = Palette.base();
      const QBrush& highlightColor = Palette.highlight();
      const QBrush& disabledColor = Palette.mid();

      QPalette& palette = style.palette;
      const bool isSelected = 0 != (style.state & QStyle::State_Selected);
      // force selection to apply only 2 brushes instead of 3
      style.state |= QStyle::State_Selected;

      switch (state)
      {
      case Playlist::Item::PLAYING:
      {
        // invert, propagate selection to text
        const QBrush& itemText = isSelected ? highlightColor : baseColor;
        const QBrush& itemBack = textColor;
        palette.setBrush(QPalette::HighlightedText, itemText);
        palette.setBrush(QPalette::Highlight, itemBack);
      }
      break;
      case Playlist::Item::PAUSED:
      {
        // disable bg, propagate selection to text
        const QBrush& itemText = isSelected ? highlightColor : baseColor;
        const QBrush& itemBack = disabledColor;
        palette.setBrush(QPalette::HighlightedText, itemText);
        palette.setBrush(QPalette::Highlight, itemBack);
      }
      break;
      case Playlist::Item::ERROR:
      {
        // disable text
        const QBrush& itemText = disabledColor;
        const QBrush& itemBack = isSelected ? highlightColor : baseColor;
        palette.setBrush(QPalette::HighlightedText, itemText);
        palette.setBrush(QPalette::Highlight, itemBack);
      }
      break;
      default:
        assert(!"Invalid playitem state");
      }
    }

  private:
    const Playlist::Item::StateCallback& Callback;
    QPalette Palette;
  };
}  // namespace

namespace Playlist::UI
{
  TableViewItem::TableViewItem(QWidget& parent)
    : QItemDelegate(&parent)
  {}

  TableViewItem* TableViewItem::Create(QWidget& parent, const Item::StateCallback& callback)
  {
    return new TableViewItemImpl(parent, callback);
  }

  TableView::TableView(QWidget& parent)
    : QTableView(&parent)
  {}

  TableView* TableView::Create(QWidget& parent, const Item::StateCallback& callback, QAbstractItemModel& model)
  {
    REGISTER_METATYPE(Playlist::Model::IndexSet::Ptr);
    return new TableViewImpl(parent, callback, model);
  }
}  // namespace Playlist::UI
