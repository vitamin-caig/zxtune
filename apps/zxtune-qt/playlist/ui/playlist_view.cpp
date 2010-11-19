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
  const std::string THIS_MODULE("Playlist::UI::View");

  class PlayitemStateCallbackImpl : public Playlist::UI::TableViewStateCallback
  {
  public:
    explicit PlayitemStateCallbackImpl(Playlist::Item::Iterator& iter)
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
               Iterator.GetState() == Playlist::Item::PLAYING;
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
               Iterator.GetState() == Playlist::Item::PAUSED;
      }
      return false;
    }
  private:
    const Playlist::Item::Iterator& Iterator;
  };

  const Qt::KeyboardModifiers DEEPSCAN_KEY = Qt::AltModifier;

  class ViewImpl : public Playlist::UI::View
  {
  public:
    ViewImpl(QWidget& parent, Playlist::Support& playlist)
      : Playlist::UI::View(parent)
      , Controller(playlist)
      , State(Controller.GetIterator())
      , Layout(new QVBoxLayout(this))
      , ScannerView(Playlist::UI::ScannerView::Create(*this, Controller.GetScanner()))
      , View(Playlist::UI::TableView::Create(*this, State, Controller.GetModel()))
    {
      //setup ui
      setAcceptDrops(true);
      Layout->setSpacing(0);
      Layout->setMargin(0);
      Layout->addWidget(View);
      Layout->addWidget(ScannerView);
      //setup connections
      Playlist::Item::Iterator& iter = Controller.GetIterator();
      iter.connect(View, SIGNAL(OnItemActivated(unsigned, const Playitem&)), SLOT(Reset(unsigned)));
      this->connect(&iter, SIGNAL(OnItem(const Playitem&)), SIGNAL(OnItemActivated(const Playitem&)));
      View->connect(&Controller.GetScanner(), SIGNAL(OnScanStop()), SLOT(updateGeometries()));

      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~ViewImpl()
    {
      Controller.deleteLater();
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);
    }

    virtual const Playlist::Support& GetPlaylist() const
    {
      return Controller;
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
        Playlist::Scanner& scanner = Controller.GetScanner();
        //TODO: use key modifiers and change cursor
        const bool deepScan = event->keyboardModifiers() & DEEPSCAN_KEY;
        scanner.AddItems(files, deepScan);
      }
    }
  private:
    Playlist::Support& Controller;
    PlayitemStateCallbackImpl State;
    QVBoxLayout* const Layout;
    Playlist::UI::ScannerView* const ScannerView;
    Playlist::UI::TableView* const View;
  };
}

namespace Playlist
{
  namespace UI
  {
    View::View(QWidget& parent) : QWidget(&parent)
    {
    }

    View* View::Create(QWidget& parent, Playlist::Support& playlist)
    {
      return new ViewImpl(parent, playlist);
    }
  }
}
