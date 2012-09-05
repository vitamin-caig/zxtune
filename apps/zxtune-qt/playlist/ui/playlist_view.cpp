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
#include "contextmenu.h"
#include "playlist_view.h"
#include "search.h"
#include "playlist/parameters.h"
#include "playlist/io/export.h"
#include "playlist/supp/container.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/model.h"
#include "playlist/supp/scanner.h"
#include "playlist/supp/storage.h"
#include "ui/state.h"
#include "ui/utils.h"
#include "ui/controls/overlay_progress.h"
#include "ui/tools/errorswidget.h"
#include "ui/tools/filedialog.h"
//local includes
#include <contract.h>
#include <debug_log.h>
#include <error.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QUrl>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QInputDialog>
#include <QtGui/QKeyEvent>
#include <QtGui/QProgressBar>
#include <QtGui/QVBoxLayout>

namespace
{
  const Debug::Stream Dbg("Playlist::UI::View");

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
      Params->FindValue(Parameters::ZXTuneQT::Playlist::DEEP_SCANNING, val);
      return val != 0;
    }

    unsigned GetPlayorderMode() const
    {
      Parameters::IntType isLooped = Parameters::ZXTuneQT::Playlist::LOOPED_DEFAULT;
      Params->FindValue(Parameters::ZXTuneQT::Playlist::LOOPED, isLooped);
      Parameters::IntType isRandom = Parameters::ZXTuneQT::Playlist::RANDOMIZED_DEFAULT;
      Params->FindValue(Parameters::ZXTuneQT::Playlist::RANDOMIZED, isRandom);
      return
        (isLooped ? Playlist::Item::LOOPED : 0) |
        (isRandom ? Playlist::Item::RANDOMIZED: 0)
      ;
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };

  const QLatin1String ITEMS_MIMETYPE("application/playlist.items");

  class ViewImpl : public Playlist::UI::View
  {
  public:
    ViewImpl(QWidget& parent, Playlist::Controller::Ptr playlist, Parameters::Accessor::Ptr params)
      : Playlist::UI::View(parent)
      , LayoutState(UI::State::Create(Parameters::ZXTuneQT::Playlist::NAMESPACE_NAME))
      , Controller(playlist)
      , Options(PlaylistOptionsWrapper(params))
      , State(*Controller->GetModel(), *Controller->GetIterator())
      , View(Playlist::UI::TableView::Create(*this, State, Controller->GetModel()))
      , OperationProgress(OverlayProgress::Create(*this))
    {
      //setup ui
      setAcceptDrops(true);
      if (QVBoxLayout* const layout = new QVBoxLayout(this))
      {
        layout->setSpacing(1);
        layout->setMargin(1);
        layout->addWidget(View);
        OperationProgress->setVisible(false);
        if (UI::ErrorsWidget* const errors = UI::ErrorsWidget::Create(*this))
        {
          layout->addWidget(errors);
          Require(errors->connect(Controller->GetScanner(), SIGNAL(ErrorOccurred(const Error&)), SLOT(AddError(const Error&))));
        }
        if (Playlist::UI::ScannerView* const scannerView = Playlist::UI::ScannerView::Create(*this, Controller->GetScanner()))
        {
          layout->addWidget(scannerView);
        }
      }
      //setup connections
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      Require(iter->connect(View, SIGNAL(TableRowActivated(unsigned)), SLOT(Reset(unsigned))));
      Require(connect(Controller.get(), SIGNAL(Renamed(const QString&)), SIGNAL(Renamed(const QString&))));
      Require(connect(iter, SIGNAL(ItemActivated(unsigned, Playlist::Item::Data::Ptr)),
        SLOT(ActivateItem(unsigned, Playlist::Item::Data::Ptr))));
      Require(View->connect(Controller->GetScanner(), SIGNAL(OnScanStop()), SLOT(updateGeometries())));

      const Playlist::Model::Ptr model = Controller->GetModel();
      Require(connect(model, SIGNAL(OperationStarted()), SLOT(LongOperationStart())));
      Require(OperationProgress->connect(model, SIGNAL(OperationProgressChanged(int)), SLOT(UpdateProgress(int))));
      Require(connect(model, SIGNAL(OperationStopped()), SLOT(LongOperationStop())));
      Require(connect(OperationProgress, SIGNAL(Canceled()), SLOT(LongOperationCancel())));

      LayoutState->AddWidget(*View->horizontalHeader());
      Dbg("Created at %1%", this);
    }

    virtual ~ViewImpl()
    {
      Dbg("Destroyed at %1%", this);
    }

    virtual Playlist::Controller::Ptr GetPlaylist() const
    {
      return Controller;
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

    virtual void AddFiles()
    {
      QStringList files;
      if (UI::OpenMultipleFilesDialog(
        Playlist::UI::View::tr("Add files"),
        Playlist::UI::View::tr("All files (*.*)"), files))
      {
        AddItems(files);
      }
    }

    virtual void AddFolder()
    {
      QStringList folders;
      folders += QString();
      if (UI::OpenFolderDialog(Playlist::UI::View::tr("Add folder"), folders.front()))
      {
        AddItems(folders);
      }
    }

    virtual void Rename()
    {
      const QString oldName = Controller->GetName();
      bool ok = false;
      const QString newName = QInputDialog::getText(this, Playlist::UI::View::tr("Rename playlist"), QString(), QLineEdit::Normal, oldName, &ok);
      if (ok && !newName.isEmpty())
      {
        Controller->SetName(newName);
      }
    }

    virtual void Save()
    {
      QStringList filters;
      filters << Playlist::UI::View::tr("Simple playlist (*.xspf)");
      filters << Playlist::UI::View::tr("Playlist with module's attributes (*.xspf)");

      QString filename = Controller->GetName();
      int usedFilter = 0;
      if (UI::SaveFileDialog(Playlist::UI::View::tr("Save playlist"),
        QLatin1String("xspf"), filters, filename, &usedFilter))
      {
        Playlist::IO::ExportFlags flags = 0;
        if (1 == usedFilter)
        {
          flags |= Playlist::IO::SAVE_ATTRIBUTES;
        }
        Playlist::Save(Controller, filename, flags);
      }
    }

    virtual void ActivateItem(unsigned idx, Playlist::Item::Data::Ptr data)
    {
      View->ActivateTableRow(idx);
      emit ItemActivated(data);
    }

    virtual void LongOperationStart()
    {
      View->setEnabled(false);
      OperationProgress->UpdateProgress(0);
      OperationProgress->setVisible(true);
      OperationProgress->setEnabled(true);
    }

    virtual void LongOperationStop()
    {
      OperationProgress->setVisible(false);
      View->setEnabled(true);
    }

    virtual void LongOperationCancel()
    {
      OperationProgress->setEnabled(false);
      Controller->GetModel()->CancelLongOperation();
    }

    //qwidget virtuals
    virtual void keyPressEvent(QKeyEvent* event)
    {
      if (event->matches(QKeySequence::Delete) || event->key() == Qt::Key_Backspace)
      {
        RemoveSelectedItems();
      }
      else if (event->matches(QKeySequence::Cut))
      {
        CopySelectedItems();
        RemoveSelectedItems();
      }
      else if (event->matches(QKeySequence::Copy))
      {
        CopySelectedItems();
      }
      else if (event->matches(QKeySequence::Paste))
      {
        PasteItems();
      }
      else if (event->matches(QKeySequence::Find))
      {
        SearchItems();
      }
      else
      {
        QWidget::keyPressEvent(event);
      }
    }

    virtual void contextMenuEvent(QContextMenuEvent* event)
    {
      Playlist::UI::ExecuteContextMenu(event->globalPos(), *View, *Controller);
    }

    virtual void dragEnterEvent(QDragEnterEvent* event)
    {
      event->acceptProposedAction();
    }

    virtual void dropEvent(QDropEvent* event)
    {
      if (const QMimeData* mimeData = event->mimeData())
      {
        PasteData(*mimeData);
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

    virtual void showEvent(QShowEvent* event)
    {
      Dbg("Layout load for %1%", this);
      LayoutState->Load();
      event->accept();
    }

    virtual void hideEvent(QHideEvent* event)
    {
      Dbg("Layout save for %1%", this);
      LayoutState->Save();
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

    void RemoveSelectedItems()
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      if (const std::size_t itemsCount = model->CountItems())
      {
        const Playlist::Model::IndexSetPtr items = View->GetSelectedItems();
        model->RemoveItems(*items);
        if (1 == items->size())
        {
          View->SelectItems(items);
          const Playlist::Model::IndexType itemToSelect = *items->begin();
          //not last
          if (itemToSelect != itemsCount - 1)
          {
            View->selectRow(itemToSelect);
          }
          else if (itemToSelect)
          {
            View->selectRow(itemToSelect - 1);
          }
        }
      }
    }

    void CopySelectedItems()
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      if (const std::size_t itemsCount = model->CountItems())
      {
        const Playlist::Model::IndexSetPtr items = View->GetSelectedItems();
        const QStringList& paths = model->GetItemsPaths(*items);
        QByteArray data;
        {
          QDataStream stream(&data, QIODevice::WriteOnly);
          stream << paths;
        }
        std::auto_ptr<QMimeData> mimeData(new QMimeData());
        mimeData->setData(ITEMS_MIMETYPE, data);
        QApplication::clipboard()->setMimeData(mimeData.release());
      }
    }

    void PasteItems()
    {
      QClipboard* const cb = QApplication::clipboard();
      if (const QMimeData* mimeData = cb->mimeData())
      {
        PasteData(*mimeData);
      }
    }

    void PasteData(const QMimeData& data)
    {
      if (data.hasUrls())
      {
        const QList<QUrl>& urls = data.urls();
        QStringList files;
        std::for_each(urls.begin(), urls.end(),
          boost::bind(&QStringList::push_back, &files,
            boost::bind(&QUrl::toLocalFile, _1)));
        AddItems(files);
      }
      else if (data.hasFormat(ITEMS_MIMETYPE))
      {
        const QByteArray& encodedData = data.data(ITEMS_MIMETYPE);
        QStringList items;
        {
          QDataStream stream(encodedData);
          stream >> items;
        }
        const Playlist::Scanner::Ptr scanner = Controller->GetScanner();
        scanner->AddItems(items, false);
      }
      else if (data.hasText())
      {
        const QString& text = data.text();
        const QUrl& url = QUrl::fromUserInput(text);
        if (url.isValid())
        {
          AddItems(QStringList(url.toString()));
        }
      }
    }

    void SearchItems()
    {
      Playlist::UI::ExecuteSearchDialog(*View, *Controller);
    }
  private:
    const UI::State::Ptr LayoutState;
    const Playlist::Controller::Ptr Controller;
    const PlaylistOptionsWrapper Options;
    //state
    PlayitemStateCallbackImpl State;
    Playlist::UI::TableView* const View;
    OverlayProgress* const OperationProgress;
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
