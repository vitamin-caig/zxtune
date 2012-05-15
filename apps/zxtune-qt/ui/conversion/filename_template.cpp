/*
Abstract:
  Filename template building widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "filename_template.h"
#include "filename_template.ui.h"
#include "parameters.h"
#include "supp/options.h"
#include "ui/state.h"
#include "ui/utils.h"
#include "ui/tools/filedialog.h"
//qt includes
#include <QtGui/QCloseEvent>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QVBoxLayout>

namespace
{
  void UpdateRecent(QComboBox& box)
  {
    //emulate QComboBox::returnPressed
    const QString txt = box.currentText();
    const int idx = box.findText(txt);
    if (-1 != idx)
    { 
      box.removeItem(idx);
    }
    box.insertItem(0, txt);
  }

  class FilenameTemplateWidgetImpl : public UI::FilenameTemplateWidget
                                   , private Ui::FilenameTemplate
  {
  public:
    explicit FilenameTemplateWidgetImpl(QWidget& parent)
      : UI::FilenameTemplateWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , State(UI::State::Create(Parameters::ZXTuneQT::UI::Export::NAMESPACE_NAME))
    {
      //setup self
      setupUi(this);
      State->AddWidget(*DirectoryName);
      State->AddWidget(*FileTemplate);

      connect(DirectoryName, SIGNAL(editTextChanged(const QString&)), SIGNAL(SettingsChanged()));
      connect(FileTemplate, SIGNAL(editTextChanged(const QString&)), SIGNAL(SettingsChanged()));

      State->Load();
    }

    virtual ~FilenameTemplateWidgetImpl()
    {
      UpdateRecentItemsLists();
      State->Save();
    }

    virtual QString GetFilenameTemplate() const
    {
      const QString name = FileTemplate->currentText();
      if (0 == name.size())
      {
        return name;
      }
      const QString dir = DirectoryName->currentText();
      if (dir.size() != 0)
      {
        return dir + '/' + name;
      }
      return name;
    }

    virtual void OnBrowseDirectory()
    {
      QString dir = DirectoryName->currentText();
      if (UI::OpenFolderDialog(dirSelectionGroup->title(), dir))
      {
        QLineEdit* const editor = DirectoryName->lineEdit();
        editor->setText(dir);
      }
    }

    virtual void OnClickHint(const QString& hint)
    {
      QLineEdit* const editor = FileTemplate->lineEdit();
      editor->setText(editor->text() + hint);
    }
  private:
    void UpdateRecentItemsLists() const
    {
      UpdateRecent(*FileTemplate);
      UpdateRecent(*DirectoryName);
    }
  private:
    const Parameters::Container::Ptr Options;
    UI::State::Ptr State;
  };

  class FilenameTemplateDialog : public QDialog
  {
  public:
    explicit FilenameTemplateDialog(QWidget& parent)
      : QDialog(&parent)
      , TemplateBuilder(0)
    {
      TemplateBuilder = UI::FilenameTemplateWidget::Create(*this);
      QDialogButtonBox* const buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
      this->connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
      this->connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
      QVBoxLayout* const layout = new QVBoxLayout(this);
      layout->addWidget(TemplateBuilder);
      layout->addWidget(buttons);
      setWindowTitle(TemplateBuilder->windowTitle());
    }

    QString GetResult() const
    {
      return TemplateBuilder->GetFilenameTemplate();
    }
  private:
    UI::FilenameTemplateWidget* TemplateBuilder;
  };
}

namespace UI
{
  FilenameTemplateWidget::FilenameTemplateWidget(QWidget& parent) : QWidget(&parent)
  {
  }

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
      QMessageBox warning(QMessageBox::Critical, QString::fromUtf8("Failed to export"),
        QString::fromUtf8("Filename template is empty"), QMessageBox::Ok);
      warning.exec();
    }
    return false;
  }
}
