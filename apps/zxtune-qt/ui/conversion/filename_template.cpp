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
#include "ui/tools/filedialog.h"
//qt includes
#include <QtGui/QDialogButtonBox>
#include <QtGui/QMessageBox>
#include <QtGui/QVBoxLayout>

namespace
{
  class FilenameTemplateWidgetImpl : public UI::FilenameTemplateWidget
                                   , private Ui::FilenameTemplate
  {
  public:
    explicit FilenameTemplateWidgetImpl(QWidget& parent)
      : UI::FilenameTemplateWidget(parent)
    {
      //setup self
      setupUi(this);
    }

    virtual QString GetFilenameTemplate() const
    {
      const QString dir = directoryName->text();
      const QString name = fileTemplate->currentText();
      const bool hasDir = 0 != dir.size();
      const bool hasName = 0 != name.size();
      if (hasName)
      {
        if (hasDir)
        {
          return dir + '/' + name;
        }
        return name;
      }
      return QString();
    }

    virtual void OnBrowseDirectory()
    {
      QString dir = directoryName->text();
      if (FileDialog::Instance().OpenFolder(QString::fromUtf8("Select target directory"), dir))
      {
        directoryName->setText(dir);
      }
    }

    virtual void OnClickHint(const QString& hint)
    {
      QLineEdit* const editor = fileTemplate->lineEdit();
      editor->setText(editor->text() + hint);
    }
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
      setWindowTitle(QString::fromUtf8("Setup filename template"));
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
