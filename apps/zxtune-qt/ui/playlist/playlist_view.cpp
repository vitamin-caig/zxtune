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
//qt includes
#include <QtGui/QHeaderView>
#include <QtGui/QPainter>

namespace
{
  const std::string THIS_MODULE("UI::PlaylistView");

  const int_t ROW_HEIGTH = 16;

  class PlaylistViewImpl : public PlaylistView
  {
  public:
    PlaylistViewImpl(const PlayitemStateCallback& callback, QWidget* parent)
      : Callback(callback)
    {
      setParent(parent);
      setItemDelegate(PlaylistItemView::Create(Callback, this));
      //setup self
      setMouseTracking(true);
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
      QHeaderView* const horHeader = horizontalHeader();
      horHeader->setHighlightSections(false);
      QHeaderView* const verHeader = verticalHeader();
      verHeader->setVisible(false);
      verHeader->setDefaultSectionSize(ROW_HEIGTH);
      verHeader->setHighlightSections(false);
    }
  private:
    const PlayitemStateCallback& Callback;
  };

  class PlaylistItemViewImpl : public PlaylistItemView
  {
  public:
    PlaylistItemViewImpl(const PlayitemStateCallback& callback, QWidget* parent)
      : Callback(callback)
      , Regular(QString::fromUtf8("Arial"), 8)
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

PlaylistView* PlaylistView::Create(const PlayitemStateCallback& callback, QWidget* parent)
{
  return new PlaylistViewImpl(callback, parent);
}

PlaylistItemView* PlaylistItemView::Create(const PlayitemStateCallback& callback, QWidget* parent)
{
  return new PlaylistItemViewImpl(callback, parent);
}
