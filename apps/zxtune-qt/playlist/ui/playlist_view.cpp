/**
 *
 * @file
 *
 * @brief Playlist view implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/ui/playlist_view.h"

#include "apps/zxtune-qt/playlist/io/export.h"
#include "apps/zxtune-qt/playlist/parameters.h"
#include "apps/zxtune-qt/playlist/supp/container.h"
#include "apps/zxtune-qt/playlist/supp/controller.h"
#include "apps/zxtune-qt/playlist/supp/model.h"
#include "apps/zxtune-qt/playlist/supp/operations.h"
#include "apps/zxtune-qt/playlist/supp/scanner.h"
#include "apps/zxtune-qt/playlist/ui/contextmenu.h"
#include "apps/zxtune-qt/playlist/ui/scanner_view.h"
#include "apps/zxtune-qt/playlist/ui/search.h"
#include "apps/zxtune-qt/playlist/ui/table_view.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/controls/overlay_progress.h"
#include "apps/zxtune-qt/ui/state.h"
#include "apps/zxtune-qt/ui/tools/errorswidget.h"
#include "apps/zxtune-qt/ui/tools/filedialog.h"
#include "apps/zxtune-qt/ui/utils.h"

#include "strings/src/find.h"

#include "binary/base64.h"
#include "binary/view.h"
#include "debug/log.h"
#include "module/attributes.h"
#include "parameters/identifier.h"
#include "parameters/template.h"
#include "parameters/types.h"
#include "strings/fields.h"
#include "strings/join.h"
#include "strings/split.h"
#include "strings/template.h"

#include "contract.h"
#include "error.h"
#include "types.h"

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QIODevice>
#include <QtCore/QIdentityProxyModel>
#include <QtCore/QList>
#include <QtCore/QMimeData>
#include <QtCore/QModelIndex>
#include <QtCore/QNonConstOverload>
#include <QtCore/QSize>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtCore/QtCore>
#include <QtGui/QClipboard>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QHideEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QKeySequence>
#include <QtGui/QPixmap>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>
#include <memory>
#include <utility>
#include <vector>

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
        if (const auto item = Model.GetItem(row))
        {
          const auto& state = item->GetState();
          if (state.IsReady() && !state.GetIfError())
          {
            return Playlist::Item::STOPPED;
          }
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
      using namespace Parameters::ZXTuneQT::Playlist;
      const auto isLooped = Parameters::GetInteger(*Params, LOOPED, LOOPED_DEFAULT);
      const auto isRandom = Parameters::GetInteger(*Params, RANDOMIZED, RANDOMIZED_DEFAULT);
      return (isLooped ? Playlist::Item::LOOPED : 0) | (isRandom ? Playlist::Item::RANDOMIZED : 0);
    }

  private:
    const Parameters::Accessor::Ptr Params;
  };

  class TooltipFieldsSourceAdapter : public Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>
  {
    using Parent = Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>;

  public:
    explicit TooltipFieldsSourceAdapter(const Parameters::Accessor& props)
      : Parent(props)
      , Props(props)
    {}

    String GetFieldValue(StringView fieldName) const override
    {
      if (fieldName == Module::ATTR_PICTURE)
      {
        return GetPicture();
      }
      const auto MAX_LINES = 16;
      auto result = Parent::GetFieldValue(fieldName);
      TrimLongMultiline(result, MAX_LINES);
      EscapeHtml(result);
      return result;
    }

  private:
    String GetPicture() const
    {
      if (const auto pic = Props.FindData(Module::ATTR_PICTURE))
      {
        return CreateThumbnail(*pic);
      }
      return "";
    }

    static String CreateThumbnail(Binary::View pic)
    {
      // TODO: move load/resize/convert to another thread
      QPixmap image;
      if (!image.loadFromData(pic.As<uchar>(), pic.Size()))
      {
        Dbg("Failed to load image!");
        return "";
      }
      const auto originalSize = image.size();
      image = image.scaled(200, 200, Qt::KeepAspectRatio);
      QByteArray bytes;
      {
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "PNG"))
        {
          Dbg("Failed to convert resized image!");
          return "";
        }
      }
      const auto scaledSize = image.size();
      Dbg("Resized image {}x{} ({} bytes) -> {}x{} ({} bytes)", originalSize.width(), originalSize.height(), pic.Size(),
          scaledSize.width(), scaledSize.height(), bytes.size());
      return CreateImageTag({bytes.constData(), static_cast<std::size_t>(bytes.size())});
    }

    static String CreateImageTag(Binary::View val)
    {
      static const char PREFIX[] = R"(<br/><img src="data:image/png;base64,)";
      static const char SUFFIX[] = R"("/>)";
      const auto encodedSize = Binary::Base64::CalculateConvertedSize(val.Size());
      String res;
      res.reserve(std::size(PREFIX) + encodedSize + std::size(SUFFIX));
      res += PREFIX;
      const auto* in = val.As<uint8_t>();
      auto* out = res.data() + res.size();
      res.resize(res.size() + encodedSize);
      Binary::Base64::Encode(in, in + val.Size(), out, out + encodedSize);
      res += SUFFIX;
      return res;
    }

    static void TrimLongMultiline(String& result, int maxLines)
    {
      const auto SEPARATOR = "\n"sv;
      const auto ELLIPSIS = "<...>"sv;
      auto lines = Strings::Split(result, SEPARATOR);
      if (lines.size() <= maxLines)
      {
        return;
      }
      auto tgt = lines.begin() + maxLines / 2 - 1;
      *tgt++ = ELLIPSIS;
      tgt = std::copy(lines.end() - maxLines / 2, lines.end(), tgt);
      lines.erase(tgt, lines.end());
      result = Strings::Join(lines, SEPARATOR);
    }

    // TODO: get rid of template, build tooltip one-by-one and escape html with Qt
    static void EscapeHtml(String& result)
    {
      if (result.size() == Strings::Find::First(result, "&<>\n"sv))
      {
        return;
      }
      String escaped;
      escaped.reserve(result.size() * 11 / 10);
      for (auto c : result)
      {
        switch (c)
        {
        case '&':
          escaped += "&amp;"sv;
          break;
        case '<':
          escaped += "&lt;"sv;
          break;
        case '>':
          escaped += "&gt;"sv;
          break;
        case '\n':
          escaped += "<br/>"sv;
          break;
        default:
          escaped += c;
          break;
        }
      }
      result.swap(escaped);
    }

  private:
    const Parameters::Accessor& Props;
  };

  class TooltipSource
  {
  public:
    QString Get(const Parameters::Accessor& properties) const
    {
      const TooltipFieldsSourceAdapter adapter(properties);
      const auto& result = GetTemplate().Instantiate(adapter);
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
          "[Picture]"
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
      Dbg("Created retranslation model at {} for {}", Self(), static_cast<const void*>(&model));
    }

    ~RetranslateModel() override
    {
      Dbg("Destroyed retranslation model at {}", Self());
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
        return {};
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
    const void* Self() const
    {
      return this;
    }

    QVariant GetTooltip(int_t itemNum) const
    {
      if (const auto item = Delegate.GetItem(itemNum))
      {
        const auto& state = item->GetState();
        if (const auto* err = state.GetIfError())
        {
          return ToQString(err->ToString());
        }
        else if (state.IsReady())
        {
          const auto properties = item->GetModuleProperties();
          return Tooltip.Get(*properties);
        }
        else
        {
          // force prefetch
          QIdentityProxyModel::data(index(itemNum, 0), Qt::DisplayRole);
          return Playlist::UI::View::tr("Loading...");
        }
      }
      return {};
    }

    static QVariant GetHeaderText(unsigned column)
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
        return {};
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
      , Options(PlaylistOptionsWrapper(std::move(params)))
      , State(*Controller->GetModel(), *Controller->GetIterator())
      , View(Playlist::UI::TableView::Create(*this, State, *new RetranslateModel(*Controller->GetModel())))
      , OperationProgress(OverlayProgress::Create(*this))
    {
      // setup ui
      setAcceptDrops(true);
      if (auto* const layout = new QVBoxLayout(this))
      {
        layout->setSpacing(1);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->addWidget(View);
        OperationProgress->setVisible(false);
        if (UI::ErrorsWidget* const errors = UI::ErrorsWidget::Create(*this))
        {
          layout->addWidget(errors);
          Require(connect(Controller->GetScanner(), &Playlist::Scanner::ErrorOccurred, errors,
                          &UI::ErrorsWidget::AddError));
        }
        if (Playlist::UI::ScannerView* const scannerView =
                Playlist::UI::ScannerView::Create(*this, Controller->GetScanner()))
        {
          layout->addWidget(scannerView);
        }
      }
      // setup connections
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      Require(connect(View, &Playlist::UI::TableView::TableRowActivated, iter,
                      qOverload<unsigned>(&Playlist::Item::Iterator::Reset)));
      Require(connect(iter, qOverload<unsigned>(&Playlist::Item::Iterator::ItemActivated), View,
                      &Playlist::UI::TableView::MoveToTableRow));

      // redirect signals to self
      Require(connect(Controller.get(), &Playlist::Controller::Renamed, this, &Playlist::UI::View::Renamed));
      Require(connect(iter, qOverload<Playlist::Item::Data::Ptr>(&Playlist::Item::Iterator::ItemActivated), this,
                      &Playlist::UI::View::ItemActivated));

      const Playlist::Model::Ptr model = Controller->GetModel();
      Require(connect(model, &Playlist::Model::OperationStarted, this, &ViewImpl::LongOperationStart));
      Require(connect(model, &Playlist::Model::OperationProgressChanged, OperationProgress,
                      &OverlayProgress::UpdateProgress));
      Require(connect(model, &Playlist::Model::OperationStopped, this, &ViewImpl::LongOperationStop));
      Require(connect(OperationProgress, &OverlayProgress::Canceled, this, &ViewImpl::LongOperationCancel));

      LayoutState->AddWidget(*View->horizontalHeader());
      Dbg("Created at {}", Self());
    }

    ~ViewImpl() override
    {
      Dbg("Destroyed at {}", Self());
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
        Playlist::Save(*Controller, filename, flags);
      }
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
      Dbg("Layout load for {}", Self());
      LayoutState->Load();
      event->accept();
    }

    void hideEvent(QHideEvent* event) override
    {
      Dbg("Layout save for {}", Self());
      LayoutState->Save();
      event->accept();
    }

  private:
    void LongOperationStart()
    {
      View->setEnabled(false);
      OperationProgress->UpdateProgress(0);
      OperationProgress->setVisible(true);
      OperationProgress->setEnabled(true);
    }

    void LongOperationStop()
    {
      OperationProgress->setVisible(false);
      View->setEnabled(true);
    }

    void LongOperationCancel()
    {
      OperationProgress->setEnabled(false);
      Controller->GetModel()->CancelLongOperation();
    }

    const void* Self() const
    {
      return this;
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

    void RemoveSelectedItems()
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      if (const std::size_t itemsCount = model->CountItems())
      {
        const Playlist::Model::IndexSet::Ptr items = View->GetSelectedItems();
        model->RemoveItems(items);
        if (1 == items->size())
        {
          const Playlist::Model::IndexType itemToSelect = *items->begin();
          View->SelectItems(std::move(items));
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
        Require(connect(op.get(), &Playlist::Item::SelectionOperation::ResultAcquired, View,
                        &Playlist::UI::TableView::SelectItems));
        model->PerformOperation(op);
      }
    }

    static Playlist::IO::ExportFlags GetSavePlaylistFlags(int saveCase)
    {
      const auto options = GlobalOptions::Instance().Get();
      using namespace Parameters::ZXTuneQT::Playlist::Store;
      Playlist::IO::ExportFlags res = 0;
      if (0 != Parameters::GetInteger(*options, PROPERTIES, PROPERTIES_DEFAULT))
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

namespace Playlist::UI
{
  View::View(QWidget& parent)
    : QWidget(&parent)
  {}

  View* View::Create(QWidget& parent, Playlist::Controller::Ptr playlist, Parameters::Accessor::Ptr params)
  {
    return new ViewImpl(parent, std::move(playlist), std::move(params));
  }
}  // namespace Playlist::UI
