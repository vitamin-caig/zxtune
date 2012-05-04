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
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/filedialog.h"
//qt includes
#include <QtGui/QCloseEvent>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QVBoxLayout>

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace UI
    {
      namespace Export
      {
        const Char RECENT_DIRECTORIES[] =
        {
          'z','x','t','u','n','e','-','q','t','.','u','i','.','e','x','p','o','r','t','.','r','e','c','e','n','t','_','d','i','r','e','c','t','o','r','i','e','s','\0'
        };

        const Char RECENT_TEMPLATES[] =
        {
          'z','x','t','u','n','e','-','q','t','.','u','i','.','e','x','p','o','r','t','.','r','e','c','e','n','t','_','t','e','m','p','l','a','t','e','s','\0'
        };
      }
    }
  }
}

namespace
{
  const Char STRINGS_DELIMITER = '\n';

  const Char DEFAULT_TEMPLATES[] = {
    '[','F','u','l','l','p','a','t','h',']','.','[','T','y','p','e',']','\n',
    '[','F','i','l','e','n','a','m','e',']','_','[','S','u','b','p','a','t','h',']','.','[','T','y','p','e',']','\n',
    '[','C','R','C',']','.','[','T','y','p','e',']'
  };

  QStringList GetAllItems(const QComboBox& box)
  {
    QStringList result;
    for (int idx = 0; idx < box.count(); ++idx)
    {
      result.push_back(box.itemText(idx));
    }
    return result;
  }

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
    {
      //setup self
      setupUi(this);

      connect(directoryName, SIGNAL(editTextChanged(const QString&)), SIGNAL(SettingsChanged()));
      connect(fileTemplate, SIGNAL(editTextChanged(const QString&)), SIGNAL(SettingsChanged()));

      LoadSettings();
    }

    virtual ~FilenameTemplateWidgetImpl()
    {
      UpdateRecentItemsLists();
      SaveSettings();
    }

    virtual QString GetFilenameTemplate() const
    {
      const QString name = fileTemplate->currentText();
      if (0 == name.size())
      {
        return name;
      }
      const QString dir = directoryName->currentText();
      if (dir.size() != 0)
      {
        return dir + '/' + name;
      }
      return name;
    }

    virtual void OnBrowseDirectory()
    {
      QString dir = directoryName->currentText();
      if (FileDialog::Instance().OpenFolder(tr("Select target directory"), dir))
      {
        QLineEdit* const editor = directoryName->lineEdit();
        editor->setText(dir);
      }
    }

    virtual void OnClickHint(const QString& hint)
    {
      QLineEdit* const editor = fileTemplate->lineEdit();
      editor->setText(editor->text() + hint);
    }
  private:
    void UpdateRecentItemsLists() const
    {
      UpdateRecent(*fileTemplate);
      UpdateRecent(*directoryName);
    }

    void LoadSettings()
    {
      LoadComboboxStrings(Parameters::ZXTuneQT::UI::Export::RECENT_DIRECTORIES, *directoryName, String());
      LoadComboboxStrings(Parameters::ZXTuneQT::UI::Export::RECENT_TEMPLATES, *fileTemplate, DEFAULT_TEMPLATES);
    }

    void LoadComboboxStrings(const Parameters::NameType& name, QComboBox& box, const Parameters::StringType& defaultValue)
    {
      Parameters::StringType str = defaultValue;
      Options->FindValue(name, str);
      const QStringList& items = ToQString(str).split(STRINGS_DELIMITER, QString::SkipEmptyParts);
      box.clear();
      box.addItems(items);
    }

    void SaveSettings() const
    {
      SaveComboboxStrings(Parameters::ZXTuneQT::UI::Export::RECENT_DIRECTORIES, *directoryName);
      SaveComboboxStrings(Parameters::ZXTuneQT::UI::Export::RECENT_TEMPLATES, *fileTemplate);
    }

    void SaveComboboxStrings(const Parameters::NameType& name, const QComboBox& box) const
    {
      const QStringList& items = GetAllItems(box);
      const QString& str = items.join(QString(STRINGS_DELIMITER));
      Options->SetValue(name, FromQString(str));
    }
  private:
    const Parameters::Container::Ptr Options;
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
      setWindowTitle(tr("Setup filename template"));
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
