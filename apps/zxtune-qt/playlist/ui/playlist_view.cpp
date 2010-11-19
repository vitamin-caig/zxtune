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
#include "playlist/supp/model.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/scanner.h"
//local includes
#include <logging.h>
//qt includes
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

  class ViewImpl : public Playlist::UI::View
  {
  public:
    ViewImpl(QWidget& parent, Playlist::Controller& playlist)
      : Playlist::UI::View(parent)
      , Controller(playlist)
      , State(Controller.GetIterator())
      , Layout(new QVBoxLayout(this))
      , ScannerView(Playlist::UI::ScannerView::Create(*this, Controller.GetScanner()))
      , View(Playlist::UI::TableView::Create(*this, State, Controller.GetModel()))
    {
      //setup ui
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

    virtual const Playlist::Controller& GetPlaylist() const
    {
      return Controller;
    }

    //modifiers
    virtual void AddItems(const QStringList& items, bool deepScan)
    {
      Playlist::Scanner& scanner = Controller.GetScanner();
      scanner.AddItems(items, deepScan);
    }

    virtual void Play()
    {
      UpdateState(Playlist::Item::PLAYING);
    }

    virtual void Pause()
    {
      UpdateState(Playlist::Item::PAUSED);
    }

    virtual void Stop()
    {
      UpdateState(Playlist::Item::STOPPED);
    }

    virtual void Finish()
    {
      Playlist::Item::Iterator& iter = Controller.GetIterator();
      if (!iter.Next())
      {
        Stop();
      }
    }

    virtual void Next()
    {
      Playlist::Item::Iterator& iter = Controller.GetIterator();
      iter.Next();
    }

    virtual void Prev()
    {
      Playlist::Item::Iterator& iter = Controller.GetIterator();
      iter.Prev();
    }

    virtual void Clear()
    {
      Playlist::Model& model = Controller.GetModel();
      model.Clear();
      Update();
    }
  private:
    void UpdateState(Playlist::Item::State state)
    {
      Playlist::Item::Iterator& iter = Controller.GetIterator();
      iter.SetState(state);
      Update();
    }

    void Update()
    {
      View->viewport()->update();
    }
  private:
    Playlist::Controller& Controller;
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

    View* View::Create(QWidget& parent, Playlist::Controller& playlist)
    {
      return new ViewImpl(parent, playlist);
    }
  }
}
