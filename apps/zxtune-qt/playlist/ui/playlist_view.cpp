/**
 *
 * @file
 *
 * @brief Playlist view implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "playlist_view.h"
#include "contextmenu.h"
#include "playlist/io/export.h"
#include "playlist/parameters.h"
#include "playlist/supp/container.h"
#include "playlist/supp/controller.h"
#include "playlist/supp/model.h"
#include "playlist/supp/scanner.h"
#include "playlist/supp/storage.h"
#include "scanner_view.h"
#include "search.h"
#include "supp/options.h"
#include "table_view.h"
#include "ui/controls/overlay_progress.h"
#include "ui/state.h"
#include "ui/tools/errorswidget.h"
#include "ui/tools/filedialog.h"
#include "ui/utils.h"
// local includes
#include <contract.h>
#include <error.h>
// library includes
#include <debug/log.h>
#include <parameters/template.h>
#include <strings/template.h>
// boost includes
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/replace.hpp>
// qt includes
#include <QtCore/QIdentityProxyModel>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QVBoxLayout>

namespace
{
  const Debug::Stream Dbg("Playlist::UI::View");

  class PlayitemStateCallbackImpl : public Playlist::Item::StateCallback
  {
  public:
    PlayitemStateCallbackImpl(Playlist::Model& model, Playlist::Item::Iterator& iter)
      : Model(model)
      , Iterator(iter)
    {}

    Playlist::Item::State GetState(const QModelIndex& index) const override
    {
      assert(index.isValid());
      if (index.internalId() == Model.GetVersion())
      {
        const Playlist::Model::IndexType row = index.row();
        if (row == Iterator.GetIndex())
        {
          return Iterator.GetState();
        }
        // TODO: do not access item
        const Playlist::Item::Data::Ptr item = Model.GetItem(row);
        if (item && !item->GetState())
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
      : Params(std::move(params))
    {}

    unsigned GetPlayorderMode() const
    {
      Parameters::IntType isLooped = Parameters::ZXTuneQT::Playlist::LOOPED_DEFAULT;
      Params->FindValue(Parameters::ZXTuneQT::Playlist::LOOPED, isLooped);
      Parameters::IntType isRandom = Parameters::ZXTuneQT::Playlist::RANDOMIZED_DEFAULT;
      Params->FindValue(Parameters::ZXTuneQT::Playlist::RANDOMIZED, isRandom);
      return (isLooped ? Playlist::Item::LOOPED : 0) | (isRandom ? Playlist::Item::RANDOMIZED : 0);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  class TooltipFieldsSourceAdapter : public Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>
  {
    typedef Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource> Parent;

  public:
    explicit TooltipFieldsSourceAdapter(const Parameters::Accessor& props)
      : Parent(props)
    {}

    String GetFieldValue(const String& fieldName) const override
    {
      static const Char AMPERSAND[] = {'&', 0};
      static const Char AMPERSAND_ESCAPED[] = {'&', 'a', 'm', 'p', ';', 0};
      static const Char LBRACKET[] = {'<', 0};
      static const Char LBRACKET_ESCAPED[] = {'&', 'l', 't', ';', 0};
      static const Char RBRACKET[] = {'>', 0};
      static const Char RBRACKET_ESCAPED[] = {'&', 'g', 't', ';', 0};
      static const Char NEWLINE[] = {'\n', 0};
      static const Char NEWLINE_ESCAPED[] = {'<', 'b', 'r', '/', '>', 0};
      static const int MAX_LINES = 16;
      String result = Parent::GetFieldValue(fieldName);
      TrimLongMultiline(result, MAX_LINES);
      boost::algorithm::replace_all(result, AMPERSAND, AMPERSAND_ESCAPED);
      boost::algorithm::replace_all(result, LBRACKET, LBRACKET_ESCAPED);
      boost::algorithm::replace_all(result, RBRACKET, RBRACKET_ESCAPED);
      boost::algorithm::replace_all(result, NEWLINE, NEWLINE_ESCAPED);
      return result;
    }

  private:
    static void TrimLongMultiline(String& result, int maxLines)
    {
      static const Char NEWLINE[] = {'\n', 0};
      static const Char ELLIPSIS[] = {'\n', '<', '.', '.', '.', '>', 0};
      typedef boost::iterator_range<String::iterator> Range;
      const Range head = boost::algorithm::find_nth(result, NEWLINE, maxLines / 2 - 1);
      const Range tail = boost::algorithm::find_nth(result, NEWLINE, -maxLines / 2);
      if (head.begin() < tail.begin())
      {
        boost::algorithm::replace_range(result, Range(head.begin(), tail.begin()), ELLIPSIS);
      }
    }
  };

  class TooltipSource
  {
  public:
    QString Get(const Parameters::Accessor& properties) const
    {
      const TooltipFieldsSourceAdapter adapter(properties);
      const String& result = GetTemplate().Instantiate(adapter);
      return ToQString(result);
    }

  private:
    const Strings::Template& GetTemplate() const
    {
      const QString view = Playlist::UI::View::tr(
          "<html>"
          "[Fullpath]<br/>"
          "[Container]&nbsp;([Size] bytes)<br/><br/>"
          "<b>Title:</b> [Title]<br/>"
          "<b>Author:</b> [Author]<br/>"
          "<b>Program:</b> [Program]<br/>"
          "[Comment]"
          "<pre>[Strings]</pre>"
          "</html>");
      return GetTemplate(view);
    }

    const Strings::Template& GetTemplate(const QString& view) const
    {
      if (view != TemplateView)
      {
        TemplateData = Strings::Template::Create(FromQString(view));
        TemplateView = view;
      }
      return *TemplateData;
    }

  private:
    mutable QString TemplateView;
    mutable Strings::Template::Ptr TemplateData;
  };

  class RetranslateModel : public QIdentityProxyModel
  {
  public:
    explicit RetranslateModel(Playlist::Model& model)
      : QIdentityProxyModel(&model)
      , Delegate(model)
    {
      setSourceModel(&model);
      Dbg("Created retranslation model at %1% for %2%", this, &model);
    }

    ~RetranslateModel() override
    {
      Dbg("Destroyed retranslation model at %1%", this);
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
      if (Qt::Horizontal == orientation && Qt::DisplayRole == role)
      {
        return GetHeaderText(section);
      }
      return QIdentityProxyModel::headerData(section, orientation, role);
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
      if (!index.isValid())
      {
        return QVariant();
      }
      else if (Qt::ToolTipRole == role)
      {
        const int_t itemNum = index.row();
        return GetTooltip(itemNum);
      }
      else
      {
        return QIdentityProxyModel::data(index, role);
      }
    }

    bool canFetchMore(const QModelIndex& index) const override
    {
      return Delegate.canFetchMore(index);
    }

  private:
    QVariant GetTooltip(int_t itemNum) const
    {
      if (const Playlist::Item::Data::Ptr item = Delegate.GetItem(itemNum))
      {
        return GetTooltip(*item);
      }
      return QVariant();
    }

    QVariant GetTooltip(const Playlist::Item::Data& item) const
    {
      if (const Error& err = item.GetState())
      {
        return ToQString(err.ToString());
      }
      else
      {
        const auto properties = item.GetModuleProperties();
        return Tooltip.Get(*properties);
      }
    }

    QVariant GetHeaderText(unsigned column) const
    {
      switch (column)
      {
      case Playlist::Model::COLUMN_TYPE:
        return Playlist::UI::View::tr("Type");
      case Playlist::Model::COLUMN_DISPLAY_NAME:
        return Playlist::UI::View::tr("Author - Title");
      case Playlist::Model::COLUMN_DURATION:
        return Playlist::UI::View::tr("Duration");
      case Playlist::Model::COLUMN_AUTHOR:
        return Playlist::UI::View::tr("Author");
      case Playlist::Model::COLUMN_TITLE:
        return Playlist::UI::View::tr("Title");
      case Playlist::Model::COLUMN_COMMENT:
        return Playlist::UI::View::tr("Comment");
      case Playlist::Model::COLUMN_PATH:
        return Playlist::UI::View::tr("Path");
      case Playlist::Model::COLUMN_SIZE:
        return Playlist::UI::View::tr("Size");
      case Playlist::Model::COLUMN_CRC:
        return Playlist::UI::View::tr("CRC");
      case Playlist::Model::COLUMN_FIXEDCRC:
        return Playlist::UI::View::tr("FixedCRC");
      default:
        return QVariant();
      };
    }

  private:
    Playlist::Model& Delegate;
    TooltipSource Tooltip;
  };

  const QLatin1String ITEMS_MIMETYPE("application/playlist.items");

  struct SaveCases
  {
    enum
    {
      RELPATHS,
      ABSPATHS,
      CONTENT,

      TOTAL
    };
  };

  class ViewImpl : public Playlist::UI::View
  {
  public:
    ViewImpl(QWidget& parent, Playlist::Controller::Ptr playlist, Parameters::Accessor::Ptr params)
      : Playlist::UI::View(parent)
      , LayoutState(UI::State::Create(Parameters::ZXTuneQT::Playlist::NAMESPACE_NAME))
      , Controller(std::move(playlist))
      , Options(PlaylistOptionsWrapper(params))
      , State(*Controller->GetModel(), *Controller->GetIterator())
      , View(Playlist::UI::TableView::Create(*this, State, *new RetranslateModel(*Controller->GetModel())))
      , OperationProgress(OverlayProgress::Create(*this))
    {
      // setup ui
      setAcceptDrops(true);
      if (const auto layout = new QVBoxLayout(this))
      {
        layout->setSpacing(1);
        layout->setMargin(1);
        layout->addWidget(View);
        OperationProgress->setVisible(false);
        if (UI::ErrorsWidget* const errors = UI::ErrorsWidget::Create(*this))
        {
          layout->addWidget(errors);
          Require(errors->connect(Controller->GetScanner(), SIGNAL(ErrorOccurred(const Error&)),
                                  SLOT(AddError(const Error&))));
        }
        if (Playlist::UI::ScannerView* const scannerView =
                Playlist::UI::ScannerView::Create(*this, Controller->GetScanner()))
        {
          layout->addWidget(scannerView);
        }
      }
      // setup connections
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      Require(iter->connect(View, SIGNAL(TableRowActivated(unsigned)), SLOT(Reset(unsigned))));
      Require(View->connect(iter, SIGNAL(ItemActivated(unsigned)), SLOT(MoveToTableRow(unsigned))));

      // redirect signals to self
      Require(connect(Controller.get(), SIGNAL(Renamed(const QString&)), SIGNAL(Renamed(const QString&))));
      Require(connect(iter, SIGNAL(ItemActivated(Playlist::Item::Data::Ptr)),
                      SIGNAL(ItemActivated(Playlist::Item::Data::Ptr))));

      const Playlist::Model::Ptr model = Controller->GetModel();
      Require(connect(model, SIGNAL(OperationStarted()), SLOT(LongOperationStart())));
      Require(OperationProgress->connect(model, SIGNAL(OperationProgressChanged(int)), SLOT(UpdateProgress(int))));
      Require(connect(model, SIGNAL(OperationStopped()), SLOT(LongOperationStop())));
      Require(connect(OperationProgress, SIGNAL(Canceled()), SLOT(LongOperationCancel())));

      LayoutState->AddWidget(*View->horizontalHeader());
      Dbg("Created at %1%", this);
    }

    ~ViewImpl() override
    {
      Dbg("Destroyed at %1%", this);
    }

    Playlist::Controller::Ptr GetPlaylist() const override
    {
      return Controller;
    }

    // modifiers
    void AddItems(const QStringList& items) override
    {
      const Playlist::Scanner::Ptr scanner = Controller->GetScanner();
      scanner->AddItems(items);
    }

    void Play() override
    {
      UpdateState(Playlist::Item::PLAYING);
    }

    void Pause() override
    {
      UpdateState(Playlist::Item::PAUSED);
    }

    void Stop() override
    {
      UpdateState(Playlist::Item::STOPPED);
    }

    void Finish() override
    {
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      bool hasMoreItems = false;
      const unsigned playorderMode = Options.GetPlayorderMode();
      while (iter->Next(playorderMode) && Playlist::Item::ERROR == iter->GetState())
      {
        hasMoreItems = true;
      }
      if (!hasMoreItems)
      {
        Stop();
      }
    }

    void Next() override
    {
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      // skip invalid ones
      const unsigned playorderMode = Options.GetPlayorderMode();
      while (iter->Next(playorderMode) && Playlist::Item::ERROR == iter->GetState())
      {}
    }

    void Prev() override
    {
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      // skip invalid ones
      const unsigned playorderMode = Options.GetPlayorderMode();
      while (iter->Prev(playorderMode) && Playlist::Item::ERROR == iter->GetState())
      {}
    }

    void Clear() override
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->Clear();
      Update();
    }

    void AddFiles() override
    {
      QStringList files;
      if (UI::OpenMultipleFilesDialog(Playlist::UI::View::tr("Add files"), Playlist::UI::View::tr("All files (*)"),
                                      files))
      {
        AddItems(files);
      }
    }

    void AddFolder() override
    {
      QStringList folders;
      folders += QString();
      if (UI::OpenFolderDialog(Playlist::UI::View::tr("Add folder"), folders.front()))
      {
        AddItems(folders);
      }
    }

    void Rename() override
    {
      const QString oldName = Controller->GetName();
      bool ok = false;
      const QString newName = QInputDialog::getText(this, Playlist::UI::View::tr("Rename playlist"), QString(),
                                                    QLineEdit::Normal, oldName, &ok);
      if (ok && !newName.isEmpty())
      {
        Controller->SetName(newName);
      }
    }

    void Save() override
    {
      QStringList filters;
      filters.insert(SaveCases::RELPATHS, Playlist::UI::View::tr("Playlist with relative paths (*.xspf)"));
      filters.insert(SaveCases::ABSPATHS, Playlist::UI::View::tr("Playlist with absolute paths (*.xspf)"));
      filters.insert(SaveCases::CONTENT, Playlist::UI::View::tr("Playlist with embedded modules' data (*.xspf)"));

      QString filename = Controller->GetName();
      int saveCase = 0;
      if (UI::SaveFileDialog(Playlist::UI::View::tr("Save playlist"), QLatin1String("xspf"), filters, filename,
                             &saveCase))
      {
        const Playlist::IO::ExportFlags flags = GetSavePlaylistFlags(saveCase);
        Playlist::Save(Controller, filename, flags);
      }
    }

    void LongOperationStart() override
    {
      View->setEnabled(false);
      OperationProgress->UpdateProgress(0);
      OperationProgress->setVisible(true);
      OperationProgress->setEnabled(true);
    }

    void LongOperationStop() override
    {
      OperationProgress->setVisible(false);
      View->setEnabled(true);
    }

    void LongOperationCancel() override
    {
      OperationProgress->setEnabled(false);
      Controller->GetModel()->CancelLongOperation();
    }

    // qwidget virtuals
    void keyPressEvent(QKeyEvent* event) override
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

    void contextMenuEvent(QContextMenuEvent* event) override
    {
      Playlist::UI::ExecuteContextMenu(event->globalPos(), *View, *Controller);
    }

    void dragEnterEvent(QDragEnterEvent* event) override
    {
      event->acceptProposedAction();
    }

    void dropEvent(QDropEvent* event) override
    {
      if (const QMimeData* mimeData = event->mimeData())
      {
        PasteData(*mimeData);
      }
    }

    void resizeEvent(QResizeEvent* event) override
    {
      const QSize& newSize = event->size();
      const QSize& opSize = OperationProgress->size();
      const QSize& newPos = (newSize - opSize) / 2;
      OperationProgress->move(newPos.width(), newPos.height());
      event->accept();
    }

    void showEvent(QShowEvent* event) override
    {
      Dbg("Layout load for %1%", this);
      LayoutState->Load();
      event->accept();
    }

    void hideEvent(QHideEvent* event) override
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
        const Playlist::Model::IndexSet::Ptr items = View->GetSelectedItems();
        model->RemoveItems(items);
        if (1 == items->size())
        {
          View->SelectItems(items);
          const Playlist::Model::IndexType itemToSelect = *items->begin();
          // not last
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
        const Playlist::Model::IndexSet::Ptr items = View->GetSelectedItems();
        const QStringList& paths = model->GetItemsPaths(*items);
        QByteArray data;
        {
          QDataStream stream(&data, QIODevice::WriteOnly);
          stream << paths;
        }
        std::unique_ptr<QMimeData> mimeData(new QMimeData());
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
        for (const auto& url : data.urls())
        {
          files.push_back(url.toLocalFile());
        }
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
        // pasting is done immediately, so update UI right here
        const Playlist::Scanner::Ptr scanner = Controller->GetScanner();
        scanner->PasteItems(items);
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
      if (const Playlist::Item::SelectionOperation::Ptr op = Playlist::UI::ExecuteSearchDialog(*View))
      {
        // TODO: extract
        const Playlist::Model::Ptr model = Controller->GetModel();
        op->setParent(model);
        Require(View->connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSet::Ptr)),
                              SLOT(SelectItems(Playlist::Model::IndexSet::Ptr))));
        model->PerformOperation(op);
      }
    }

    Playlist::IO::ExportFlags GetSavePlaylistFlags(int saveCase) const
    {
      const Parameters::Accessor::Ptr options = GlobalOptions::Instance().Get();
      Parameters::IntType val = Parameters::ZXTuneQT::Playlist::Store::PROPERTIES_DEFAULT;
      options->FindValue(Parameters::ZXTuneQT::Playlist::Store::PROPERTIES, val);
      Playlist::IO::ExportFlags res = 0;
      if (val)
      {
        res |= Playlist::IO::SAVE_ATTRIBUTES;
      }
      switch (saveCase)
      {
      case SaveCases::RELPATHS:
        res |= Playlist::IO::RELATIVE_PATHS;
        break;
      case SaveCases::CONTENT:
        res |= Playlist::IO::SAVE_CONTENT;
        break;
      }
      return res;
    }

  private:
    const UI::State::Ptr LayoutState;
    const Playlist::Controller::Ptr Controller;
    const PlaylistOptionsWrapper Options;
    // state
    PlayitemStateCallbackImpl State;
    Playlist::UI::TableView* const View;
    OverlayProgress* const OperationProgress;
  };
}  // namespace

namespace Playlist
{
  namespace UI
  {
    View::View(QWidget& parent)
      : QWidget(&parent)
    {}

    View* View::Create(QWidget& parent, Playlist::Controller::Ptr playlist, Parameters::Accessor::Ptr params)
    {
      return new ViewImpl(parent, playlist, params);
    }
  }  // namespace UI
}  // namespace Playlist
