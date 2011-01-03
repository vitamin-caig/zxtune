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
#include "playlist/playlist_parameters.h"
#include "playlist/supp/model.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/scanner.h"
//local includes
#include <logging.h>
//qt includes
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QMenu>
#include <QtGui/QVBoxLayout>

namespace
{
  const std::string THIS_MODULE("Playlist::UI::View");

  class PlayitemStateCallbackImpl : public Playlist::Item::StateCallback
  {
  public:
    explicit PlayitemStateCallbackImpl(Playlist::Item::Iterator& iter)
      : Iterator(iter)
    {
    }

    virtual Playlist::Item::State GetState(const QModelIndex& index) const
    {
      assert(index.isValid());
      if (const Playlist::Item::Data* const indexItem = static_cast<const Playlist::Item::Data*>(index.internalPointer()))
      {
        const Playlist::Item::Data* operationalItem = Iterator.GetData();
        if (indexItem == operationalItem)
        {
          return Iterator.GetState();
        }
        else if (indexItem->IsValid())
        {
          return Playlist::Item::STOPPED;
        }
      }
      return Playlist::Item::ERROR;
    }
  private:
    const Playlist::Item::Iterator& Iterator;
  };

  class PlaylistOptionsWrapper
  {
  public:
    explicit PlaylistOptionsWrapper(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }

    bool IsDeepScanning() const
    {
      Parameters::IntType val = Parameters::ZXTuneQT::Playlist::DEEP_SCANNING_DEFAULT;
      Params->FindIntValue(Parameters::ZXTuneQT::Playlist::DEEP_SCANNING, val);
      return val != 0;
    }

    unsigned GetPlayorderMode() const
    {
      Parameters::IntType isLooped = Parameters::ZXTuneQT::Playlist::LOOPED_DEFAULT;
      Params->FindIntValue(Parameters::ZXTuneQT::Playlist::LOOPED, isLooped);
      Parameters::IntType isRandom = Parameters::ZXTuneQT::Playlist::RANDOMIZED_DEFAULT;
      Params->FindIntValue(Parameters::ZXTuneQT::Playlist::RANDOMIZED, isRandom);
      return
        (isLooped ? Playlist::Item::LOOPED : 0) |
        (isRandom ? Playlist::Item::RANDOMIZED: 0)
      ;
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  class ViewImpl : public Playlist::UI::View
  {
  public:
    ViewImpl(QWidget& parent, Playlist::Controller::Ptr playlist, Parameters::Accessor::Ptr params)
      : Playlist::UI::View(parent)
      , Controller(playlist)
      , Options(PlaylistOptionsWrapper(params))
      , PlaylistMenu(new QMenu(this))
      , PlayAction(QIcon(":/playback/play.png"), tr("Play"), this)
      , DeleteAction(QIcon(":/application/exit.png"), tr("Delete"), this)
      , State(*Controller->GetIterator())
      , Layout(new QVBoxLayout(this))
      , ScannerView(Playlist::UI::ScannerView::Create(*this, Controller->GetScanner()))
      , View(Playlist::UI::TableView::Create(*this, State, Controller->GetModel()))
    {
      //setup ui
      SetupMenu();
      Layout->setSpacing(0);
      Layout->setMargin(0);
      Layout->addWidget(View);
      Layout->addWidget(ScannerView);
      //setup connections
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      iter->connect(View, SIGNAL(OnItemActivated(unsigned)), SLOT(Reset(unsigned)));
      this->connect(iter, SIGNAL(OnItem(unsigned)), SLOT(ItemActivated(unsigned)));
      View->connect(Controller->GetScanner(), SIGNAL(OnScanStop()), SLOT(updateGeometries()));

      this->connect(&PlayAction, SIGNAL(triggered()), SLOT(PlaySelected()));
      this->connect(&DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));

      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~ViewImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);
    }

    virtual const Playlist::Controller& GetPlaylist() const
    {
      return *Controller;
    }

    //modifiers
    virtual void AddItems(const QStringList& items)
    {
      const Playlist::Scanner::Ptr scanner = Controller->GetScanner();
      const bool deepScan = Options.IsDeepScanning();
      scanner->AddItems(items, deepScan);
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
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      bool hasMoreItems = false;
      const unsigned playorderMode = Options.GetPlayorderMode();
      while (iter->Next(playorderMode) &&
             Playlist::Item::ERROR == iter->GetState())
      {
        hasMoreItems = true;
      }
      if (!hasMoreItems)
      {
        Stop();
      }
    }

    virtual void Next()
    {
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      //skip invalid ones
      const unsigned playorderMode = Options.GetPlayorderMode();
      while (iter->Next(playorderMode) &&
             Playlist::Item::ERROR == iter->GetState())
      {
      }
    }

    virtual void Prev()
    {
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      //skip invalid ones
      const unsigned playorderMode = Options.GetPlayorderMode();
      while (iter->Prev(playorderMode) &&
             Playlist::Item::ERROR == iter->GetState())
      {
      }
    }

    virtual void Clear()
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->Clear();
      Update();
    }

    virtual QMenu* GetPlaylistMenu() const
    {
      return PlaylistMenu;
    }

    virtual void PlaySelected() const
    {
      QSet<unsigned> selected;
      View->GetSelectedItems(selected);
      if (!selected.empty())
      {
        const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
        iter->Reset(*selected.begin());
      }
    }

    virtual void RemoveSelected() const
    {
      QSet<unsigned> selected;
      View->GetSelectedItems(selected);
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->RemoveItems(selected);
    }

    virtual void ItemActivated(unsigned idx)
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      if (Playlist::Item::Data::Ptr item = model->GetItem(idx))
      {
        View->NavigateItem(idx);
        OnItemActivated(*item);
      }
    }

    //qwidget virtuals
    virtual void keyReleaseEvent(QKeyEvent* event)
    {
      const int curKey = event->key();
      if (curKey == Qt::Key_Delete || curKey == Qt::Key_Backspace)
      {
        RemoveSelected();
      }
      else
      {
        QWidget::keyReleaseEvent(event);
      }
    }

    virtual void contextMenuEvent(QContextMenuEvent* event)
    {
      QMenu* const menu = GetPlaylistMenu();
      menu->exec(event->globalPos());
    }
  private:
    void SetupMenu()
    {
      PlaylistMenu->addAction(&PlayAction);
      PlaylistMenu->addAction(&DeleteAction);
    }

    void UpdateState(Playlist::Item::State state)
    {
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      iter->SetState(state);
      Update();
    }

    void Update()
    {
      View->viewport()->update();
    }
  private:
    const Playlist::Controller::Ptr Controller;
    const PlaylistOptionsWrapper Options;
    QMenu* const PlaylistMenu;
    QAction PlayAction;
    QAction DeleteAction;
    //state
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

    View* View::Create(QWidget& parent, Playlist::Controller::Ptr playlist, Parameters::Accessor::Ptr params)
    {
      return new ViewImpl(parent, playlist, params);
    }
  }
}
