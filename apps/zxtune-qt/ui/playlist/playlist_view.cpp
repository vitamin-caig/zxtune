/*
Abstract:
  Playlist view implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist_model.h"
#include "playlist_view.h"
#include "playlist_view_moc.h"
//common includes
#include <logging.h>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QHeaderView>

namespace
{
  const std::string THIS_MODULE("UI::PlaylistView");

  //Options
  const char FONT_FAMILY[] = "Arial";
  const int_t FONT_SIZE = 8;
  const int_t ROW_HEIGTH = 16;
  const int_t ICON_WIDTH = 24;
  const int_t TITLE_WIDTH = 320;
  const int_t DURATION_WIDTH = 60;

  class PlaylistTableViewImpl : public PlaylistTableView
  {
  public:
    PlaylistTableViewImpl(QWidget* parent, const PlayitemStateCallback& callback, PlaylistModel& model)
      : Callback(callback)
      , Model(model)
      , Font(QString::fromUtf8(FONT_FAMILY), FONT_SIZE)
    {
      //setup self
      setParent(parent);
      setModel(&Model);
      setItemDelegate(PlaylistItemTableView::Create(this, Callback));
      setFont(Font);
      //setup ui
      setAcceptDrops(true);
      setEditTriggers(QAbstractItemView::NoEditTriggers);
      setSelectionMode(QAbstractItemView::ExtendedSelection);
      setSelectionBehavior(QAbstractItemView::SelectRows);
      setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
      setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
      setShowGrid(false);
      setGridStyle(Qt::NoPen);
      setSortingEnabled(true);
      setWordWrap(false);
      setCornerButtonEnabled(false);
      //setup dynamic ui
      if (QHeaderView* const horHeader = horizontalHeader())
      {
        horHeader->setFont(Font);
        horHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        horHeader->setHighlightSections(false);
        horHeader->setTextElideMode(Qt::ElideRight);
        horHeader->resizeSection(PlaylistModel::COLUMN_TYPEICON, ICON_WIDTH);
        horHeader->resizeSection(PlaylistModel::COLUMN_TITLE, TITLE_WIDTH);
        horHeader->resizeSection(PlaylistModel::COLUMN_DURATION, DURATION_WIDTH);
      }
      if (QHeaderView* const verHeader = verticalHeader())
      {
        verHeader->setFont(Font);
        verHeader->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);
        verHeader->setDefaultSectionSize(ROW_HEIGTH);
      }

      //signals
      this->connect(this, SIGNAL(activated(const QModelIndex&)), SLOT(ActivateItem(const QModelIndex&)));
    }

    virtual void ActivateItem(const QModelIndex& index)
    {
      const unsigned number = index.row();
      if (const Playitem::Ptr item = Model.GetItem(number))
      {
        OnItemActivated(number, *item);
      }
    }

    //QWidget virtuals
    virtual void keyReleaseEvent(QKeyEvent* event)
    {
      const int curKey = event->key();
      if (curKey == Qt::Key_Delete || curKey == Qt::Key_Backspace)
      {
        ClearSelected();
      }
      else
      {
        QWidget::keyReleaseEvent(event);
      }
    }
  private:
    void ClearSelected()
    {
      const QItemSelectionModel* const selection = selectionModel();
      const QModelIndexList& items = selection->selectedRows();
      QSet<unsigned> indexes;
      std::for_each(items.begin(), items.end(),
        boost::bind(&QSet<unsigned>::insert, &indexes, 
          boost::bind(&QModelIndex::row, _1)));
      Model.RemoveItems(indexes);
    }
  private:
    const PlayitemStateCallback& Callback;
    PlaylistModel& Model;
    QFont Font;
  };

  class PlaylistItemTableViewImpl : public PlaylistItemTableView
  {
  public:
    PlaylistItemTableViewImpl(QWidget* parent, const PlayitemStateCallback& callback)
      : Callback(callback)
      , Palette()
    {
      setParent(parent);
    }

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
      QStyleOptionViewItem fixedOption(option);
      FillItemStyle(index, fixedOption);
      PlaylistItemTableView::paint(painter, fixedOption, index);
    }
  private:
    void FillItemStyle(const QModelIndex& index, QStyleOptionViewItem& style) const
    {
      style.state &= ~QStyle::State_HasFocus;
      const bool isSelected = 0 != (style.state & QStyle::State_Selected);
      const bool isPaused = Callback.IsPaused(index);
      const bool isPlaying = Callback.IsPlaying(index);
      if (isPaused || isPlaying)
      {
        style.state |= QStyle::State_Selected;
        const QBrush& bgBrush = isPaused ? Palette.mid() : Palette.text();
        const QBrush& txtBrush = isSelected ? Palette.highlight() : Palette.base();
        QPalette& palette = style.palette;
        palette.setBrush(QPalette::Highlight, bgBrush);
        palette.setBrush(QPalette::HighlightedText, txtBrush);
      }
    }
  private:
    const PlayitemStateCallback& Callback;
    QPalette Palette;
  };
}

PlaylistItemTableView* PlaylistItemTableView::Create(QWidget* parent, const PlayitemStateCallback& callback)
{
  return new PlaylistItemTableViewImpl(parent, callback);
}

PlaylistTableView* PlaylistTableView::Create(QWidget* parent, const PlayitemStateCallback& callback, PlaylistModel& model)
{
  return new PlaylistTableViewImpl(parent, callback, model);
}
