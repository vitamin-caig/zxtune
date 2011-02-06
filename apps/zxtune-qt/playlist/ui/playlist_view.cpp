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
#include "playlist_contextmenu.h"
#include "playlist_view.h"
#include "playlist/playlist_parameters.h"
#include "playlist/supp/model.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/scanner.h"
//local includes
#include <logging.h>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
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
      , State(*Controller->GetIterator())
      , Layout(new QVBoxLayout(this))
      , ScannerView(Playlist::UI::ScannerView::Create(*this, Controller->GetScanner()))
      , View(Playlist::UI::TableView::Create(*this, State, Controller->GetModel()))
      , PlaylistMenu(Playlist::UI::ContextMenu::Create(*View, playlist))
    {
      //setup ui
      setAcceptDrops(true);
      Layout->setSpacing(0);
      Layout->setMargin(0);
      Layout->addWidget(View);
      Layout->addWidget(ScannerView);
      //setup connections
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      iter->connect(View, SIGNAL(OnItemActivated(unsigned)), SLOT(Reset(unsigned)));
      this->connect(iter, SIGNAL(OnItem(unsigned)), SLOT(ItemActivated(unsigned)));
      View->connect(Controller->GetScanner(), SIGNAL(OnScanStop()), SLOT(updateGeometries()));

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
        PlaylistMenu->RemoveSelected();
      }
      else
      {
        QWidget::keyReleaseEvent(event);
      }
    }

    virtual void contextMenuEvent(QContextMenuEvent* event)
    {
      PlaylistMenu->exec(event->globalPos());
    }

    virtual void dragEnterEvent(QDragEnterEvent* event)
    {
      event->acceptProposedAction();
    }

    virtual void dropEvent(QDropEvent* event)
    {
      const QMimeData* const mimeData = event->mimeData();
      if (mimeData && mimeData->hasUrls())
      {
        const QList<QUrl>& urls = mimeData->urls();
        QStringList files;
        std::for_each(urls.begin(), urls.end(),
          boost::bind(&QStringList::push_back, &files,
            boost::bind(&QUrl::toLocalFile, _1)));
        AddItems(files);
      }
    }
  private:
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
    //state
    PlayitemStateCallbackImpl State;
    QVBoxLayout* const Layout;
    Playlist::UI::ScannerView* const ScannerView;
    Playlist::UI::TableView* const View;
    Playlist::UI::ContextMenu* const PlaylistMenu;
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
