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
#include "apps/base/app.h"
#include "playlist_contextmenu.h"
#include "playlist_view.h"
#include "playlist/playlist_parameters.h"
#include "playlist/io/export.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/model.h"
#include "playlist/supp/scanner.h"
#include "playlist/supp/storage.h"
#include "ui/utils.h"
#include "ui/controls/overlay_progress.h"
#include "ui/tools/filedialog.h"
//local includes
#include <error.h>
#include <logging.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QProgressBar>
#include <QtGui/QVBoxLayout>

namespace
{
  const std::string THIS_MODULE("Playlist::UI::View");

  class ContainerImpl : public Playlist::IO::Container
  {
  public:
    ContainerImpl(const String& name, const Playlist::Item::Storage& storage)
      : Properties(Parameters::Container::Create())
      , Storage(storage)
    {
      Properties->SetStringValue(Playlist::ATTRIBUTE_NAME, name);
      Properties->SetStringValue(Playlist::ATTRIBUTE_CREATOR, GetProgramVersionString());
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual unsigned GetItemsCount() const
    {
      return Storage.CountItems();
    }

    virtual void ForAllItems(Playlist::Item::Callback& callback) const
    {
      ItemCallbackAdapter adapter(callback);
      Storage.ForAllItems(adapter);
    }
  private:
    class ItemCallbackAdapter : public Playlist::Model::Visitor
    {
    public:
      explicit ItemCallbackAdapter(Playlist::Item::Callback& delegate)
        : Delegate(delegate)
      {
      }

      virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
      {
        return Delegate.OnItem(data);
      }
    private:
      Playlist::Item::Callback& Delegate;
    };
  private:
    const Parameters::Container::Ptr Properties;
    const Playlist::Item::Storage& Storage;
  };

  class CallbackWrapper : public Playlist::IO::ExportCallback
  {
  public:
    explicit CallbackWrapper(Log::ProgressCallback& cb)
      : Delegate(cb)
    {
    }

    virtual void Progress(unsigned current)
    {
      Delegate.OnProgress(current);
    }

    virtual bool IsCanceled() const
    {
      return false;
    }
  private:
    Log::ProgressCallback& Delegate;
  };

  class SavePlaylistOperation : public Playlist::Item::StorageAccessOperation
  {
  public:
    explicit SavePlaylistOperation(const QString& name, const QString& filename)
      : Name(FromQString(name))
      , Filename(filename)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& storage, Log::ProgressCallback& cb)
    {
      const Playlist::IO::Container::Ptr container = boost::make_shared<ContainerImpl>(Name, storage);
      CallbackWrapper callback(cb);
      if (const Error& err = Playlist::IO::SaveXSPF(container, Filename, callback))
      {
        //TODO: handle error
      }
    }
  private:
    const String Name;
    const QString Filename;
  };

  class PlayitemStateCallbackImpl : public Playlist::Item::StateCallback
  {
  public:
    PlayitemStateCallbackImpl(Playlist::Model& model, Playlist::Item::Iterator& iter)
      : Model(model)
      , Iterator(iter)
    {
    }

    virtual Playlist::Item::State GetState(const QModelIndex& index) const
    {
      assert(index.isValid());
      if (index.internalId() == Model.GetVersion())
      {
        const Playlist::Model::IndexType row = index.row();
        if (row == Iterator.GetIndex())
        {
          return Iterator.GetState();
        }
        //TODO: do not access item
        const Playlist::Item::Data::Ptr item = Model.GetItem(row);
        if (item && item->IsValid())
        {
          return Playlist::Item::STOPPED;
        }
      }
      return Playlist::Item::ERROR;
    }
  private:
    const Playlist::Model& Model;
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
      , State(*Controller->GetModel(), *Controller->GetIterator())
      , Layout(new QVBoxLayout(this))
      , ScannerView(Playlist::UI::ScannerView::Create(*this, Controller->GetScanner()))
      , View(Playlist::UI::TableView::Create(*this, State, Controller->GetModel()))
      , OperationProgress(OverlayProgress::Create(*this))
      , ItemsMenu(Playlist::UI::ItemsContextMenu::Create(*View, playlist))
    {
      //setup ui
      setAcceptDrops(true);
      Layout->setSpacing(0);
      Layout->setMargin(0);
      Layout->addWidget(View);
      OperationProgress->setVisible(false);
      Layout->addWidget(ScannerView);
      //setup connections
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      iter->connect(View, SIGNAL(OnTableRowActivated(unsigned)), SLOT(Reset(unsigned)));
      this->connect(iter, SIGNAL(OnListItemActivated(unsigned, const Playlist::Item::Data&)), 
        SLOT(ListItemActivated(unsigned, const Playlist::Item::Data&)));
      View->connect(Controller->GetScanner(), SIGNAL(OnScanStop()), SLOT(updateGeometries()));

      const Playlist::Model::Ptr model = Controller->GetModel();
      this->connect(model, SIGNAL(OnLongOperationStart()), SLOT(LongOperationStart()));
      OperationProgress->connect(model, SIGNAL(OnLongOperationProgress(int)), SLOT(UpdateProgress(int)));
      this->connect(model, SIGNAL(OnLongOperationStop()), SLOT(LongOperationStop()));

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

    virtual void Rename()
    {
      const QString oldName = Controller->GetName();
      bool ok = false;
      const QString newName = QInputDialog::getText(this, QString::fromUtf8("Rename playlist"), QString(), QLineEdit::Normal, oldName, &ok);
      if (ok && !newName.isEmpty())
      {
        Controller->SetName(newName);
      }
    }

    virtual void Save()
    {
      const QString name = Controller->GetName();
      QString filename = name;
      if (FileDialog::Instance().SaveFile(QString::fromUtf8("Save playlist"),
        QString::fromUtf8("xspf"),
        QString::fromUtf8("Playlist files (*.xspf)"),
        filename))
      {
        const Playlist::Item::StorageAccessOperation::Ptr op = boost::make_shared<SavePlaylistOperation>(name, filename);
        Controller->GetModel()->PerformOperation(op);
      }
    }

    virtual void ListItemActivated(unsigned idx, const Playlist::Item::Data& data)
    {
      View->ActivateTableRow(idx);
      OnItemActivated(data);
    }

    virtual void LongOperationStart()
    {
      setEnabled(false);
      OperationProgress->setVisible(true);
    }

    virtual void LongOperationStop()
    {
      OperationProgress->setVisible(false);
      setEnabled(true);
    }

    //qwidget virtuals
    virtual void keyReleaseEvent(QKeyEvent* event)
    {
      const int curKey = event->key();
      if (curKey == Qt::Key_Delete || curKey == Qt::Key_Backspace)
      {
        const Playlist::Model::IndexSet& items = View->GetSelectedItems();
        const Playlist::Model::Ptr model = Controller->GetModel();
        model->RemoveItems(items);
      }
      else
      {
        QWidget::keyReleaseEvent(event);
      }
    }

    virtual void contextMenuEvent(QContextMenuEvent* event)
    {
      ItemsMenu->Exec(event->globalPos());
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

    virtual void resizeEvent(QResizeEvent* event)
    {
      const QSize& newSize = event->size();
      const QSize& opSize = OperationProgress->size();
      const QSize& newPos = (newSize - opSize) / 2;
      OperationProgress->move(newPos.width(), newPos.height());
      event->accept();
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
    OverlayProgress* const OperationProgress;
    Playlist::UI::ItemsContextMenu* const ItemsMenu;
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
