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
#include "playlist_view_ui.h"
#include "playlist_view_moc.h"
//common includes
#include <logging.h>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QTableView>

namespace
{
  const std::string THIS_MODULE("UI::PlaylistView");

  //Options
  const char FONT_FAMILY[] = "Arial";
  const int_t FONT_SIZE = 8;
  const int_t ROW_HEIGTH = 16;
  const int_t TITLE_WIDTH = 240;
  const int_t DURATION_WIDTH = 64;

  class PlaylistViewImpl : public PlaylistView
                         , private Ui::PlaylistView
  {
  public:
    PlaylistViewImpl(QWidget* parent, const PlayitemStateCallback& callback, PlaylistModel& model)
      : Callback(callback)
      , Model(model)
      , Table(new QTableView(this))
    {
      //setup self
      setParent(parent);
      setupUi(this);
      setAcceptDrops(true);
      tableView->setItemDelegate(PlaylistItemView::Create(Callback, tableView));
      tableView->setModel(&Model);
      //setup dynamic ui
      if (QHeaderView* const horHeader = tableView->horizontalHeader())
      {
        horHeader->setDefaultAlignment(Qt::AlignLeft);
        horHeader->setTextElideMode(Qt::ElideRight);
        horHeader->resizeSection(PlaylistModel::COLUMN_TITLE, TITLE_WIDTH);
        horHeader->resizeSection(PlaylistModel::COLUMN_DURATION, DURATION_WIDTH);
      }
      if (QHeaderView* const verHeader = tableView->verticalHeader())
      {
        verHeader->setDefaultSectionSize(ROW_HEIGTH);
      }

      //signals
      this->connect(tableView, SIGNAL(activated(const QModelIndex&)), SLOT(ActivateItem(const QModelIndex&)));
    }

    void Update()
    {
      tableView->viewport()->update();
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
      const QItemSelectionModel* const selection = tableView->selectionModel();
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
    QTableView* const Table;
  };

  class PlaylistItemViewImpl : public PlaylistItemView
  {
  public:
    PlaylistItemViewImpl(const PlayitemStateCallback& callback, QWidget* parent)
      : Callback(callback)
      , Regular(QString::fromUtf8(FONT_FAMILY), FONT_SIZE)
      , Playing(Regular)
      , Paused(Regular)
    {
      Playing.setBold(true);
      Paused.setBold(true);
      Paused.setItalic(true);

      setParent(parent);
    }

    virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
      QStyleOptionViewItem fixedOption(option);
      fixedOption.state &= ~QStyle::State_HasFocus;
      fixedOption.font = GetItemFont(index);
      PlaylistItemView::paint(painter, fixedOption, index);
    }
  private:
    QFont GetItemFont(const QModelIndex& index) const
    {
      if (Callback.IsPaused(index))
      {
        return Paused;
      }
      else if (Callback.IsPlaying(index))
      {
        return Playing;
      }
      else
      {
        return Regular;
      }
    }
  private:
    const PlayitemStateCallback& Callback;
    QFont Regular;
    QFont Playing;
    QFont Paused;
  };
}

PlaylistItemView* PlaylistItemView::Create(const PlayitemStateCallback& callback, QWidget* parent)
{
  return new PlaylistItemViewImpl(callback, parent);
}

PlaylistView* PlaylistView::Create(QWidget* parent, const PlayitemStateCallback& callback, PlaylistModel& model)
{
  return new PlaylistViewImpl(parent, callback, model);
}
