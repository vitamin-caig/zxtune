/*
Abstract:
  Conversion setup dialog

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "setup_conversion.h"
#include "setup_conversion.ui.h"
#include "filename_template.h"
#include "supported_formats.h"
#include "mp3_settings.h"
//qt includes
#include <QtGui/QPushButton>

namespace
{
  enum
  {
    TEMPLATE_PAGE = 0,
    FORMAT_PAGE = 1,
    SETTINGS_PAGE = 2
  };

  class SetupConversionDialogImpl : public UI::SetupConversionDialog
                                  , private Ui::SetupConversion
  {
  public:
    explicit SetupConversionDialogImpl(QWidget& parent)
      : UI::SetupConversionDialog(parent)
      , TargetTemplate(UI::FilenameTemplateWidget::Create(*this))
      , TargetFormat(UI::SupportedFormatsWidget::Create(*this))
    {
      //setup self
      setupUi(this);
      toolBox->insertItem(TEMPLATE_PAGE, TargetTemplate, QString());
      toolBox->insertItem(FORMAT_PAGE, TargetFormat, QString());
      QWidget* const settingsWidget = toolBox->widget(SETTINGS_PAGE);
      MP3Settings = UI::MP3SettingsWidget::Create(*settingsWidget);

      connect(TargetTemplate, SIGNAL(SettingsChanged()), SLOT(UpdateDescriptions()));
      connect(TargetFormat, SIGNAL(SettingsChanged()), SLOT(UpdateDescriptions()));
      connect(MP3Settings, SIGNAL(SettingsChanged()), SLOT(UpdateDescriptions()));

      toolBox->setCurrentIndex(TEMPLATE_PAGE);
      UpdateDescriptions();
    }

    virtual void UpdateDescriptions()
    {
      UpdateTargetDescription();
      UpdateFormatDescription();
      UpdateSettingsDescription();
    }
  private:
    void UpdateTargetDescription()
    {
      const QString templ = TargetTemplate->GetFilenameTemplate();
      toolBox->setItemText(TEMPLATE_PAGE, templ);
      if (QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok))
      {
        okButton->setEnabled(!templ.isEmpty());
      }
    }

    void UpdateFormatDescription()
    {
      toolBox->setItemText(FORMAT_PAGE, TargetFormat->GetDescription());
    }

    void UpdateSettingsDescription()
    {
      bool enabledSettings = true;
      const String type = TargetFormat->GetSelectedId();
      const bool isMP3 = type == MP3Settings->GetBackendId();
      MP3Settings->setVisible(isMP3);
      if (isMP3)
      {
        toolBox->setItemText(SETTINGS_PAGE, MP3Settings->GetDescription());
      }
      else
      {
        toolBox->setItemText(SETTINGS_PAGE, tr("No options"));
        enabledSettings = false;
      }
      toolBox->setItemEnabled(SETTINGS_PAGE, enabledSettings);
    }
  private:
    UI::FilenameTemplateWidget* const TargetTemplate;
    UI::SupportedFormatsWidget* const TargetFormat;
    UI::MP3SettingsWidget* MP3Settings;
  };
}

namespace UI
{
  SetupConversionDialog::SetupConversionDialog(QWidget& parent)
    : QDialog(&parent)
  {
  }

  SetupConversionDialog* SetupConversionDialog::Create(QWidget& parent)
  {
    return new SetupConversionDialogImpl(parent);
  }
}
