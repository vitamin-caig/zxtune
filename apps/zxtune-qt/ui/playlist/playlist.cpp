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
  class PlaylistImpl : public Playlist
  {
  public:
    PlaylistImpl(QObject* parent, const QString& name, PlayitemsProvider::Ptr provider)
      : Scanner(PlaylistScanner::Create(this, provider))
      , Model(PlaylistModel::Create(this))
    {
      //setup self
      setParent(parent);
      //setup connections
      Model->connect(Scanner, SIGNAL(OnGetItem(Playitem::Ptr)), SLOT(AddItem(Playitem::Ptr)));
    }

    virtual ~PlaylistImpl()
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

  private:
    PlayitemsProvider::Ptr Provider;
    PlaylistScanner* const Scanner;
    PlaylistModel* const Model;
  };

  class PlaylistWidgetImpl : public PlaylistWidget
  {
  public:
    PlaylistWidgetImpl(QWidget* parent, const Playlist& playlist, const PlayitemStateCallback& callback)
      : Scanner(playlist.GetScanner())
      , Layout(new QVBoxLayout(this))
      , ScannerView(PlaylistScannerView::Create(this, Scanner))
      , View(PlaylistView::Create(this, callback, playlist.GetModel()))
    {
      //setup self
      setParent(parent);
      //setup ui
      setMouseTracking(true);
      Layout->setSpacing(0);
      Layout->setMargin(0);
      Layout->addWidget(View);
      Layout->addWidget(ScannerView);
      //setup connections
      this->connect(View, SIGNAL(OnItemSet(const Playitem&)), SIGNAL(OnItemSet(const Playitem&)));
    }

    virtual void dragEnterEvent(QDragEnterEvent* event)
    {
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
        Scanner.AddItems(files);
      }
    }
  private:
    PlaylistScanner& Scanner;
    QVBoxLayout* const Layout;
    PlaylistScannerView* const ScannerView;
    PlaylistView* const View;
  };
}

Playlist* Playlist::Create(QObject* parent, const QString& name, PlayitemsProvider::Ptr provider)
{
  return new PlaylistImpl(parent, name, provider);
}

PlaylistWidget* PlaylistWidget::Create(QWidget* parent, const Playlist& playlist, const class PlayitemStateCallback& callback)
{
  return new PlaylistWidgetImpl(parent, playlist, callback);
}
