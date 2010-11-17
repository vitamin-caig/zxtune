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
#include "scanner_view.h"
#include "table_view.h"
#include "playlist_view.h"
#include "playlist/supp/playlist.h"
#include "playlist/supp/scanner.h"
//local includes
#include <logging.h>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QVBoxLayout>

namespace
{
  const std::string THIS_MODULE("Playlist::View");

  class PlayitemStateCallbackImpl : public PlayitemStateCallback
  {
  public:
    explicit PlayitemStateCallbackImpl(PlayitemIterator& iter)
      : Iterator(iter)
    {
    }

    virtual bool IsPlaying(const QModelIndex& index) const
    {
      assert(index.isValid());
      if (const Playitem* operationalItem = Iterator.Get())
      {
        const Playitem* const indexItem = static_cast<const Playitem*>(index.internalPointer());
        return indexItem == operationalItem &&
               Iterator.GetState() == PLAYING;
      }
      return false;
    }

    virtual bool IsPaused(const QModelIndex& index) const
    {
      assert(index.isValid());
      if (const Playitem* operationalItem = Iterator.Get())
      {
        const Playitem* const indexItem = static_cast<const Playitem*>(index.internalPointer());
        return indexItem == operationalItem &&
               Iterator.GetState() == PAUSED;
      }
      return false;
    }
  private:
    const PlayitemIterator& Iterator;
  };

  const Qt::KeyboardModifiers DEEPSCAN_KEY = Qt::AltModifier;

  class PlaylistViewImpl : public PlaylistView
  {
  public:
    PlaylistViewImpl(QWidget& parent, const PlaylistSupport& playlist)
      : PlaylistView(parent)
      , Playlist(playlist)
      , State(Playlist.GetIterator())
      , Layout(new QVBoxLayout(this))
      , ScannerView(PlaylistScannerView::Create(*this, Playlist.GetScanner()))
      , View(PlaylistTableView::Create(*this, State, Playlist.GetModel()))
    {
      //setup ui
      setAcceptDrops(true);
      Layout->setSpacing(0);
      Layout->setMargin(0);
      Layout->addWidget(View);
      Layout->addWidget(ScannerView);
      //setup connections
      PlayitemIterator& iter = Playlist.GetIterator();
      iter.connect(View, SIGNAL(OnItemActivated(unsigned, const Playitem&)), SLOT(Reset(unsigned)));
      View->connect(&Playlist.GetScanner(), SIGNAL(OnScanStop()), SLOT(updateGeometries()));

      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~PlaylistViewImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);
    }

    virtual const PlaylistSupport& GetPlaylist() const
    {
      return Playlist;
    }

    virtual void Update()
    {
      View->viewport()->update();
    }

    virtual void dragEnterEvent(QDragEnterEvent* event)
    {
      const bool deepScan = event->keyboardModifiers() & DEEPSCAN_KEY;
      event->setDropAction(deepScan ? Qt::CopyAction : Qt::MoveAction);
      event->acceptProposedAction();
    }

    virtual void dropEvent(QDropEvent* event)
    {
      if (event->mimeData()->hasUrls())
      {
        const QList<QUrl>& urls = event->mimeData()->urls();
        QStringList files;
        std::for_each(urls.begin(), urls.end(),
          boost::bind(&QStringList::push_back, &files,
            boost::bind(&QUrl::toLocalFile, _1)));
        PlaylistScanner& scanner = Playlist.GetScanner();
        //TODO: use key modifiers and change cursor
        const bool deepScan = event->keyboardModifiers() & DEEPSCAN_KEY;
        scanner.AddItems(files, deepScan);
      }
    }
  private:
    const PlaylistSupport& Playlist;
    PlayitemStateCallbackImpl State;
    QVBoxLayout* const Layout;
    PlaylistScannerView* const ScannerView;
    PlaylistTableView* const View;
  };
}

PlaylistView::PlaylistView(QWidget& parent) : QWidget(&parent)
{
}

PlaylistView* PlaylistView::Create(QWidget& parent, const PlaylistSupport& playlist)
{
  return new PlaylistViewImpl(parent, playlist);
}
