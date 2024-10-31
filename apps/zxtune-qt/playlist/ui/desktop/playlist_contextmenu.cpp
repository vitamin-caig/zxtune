/**
 *
 * @file
 *
 * @brief Playlist context menu implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/playlist/ui/desktop/playlist_contextmenu.h"
#include "apps/zxtune-qt/playlist/supp/operations.h"
#include "apps/zxtune-qt/playlist/supp/operations_convert.h"
#include "apps/zxtune-qt/playlist/supp/operations_statistic.h"
#include "apps/zxtune-qt/playlist/supp/storage.h"
#include "apps/zxtune-qt/playlist/ui/contextmenu.h"
#include "apps/zxtune-qt/playlist/ui/desktop/properties_dialog.h"
#include "apps/zxtune-qt/playlist/ui/desktop/search_dialog.h"
#include "apps/zxtune-qt/playlist/ui/table_view.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/conversion/filename_template.h"
#include "apps/zxtune-qt/ui/conversion/setup_conversion.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "multiple_items_contextmenu.ui.h"
#include "no_items_contextmenu.ui.h"
#include "single_item_contextmenu.ui.h"
// common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <strings/map.h>
#include <strings/set.h>
#include <time/serialize.h>
// qt includes
#include <QtGui/QClipboard>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>

namespace
{
  class NoItemsContextMenu
    : public QMenu
    , private Ui::NoItemsContextMenu
  {
  public:
    NoItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& menu)
      : QMenu(&parent)
    {
      // setup self
      setupUi(this);

      Require(connect(DelDupsAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::RemoveAllDuplicates));
      Require(connect(DelUnavailableAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::RemoveAllUnavailable));
      Require(connect(ShuffleAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::ShuffleAll));
      Require(connect(SelRipOffsAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::SelectAllRipOffs));
      Require(connect(SelFoundAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::SelectFound));
      Require(
          connect(ShowStatisticAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::ShowAllStatistic));
      Require(connect(ExportAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::ExportAll));
    }
  };

  class SingleItemContextMenu
    : public QMenu
    , private Ui::SingleItemContextMenu
  {
  public:
    SingleItemContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& menu)
      : QMenu(&parent)
    {
      // setup self
      setupUi(this);

      Require(connect(PlayAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::PlaySelected));
      Require(connect(DeleteAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::RemoveSelected));
      Require(connect(CropAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::CropSelected));
      Require(connect(DelDupsAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::RemoveDuplicatesOfSelected));
      Require(connect(SelRipOffsAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::SelectRipOffsOfSelected));
      Require(connect(SelSameTypesAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::SelectSameTypesOfSelected));
      Require(connect(SelSameFilesAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::SelectSameFilesOfSelected));
      Require(connect(CopyToClipboardAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::CopyPathToClipboard));
      Require(connect(ExportAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::ExportSelected));
      Require(connect(ConvertAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::ConvertSelected));
      Require(connect(SaveAsAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::SaveAsSelected));
      Require(connect(PropertiesAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::ShowPropertiesOfSelected));
    }
  };

  QString ModulesCount(uint_t count)
  {
    return Playlist::UI::ItemsContextMenu::tr("%n item(s)", "", static_cast<int>(count));
  }

  QString QFileSystemModelTranslate(const char* msg)
  {
    return QApplication::translate("QFileSystemModel", msg);
  }

  QString MemorySize(uint64_t size)
  {
    // From gui/dialogs/qfilesystemmodel.cpp
    const uint64_t kb = 1024;
    const uint64_t mb = 1024 * kb;
    const uint64_t gb = 1024 * mb;
    const uint64_t tb = 1024 * gb;

    if (size >= tb)
    {
      return QFileSystemModelTranslate("%1 TB").arg(QLocale().toString(qreal(size) / tb, 'f', 3));
    }
    else if (size >= gb)
    {
      return QFileSystemModelTranslate("%1 GB").arg(QLocale().toString(qreal(size) / gb, 'f', 2));
    }
    else if (size >= mb)
    {
      return QFileSystemModelTranslate("%1 MB").arg(QLocale().toString(qreal(size) / mb, 'f', 1));
    }
    else if (size >= kb)
    {
      return QFileSystemModelTranslate("%1 KB").arg(QLocale().toString(qreal(size) / kb));
    }
    else
    {
      return QFileSystemModelTranslate("%1 bytes").arg(QLocale().toString(qulonglong(size)));
    }
  }

  class MultipleItemsContextMenu
    : public QMenu
    , private Ui::MultipleItemsContextMenu
  {
  public:
    MultipleItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& menu, std::size_t count)
      : QMenu(&parent)
    {
      // setup self
      setupUi(this);
      InfoAction->setText(ModulesCount(count));

      Require(connect(DeleteAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::RemoveSelected));
      Require(connect(CropAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::CropSelected));
      Require(connect(GroupAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::GroupSelected));
      Require(connect(DelDupsAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::RemoveDuplicatesInSelected));
      Require(connect(DelUnavailableAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::RemoveUnavailableInSelected));
      Require(connect(SelRipOffsAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::SelectRipOffsInSelected));
      Require(connect(SelSameTypesAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::SelectSameTypesOfSelected));
      Require(connect(SelSameFilesAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::SelectSameFilesOfSelected));
      Require(
          connect(SelFoundAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::SelectFoundInSelected));
      Require(connect(CopyToClipboardAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::CopyPathToClipboard));
      Require(connect(ShowStatisticAction, &QAction::triggered, &menu,
                      &Playlist::UI::ItemsContextMenu::ShowStatisticOfSelected));
      Require(connect(ExportAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::ExportSelected));
      Require(connect(ConvertAction, &QAction::triggered, &menu, &Playlist::UI::ItemsContextMenu::ConvertSelected));
    }
  };

  const QLatin1String LINE_BREAK("\n");

  class StatisticNotification : public Playlist::Item::StatisticTextNotification
  {
  public:
    StatisticNotification() = default;

    QString Category() const override
    {
      return Playlist::UI::ItemsContextMenu::tr("Statistic");
    }

    QString Text() const override
    {
      QStringList result;
      result.append(Playlist::UI::ItemsContextMenu::tr("Total: %1").arg(ModulesCount(Processed)));
      result.append(Playlist::UI::ItemsContextMenu::tr("Invalid: %1").arg(ModulesCount(Invalids)));
      result.append(Playlist::UI::ItemsContextMenu::tr("Total duration: %1").arg(ToQString(Time::ToString(Duration))));
      result.append(Playlist::UI::ItemsContextMenu::tr("Total size: %1").arg(MemorySize(Size)));
      result.append(Playlist::UI::ItemsContextMenu::tr("%n different modules' type(s)", "", Types.size()));
      result.append(Playlist::UI::ItemsContextMenu::tr("%n files referenced", "", Paths.size()));
      return result.join(LINE_BREAK);
    }

    QString Details() const override
    {
      QStringList result;
      for (const auto& type : Types)
      {
        result.append(QString::fromLatin1("%1: %2").arg(ToQString(type.first)).arg(ModulesCount(type.second)));
      }
      return result.join(LINE_BREAK);
    }

    void AddInvalid() override
    {
      ++Invalids;
    }

    void AddValid() override
    {
      ++Processed;
    }

    void AddType(StringView type) override
    {
      ++Types[type];
    }

    void AddDuration(const Time::Milliseconds& duration) override
    {
      Duration += duration;
    }

    void AddSize(std::size_t size) override
    {
      Size += size;
    }

    void AddPath(StringView path) override
    {
      Paths.emplace(path);  // TODO
    }

  private:
    std::size_t Processed = 0;
    std::size_t Invalids = 0;
    Time::Duration<Time::BaseUnit<uint64_t, 1000>> Duration;
    uint64_t Size = 0;
    Strings::ValueMap<std::size_t> Types;
    Strings::Set Paths;
  };

  Playlist::Item::StatisticTextNotification::Ptr CreateStatisticNotification()
  {
    return MakePtr<StatisticNotification>();
  }

  class ExportResult : public Playlist::Item::ConversionResultNotification
  {
  public:
    ExportResult() = default;

    QString Category() const override
    {
      return Playlist::UI::ItemsContextMenu::tr("Export");
    }

    QString Text() const override
    {
      return Playlist::UI::ItemsContextMenu::tr("Converted: %1<br/>Failed: %2")
          .arg(ModulesCount(Succeeds))
          .arg(ModulesCount(Errors.size()));
    }

    QString Details() const override
    {
      return Errors.join(LINE_BREAK);
    }

    void AddSucceed() override
    {
      ++Succeeds;
    }

    void AddFailedToOpen(StringView path) override
    {
      Errors.append(Playlist::UI::ItemsContextMenu::tr("Failed to open '%1' for conversion").arg(ToQString(path)));
    }

    void AddFailedToConvert(StringView path, const Error& err) override
    {
      Errors.append(Playlist::UI::ItemsContextMenu::tr("Failed to convert '%1': %2")
                        .arg(ToQString(path))
                        .arg(ToQString(err.ToString())));
    }

  private:
    std::size_t Succeeds = 0;
    QStringList Errors;
  };

  Playlist::Item::ConversionResultNotification::Ptr CreateConversionResultNotification()
  {
    return MakePtr<ExportResult>();
  }

  class ShuffleOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    void Execute(Playlist::Item::Storage& storage, Log::ProgressCallback& /*cb*/) override
    {
      storage.Shuffle();
    }
  };

  class ItemsContextMenuImpl : public Playlist::UI::ItemsContextMenu
  {
  public:
    ItemsContextMenuImpl(Playlist::UI::TableView& view, Playlist::Controller& playlist)
      : Playlist::UI::ItemsContextMenu(view)
      , View(view)
      , Controller(playlist)
    {}

    void Exec(const QPoint& pos)
    {
      SelectedItems = View.GetSelectedItems();
      const std::unique_ptr<QMenu> delegate = CreateMenu();
      delegate->exec(pos);
    }

    void PlaySelected() const override
    {
      assert(SelectedItems->size() == 1);
      const Playlist::Item::Iterator::Ptr iter = Controller.GetIterator();
      iter->Reset(*SelectedItems->begin());
    }

    void RemoveSelected() const override
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      model->RemoveItems(SelectedItems);
    }

    void CropSelected() const override
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Model::IndexSet::RWPtr unselected = MakeRWPtr<Playlist::Model::IndexSet>();
      for (unsigned idx = 0, total = model->CountItems(); idx < total; ++idx)
      {
        if (!SelectedItems->count(idx))
        {
          unselected->insert(idx);
        }
      }
      model->RemoveItems(unselected);
    }

    void GroupSelected() const override
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const unsigned moveTo = *SelectedItems->begin();
      model->MoveItems(*SelectedItems, moveTo);
      auto toSelect = MakeRWPtr<Playlist::Model::IndexSet>();
      for (unsigned idx = 0, lim = SelectedItems->size(); idx != lim; ++idx)
      {
        toSelect->insert(moveTo + idx);
      }
      View.SelectItems(std::move(toSelect));
    }

    void RemoveAllDuplicates() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectAllDuplicatesOperation();
      ExecuteRemoveOperation(op);
    }

    void RemoveDuplicatesOfSelected() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op =
          Playlist::Item::CreateSelectDuplicatesOfSelectedOperation(SelectedItems);
      ExecuteRemoveOperation(op);
    }

    void RemoveDuplicatesInSelected() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op =
          Playlist::Item::CreateSelectDuplicatesInSelectedOperation(SelectedItems);
      ExecuteRemoveOperation(op);
    }

    void RemoveAllUnavailable() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectAllUnavailableOperation();
      ExecuteRemoveOperation(op);
    }

    void RemoveUnavailableInSelected() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op =
          Playlist::Item::CreateSelectUnavailableInSelectedOperation(SelectedItems);
      ExecuteRemoveOperation(op);
    }

    void SelectAllRipOffs() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectAllRipOffsOperation();
      ExecuteSelectOperation(op);
    }

    void SelectRipOffsOfSelected() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op =
          Playlist::Item::CreateSelectRipOffsOfSelectedOperation(SelectedItems);
      ExecuteSelectOperation(op);
    }

    void SelectRipOffsInSelected() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op =
          Playlist::Item::CreateSelectRipOffsInSelectedOperation(SelectedItems);
      ExecuteSelectOperation(op);
    }

    void SelectSameTypesOfSelected() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op =
          Playlist::Item::CreateSelectTypesOfSelectedOperation(SelectedItems);
      ExecuteSelectOperation(op);
    }

    void SelectSameFilesOfSelected() const override
    {
      const Playlist::Item::SelectionOperation::Ptr op =
          Playlist::Item::CreateSelectFilesOfSelectedOperation(SelectedItems);
      ExecuteSelectOperation(op);
    }

    void CopyPathToClipboard() const override
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const QStringList paths = model->GetItemsPaths(*SelectedItems);
      QApplication::clipboard()->setText(paths.join(LINE_BREAK));
    }

    void ShowAllStatistic() const override
    {
      const Playlist::Item::StatisticTextNotification::Ptr result = CreateStatisticNotification();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation(result);
      ExecuteNotificationOperation(op);
    }

    void ShowStatisticOfSelected() const override
    {
      const Playlist::Item::StatisticTextNotification::Ptr result = CreateStatisticNotification();
      const Playlist::Item::TextResultOperation::Ptr op =
          Playlist::Item::CreateCollectStatisticOperation(SelectedItems, result);
      ExecuteNotificationOperation(op);
    }

    void ExportAll() const override
    {
      if (const Playlist::Item::Conversion::Options::Ptr params = UI::GetExportParameters(View))
      {
        ExecuteConvertAllOperation(*params);
      }
    }

    void ExportSelected() const override
    {
      if (const Playlist::Item::Conversion::Options::Ptr params = UI::GetExportParameters(View))
      {
        ExecuteConvertOperation(*params);
      }
    }

    void ConvertSelected() const override
    {
      if (const Playlist::Item::Conversion::Options::Ptr params = UI::GetConvertParameters(View))
      {
        ExecuteConvertOperation(*params);
      }
    }

    void SaveAsSelected() const override
    {
      const Playlist::Item::Data::Ptr item = GetSelectedItem();
      if (const Playlist::Item::Conversion::Options::Ptr params = UI::GetSaveAsParameters(*item))
      {
        ExecuteConvertOperation(*params);
      }
    }

    void SelectFound() const override
    {
      if (auto op = Playlist::UI::ExecuteSearchDialog(View))
      {
        ExecuteSelectOperation(std::move(op));
      }
    }

    void SelectFoundInSelected() const override
    {
      if (auto op = Playlist::UI::ExecuteSearchDialog(View, SelectedItems))
      {
        ExecuteSelectOperation(std::move(op));
      }
    }

    void ShowPropertiesOfSelected() const override
    {
      Playlist::UI::ExecutePropertiesDialog(View, Controller.GetModel(), *SelectedItems);
    }

    void ShuffleAll() const override
    {
      const Playlist::Item::StorageModifyOperation::Ptr op = MakePtr<ShuffleOperation>();
      Controller.GetModel()->PerformOperation(op);
    }

  private:
    std::unique_ptr<QMenu> CreateMenu()
    {
      switch (const std::size_t items = SelectedItems->size())
      {
      case 0:
        return std::unique_ptr<QMenu>(new NoItemsContextMenu(View, *this));
      case 1:
        return std::unique_ptr<QMenu>(new SingleItemContextMenu(View, *this));
      default:
        return std::unique_ptr<QMenu>(new MultipleItemsContextMenu(View, *this, items));
      }
    }

    void ExecuteRemoveOperation(Playlist::Item::SelectionOperation::Ptr op) const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      Require(
          connect(op.get(), &Playlist::Item::SelectionOperation::ResultAcquired, model, &Playlist::Model::RemoveItems));
      model->PerformOperation(std::move(op));
    }

    void ExecuteSelectOperation(Playlist::Item::SelectionOperation::Ptr op) const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      Require(connect(op.get(), &Playlist::Item::SelectionOperation::ResultAcquired, &View,
                      &Playlist::UI::TableView::SelectItems));
      model->PerformOperation(std::move(op));
    }

    void ExecuteNotificationOperation(Playlist::Item::TextResultOperation::Ptr op) const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      Require(connect(op.get(), &Playlist::Item::TextResultOperation::ResultAcquired, &Controller,
                      &Playlist::Controller::ShowNotification));
      model->PerformOperation(std::move(op));
    }

    void ExecuteConvertAllOperation(const Playlist::Item::Conversion::Options& opts) const
    {
      const Playlist::Item::ConversionResultNotification::Ptr result = CreateConversionResultNotification();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateConvertOperation(opts, result);
      ExecuteNotificationOperation(op);
    }

    void ExecuteConvertOperation(const Playlist::Item::Conversion::Options& opts) const
    {
      const Playlist::Item::ConversionResultNotification::Ptr result = CreateConversionResultNotification();
      const Playlist::Item::TextResultOperation::Ptr op =
          Playlist::Item::CreateConvertOperation(SelectedItems, opts, result);
      ExecuteNotificationOperation(op);
    }

    Playlist::Item::Data::Ptr GetSelectedItem() const
    {
      assert(SelectedItems->size() == 1);
      return Controller.GetModel()->GetItem(*SelectedItems->begin());
    }

  private:
    Playlist::UI::TableView& View;
    Playlist::Controller& Controller;
    // data
    Playlist::Model::IndexSet::Ptr SelectedItems;
  };
}  // namespace

namespace Playlist::UI
{
  ItemsContextMenu::ItemsContextMenu(QObject& parent)
    : QObject(&parent)
  {}

  void ExecuteContextMenu(const QPoint& pos, TableView& view, Controller& playlist)
  {
    ItemsContextMenuImpl menu(view, playlist);
    menu.Exec(pos);
  }
}  // namespace Playlist::UI
