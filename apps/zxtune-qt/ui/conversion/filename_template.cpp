/**
 *
 * @file
 *
 * @brief Filename template building widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "filename_template.h"
#include "filename_template.ui.h"
#include "parameters.h"
#include "supp/options.h"
#include "ui/state.h"
#include "ui/tools/filedialog.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
// library includes
#include <io/providers_parameters.h>
// qt includes
#include <QtGui/QCloseEvent>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QVBoxLayout>
// std includes
#include <utility>

namespace
{
  void UpdateRecent(QComboBox& box)
  {
    // Do not notify subscribers - called from dtor
    const AutoBlockSignal blockstate(box);
    // emulate QComboBox::returnPressed
    const QString txt = box.currentText();
    if (const int idx = box.findText(txt))
    {
      if (-1 != idx)
      {
        box.removeItem(idx);
      }
      box.insertItem(0, txt);
    }
  }

  class FilenameTemplateWidgetImpl
    : public UI::FilenameTemplateWidget
    , private UI::Ui_FilenameTemplateWidget
  {
  public:
    explicit FilenameTemplateWidgetImpl(QWidget& parent)
      : UI::FilenameTemplateWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , State(UI::State::Create(Parameters::ZXTuneQT::UI::Export::NAMESPACE_NAME))
    {
      // setup self
      setupUi(this);
      State->AddWidget(*DirectoryName);
      State->AddWidget(*FileTemplate);

      auto onChangeSettings = [this](const QString&) { emit SettingsChanged(); };
      Require(connect(DirectoryName, &QComboBox::editTextChanged, this, onChangeSettings));
      Require(connect(FileTemplate, &QComboBox::editTextChanged, this, onChangeSettings));
      Require(connect(browseDirButton, &QToolButton::clicked, this, &FilenameTemplateWidgetImpl::OnBrowseDirectory));
      for (auto* hint : {hintFullPath, hintPath, hintFilename, hintExtension, hintSubpath, hintContainer, hintType,
                         hintSize, hintCRC, hintFixedCRC, hintTitle, hintAuthor, hintComment, hintProgram, hintVersion})
      {
        Require(connect(hint, &QLabel::linkActivated, this, &FilenameTemplateWidgetImpl::OnClickHint));
      }

      State->Load();

      using namespace Parameters;
      IntegerValue::Bind(*overwriteExisting, *Options, ZXTune::IO::Providers::File::OVERWRITE_EXISTING,
                         ZXTune::IO::Providers::File::OVERWRITE_EXISTING_DEFAULT);
      BooleanValue::Bind(*createDirectories, *Options, ZXTune::IO::Providers::File::CREATE_DIRECTORIES,
                         ZXTune::IO::Providers::File::CREATE_DIRECTORIES_DEFAULT);
    }

    ~FilenameTemplateWidgetImpl() override
    {
      UpdateRecentItemsLists();
      State->Save();
    }

    QString GetFilenameTemplate() const override
    {
      auto name = FileTemplate->currentText();
      if (0 == name.size())
      {
        return name;
      }
      const QString dir = DirectoryName->currentText();
      if (dir.size() != 0)
      {
        static const QLatin1Char SEPARATOR('/');
        return dir.endsWith(SEPARATOR) ? dir + name : dir + SEPARATOR + name;
      }
      return name;
    }

  private:
    void OnBrowseDirectory()
    {
      QString dir = DirectoryName->currentText();
      if (UI::OpenFolderDialog(dirSelectionGroup->title(), dir))
      {
        QLineEdit* const editor = DirectoryName->lineEdit();
        editor->setText(dir);
      }
    }

    void OnClickHint(const QString& hint)
    {
      QLineEdit* const editor = FileTemplate->lineEdit();
      editor->setText(editor->text() + hint);
    }

    void UpdateRecentItemsLists() const
    {
      UpdateRecent(*FileTemplate);
      UpdateRecent(*DirectoryName);
    }

  private:
    const Parameters::Container::Ptr Options;
    const UI::State::Ptr State;
  };

  class FilenameTemplateDialog : public QDialog
  {
  public:
    explicit FilenameTemplateDialog(QWidget& parent)
      : QDialog(&parent)
    {
      TemplateBuilder = UI::FilenameTemplateWidget::Create(*this);
      auto* const buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
      Require(connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept));
      Require(connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject));
      auto* const layout = new QVBoxLayout(this);
      layout->setContentsMargins(4, 4, 4, 4);
      layout->setSpacing(4);
      layout->addWidget(TemplateBuilder);
      layout->addWidget(buttons);
      setWindowTitle(TemplateBuilder->windowTitle());
    }

    QString GetResult() const
    {
      return TemplateBuilder->GetFilenameTemplate();
    }

  private:
    UI::FilenameTemplateWidget* TemplateBuilder = nullptr;
  };
}  // namespace

namespace UI
{
  FilenameTemplateWidget::FilenameTemplateWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  FilenameTemplateWidget* FilenameTemplateWidget::Create(QWidget& parent)
  {
    return new FilenameTemplateWidgetImpl(parent);
  }

  bool GetFilenameTemplate(QWidget& parent, QString& result)
  {
    FilenameTemplateDialog dialog(parent);
    if (dialog.exec())
    {
      const QString res = dialog.GetResult();
      if (res.size())
      {
        result = res;
        return true;
      }
      QMessageBox warning(QMessageBox::Critical, UI::FilenameTemplateWidget::tr("Invalid parameter"),
                          UI::FilenameTemplateWidget::tr("Filename template is empty"), QMessageBox::Ok);
      warning.exec();
    }
    return false;
  }
}  // namespace UI
