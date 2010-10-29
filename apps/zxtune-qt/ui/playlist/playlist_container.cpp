/*
Abstract:
  Playlist container implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist_container.h"
#include "playlist_container_ui.h"
#include "playlist_container_moc.h"
#include "playlist_view.h"
#include "playlist_scanner.h"
#include "playlist.h"
#include "ui/utils.h"
//std includes
#include <cassert>
//qt includes
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>

namespace
{
  const std::string THIS_MODULE("UI::Playlist");

  class PlayitemStateCallbackImpl : public PlayitemStateCallback
  {
    enum PlayitemMode
    {
      STOPPED,
      PLAYING,
      PAUSED
    };
  public:
    PlayitemStateCallbackImpl()
      : OperationalItem(0)
      , OperationalMode(STOPPED)
    {
    }

    virtual bool IsPlaying(const QModelIndex& index) const
    {
      assert(index.isValid());
      const Playitem* const indexItem = static_cast<const Playitem*>(index.internalPointer());
      return OperationalItem &&
             indexItem == OperationalItem &&
             OperationalMode == PLAYING;
    }

    virtual bool IsPaused(const QModelIndex& index) const
    {
      assert(index.isValid());
      const Playitem* const indexItem = static_cast<const Playitem*>(index.internalPointer());
      return OperationalItem &&
             indexItem == OperationalItem &&
             OperationalMode == PAUSED;
    }

  private:
    const Playitem* OperationalItem;
    PlayitemMode OperationalMode;
  };

  class PlaylistContainerImpl : public PlaylistContainer
  {
  public:
    explicit PlaylistContainerImpl(QObject* parent)
      : Provider(PlayitemsProvider::Create())
    {
      //setup self
      setParent(parent);
    }

    virtual Playlist* CreatePlaylist(const QString& name)
    {
      return Playlist::Create(this, name, Provider);
    }

    virtual Playlist* CreatePlaylist(const QString& name, const class QStringList& items)
    {
      Playlist* const res = Playlist::Create(this, name, Provider);
      PlaylistScanner& scanner = res->GetScanner();
      scanner.AddItems(items);
      return res;
    }
  private:
    const PlayitemsProvider::Ptr Provider;
  };

  class PlaylistContainerViewImpl : public PlaylistContainerView
                                  , public Ui::PlaylistContainerView
  {
  public:
    explicit PlaylistContainerViewImpl(QMainWindow* parent)
      : Provider(PlayitemsProvider::Create())
      , Container(PlaylistContainer::Create(this))
    {
      //setup self
      setParent(parent);
      setupUi(this);

      //create and fill menu
      QMenuBar* const menuBar = parent->menuBar();
      QMenu* const menu = menuBar->addMenu(QString::fromUtf8("Playlist"));
      menu->addAction(actionAddFiles);
      //menu->addAction(actionLoad);
      //menu->addAction(actionSave);
      menu->addSeparator();
      menu->addAction(actionClear);
      menu->addSeparator();
      menu->addAction(actionLoop);
      menu->addAction(actionRandom);
    }

    virtual void CreatePlaylist(const QStringList& items)
    {
      Playlist* const pl = Container->CreatePlaylist(tr("Default"), items);
      RegisterPlaylist(*pl);
    }
  private:
    void RegisterPlaylist(Playlist& pl)
    {
      PlaylistWidget* const plView = PlaylistWidget::Create(this, pl, State);
      this->connect(plView, SIGNAL(OnItemSet(const Playitem&)), SIGNAL(OnItemSet(const Playitem&)));
      widgetsContainer->addTab(plView, pl.objectName());
    }
  private:
    PlayitemsProvider::Ptr Provider;
    PlaylistContainer* const Container;
    PlayitemStateCallbackImpl State;
  };
}

PlaylistContainer* PlaylistContainer::Create(QObject* parent)
{
  return new PlaylistContainerImpl(parent);
}

PlaylistContainerView* PlaylistContainerView::Create(QMainWindow* parent)
{
  REGISTER_METATYPE(Playitem::Ptr);
  assert(parent);
  return new PlaylistContainerViewImpl(parent);
}
