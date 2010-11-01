/*
Abstract:
  Playlist entity and view implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist.h"
#include "playlist_moc.h"
#include "playlist_model.h"
#include "playlist_view.h"
#include "playlist_scanner.h"
#include "playlist_scanner_view.h"
//std includes
#include <cassert>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QVBoxLayout>
//text includes
#include "text/text.h"

namespace
{
  class PlayitemIteratorImpl : public PlayitemIterator
  {
  public:
    PlayitemIteratorImpl(QObject* parent, const PlaylistModel& model)
      : Model(model)
      , Index(0)
      , Item(Model.GetItem(Index))
      , State(STOPPED)
    {
      setParent(parent);
    }

    virtual const Playitem* Get() const
    {
      return Item.get();
    }

    virtual PlayitemState GetState() const
    {
      return State;
    }

    virtual bool Reset(unsigned idx)
    {
      if (Playitem::Ptr item = Model.GetItem(idx))
      {
        Index = idx;
        Item = item;
        State = STOPPED;
        OnItem(*Item);
        return true;
      }
      return false;
    }

    virtual bool Next()
    {
      return Reset(Index + 1);
    }

    virtual bool Prev()
    {
      return Index &&
             Reset(Index - 1);
    }

    virtual void SetState(PlayitemState state)
    {
      State = state;
    }
  private:
    const PlaylistModel& Model;
    unsigned Index;
    Playitem::Ptr Item;
    PlayitemState State;
  };

  class PlaylistSupportImpl : public PlaylistSupport
  {
  public:
    PlaylistSupportImpl(QObject* parent, const QString& name, PlayitemsProvider::Ptr provider)
      : Scanner(PlaylistScanner::Create(this, provider))
      , Model(PlaylistModel::Create(this))
      , Iterator(new PlayitemIteratorImpl(this, *Model))
    {
      //setup self
      setParent(parent);
      setObjectName(name);
      //setup connections
      Model->connect(Scanner, SIGNAL(OnGetItem(Playitem::Ptr)), SLOT(AddItem(Playitem::Ptr)));
    }

    virtual ~PlaylistSupportImpl()
    {
      Scanner->Cancel();
      Scanner->wait();
    }

    virtual class PlaylistScanner& GetScanner() const
    {
      return *Scanner;
    }

    virtual class PlaylistModel& GetModel() const
    {
      return *Model;
    }

    virtual PlayitemIterator& GetIterator() const
    {
      return *Iterator;
    }

  private:
    PlayitemsProvider::Ptr Provider;
    PlaylistScanner* const Scanner;
    PlaylistModel* const Model;
    PlayitemIterator* const Iterator;
  };

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
    PlaylistViewImpl(QWidget* parent, const PlaylistSupport& playlist)
      : Playlist(playlist)
      , State(Playlist.GetIterator())
      , Layout(new QVBoxLayout(this))
      , ScannerView(PlaylistScannerView::Create(this, Playlist.GetScanner()))
      , View(PlaylistTableView::Create(this, State, Playlist.GetModel()))
    {
      //setup self
      setParent(parent);
      //setup ui
      setAcceptDrops(true);
      Layout->setSpacing(0);
      Layout->setMargin(0);
      Layout->addWidget(View);
      Layout->addWidget(ScannerView);
      //setup connections
      PlayitemIterator& iter = Playlist.GetIterator();
      iter.connect(View, SIGNAL(OnItemActivated(unsigned, const Playitem&)), SLOT(Reset(unsigned)));
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

PlaylistSupport* PlaylistSupport::Create(QObject* parent, const QString& name, PlayitemsProvider::Ptr provider)
{
  return new PlaylistSupportImpl(parent, name, provider);
}

PlaylistView* PlaylistView::Create(QWidget* parent, const PlaylistSupport& playlist)
{
  return new PlaylistViewImpl(parent, playlist);
}
