/**
* 
* @file
*
* @brief Conversion setup dialog implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "setup_conversion.h"
#include "setup_conversion.ui.h"
#include "filename_template.h"
#include "supported_formats.h"
#include "mp3_settings.h"
#include "ogg_settings.h"
#include "flac_settings.h"
#include "supp/options.h"
#include "ui/state.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <parameters/tools.h>
#include <sound/backends_parameters.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QThread>
#include <QtGui/QCloseEvent>
#include <QtGui/QPushButton>

namespace
{
  enum
  {
    TEMPLATE_PAGE = 0,
    FORMAT_PAGE = 1,
    SETTINGS_PAGE = 2
  };

  const uint_t MULTITHREAD_BUFFERS_COUNT = 1000;//20 sec usually

  bool HasMultithreadEnvironment()
  {
    return QThread::idealThreadCount() > 1;
  }

  template<class ValueType>
  class TemporaryProperty
  {
  public:
    TemporaryProperty(Parameters::Container& param, const Parameters::NameType& name, const ValueType& newValue)
      : Param(param)
      , Name(name)
      , OldValue(newValue)
      , Changed(Param.FindValue(Name, OldValue) && OldValue != newValue)
    {
      if (Changed)
      {
        Param.SetValue(Name, newValue);
      }
    }

    ~TemporaryProperty()
    {
      if (Changed)
      {
        Param.SetValue(Name, OldValue);
      }
    }
  private:
    Parameters::Container& Param;
    const Parameters::NameType Name;
    ValueType OldValue;
    const bool Changed;
  };

  class SetupConversionDialogImpl : public UI::SetupConversionDialog
                                  , private UI::Ui_SetupConversionDialog
  {
  public:
    explicit SetupConversionDialogImpl(QWidget& parent)
      : UI::SetupConversionDialog(parent)
      , Options(GlobalOptions::Instance().Get())
      , TargetTemplate(UI::FilenameTemplateWidget::Create(*this))
      , TargetFormat(UI::SupportedFormatsWidget::Create(*this))
    {
      //setup self
      setupUi(this);
      State = UI::State::Create(*this);
      toolBox->insertItem(TEMPLATE_PAGE, TargetTemplate, QString());
      toolBox->insertItem(FORMAT_PAGE, TargetFormat, QString());

      AddBackendSettingsWidget(&UI::CreateMP3SettingsWidget);
      AddBackendSettingsWidget(&UI::CreateOGGSettingsWidget);
      AddBackendSettingsWidget(&UI::CreateFLACSettingsWidget);

      Require(connect(TargetTemplate, SIGNAL(SettingsChanged()), SLOT(UpdateDescriptions())));
      Require(connect(TargetFormat, SIGNAL(SettingsChanged()), SLOT(UpdateDescriptions())));

      Require(connect(buttonBox, SIGNAL(accepted()), SLOT(accept())));
      Require(connect(buttonBox, SIGNAL(rejected()), SLOT(reject())));

      toolBox->setCurrentIndex(TEMPLATE_PAGE);
      useMultithreading->setEnabled(HasMultithreadEnvironment());
      using namespace Parameters;
      BooleanValue::Bind(*useMultithreading, *Options, ZXTune::Sound::Backends::File::BUFFERS, false, MULTITHREAD_BUFFERS_COUNT);

      UpdateDescriptions();
      State->Load();
    }

    virtual bool Execute(Playlist::Item::Conversion::Options& opts)
    {
      if (exec())
      {
        opts.Type = TargetFormat->GetSelectedId();
        opts.FilenameTemplate = FromQString(TargetTemplate->GetFilenameTemplate());
        Options->SetValue(Parameters::ZXTune::Sound::Backends::File::FILENAME, ToLocal(opts.FilenameTemplate));
        const TemporaryProperty<Parameters::IntType> disableLoop(*Options, Parameters::ZXTune::Sound::LOOPED, 0);
        opts.Params = GlobalOptions::Instance().GetSnapshot();
        return true;
      }
      else
      {
        return false;
      }
    }

    virtual void UpdateDescriptions()
    {
      UpdateTargetDescription();
      UpdateFormatDescription();
      UpdateSettingsDescription();
    }

    //QWidgets virtuals
    virtual void closeEvent(QCloseEvent* event)
    {
      State->Save();
      event->accept();
    }
  private:
    void AddBackendSettingsWidget(UI::BackendSettingsWidget* factory(QWidget&))
    {
      QWidget* const settingsWidget = toolBox->widget(SETTINGS_PAGE);
      UI::BackendSettingsWidget* const result = factory(*settingsWidget);
      formatSettingsLayout->addWidget(result);
      Require(connect(result, SIGNAL(SettingsChanged()), SLOT(UpdateDescriptions())));
      BackendSettings[result->GetBackendId()] = result;
    }

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
      const String type = TargetFormat->GetSelectedId();
      std::for_each(BackendSettings.begin(), BackendSettings.end(),
        boost::bind(&QWidget::setVisible, boost::bind(&BackendIdToSettings::value_type::second, _1), false));
      const BackendIdToSettings::const_iterator it = BackendSettings.find(type);
      if (it != BackendSettings.end())
      {
        it->second->setVisible(true);
        toolBox->setItemText(SETTINGS_PAGE, it->second->GetDescription());
        toolBox->setItemEnabled(SETTINGS_PAGE, true);
      }
      else
      {
        toolBox->setItemText(SETTINGS_PAGE, UI::SetupConversionDialog::tr("No options"));
        toolBox->setItemEnabled(SETTINGS_PAGE, false);
      }
    }
  private:
    const Parameters::Container::Ptr Options;
    UI::State::Ptr State;
    UI::FilenameTemplateWidget* const TargetTemplate;
    UI::SupportedFormatsWidget* const TargetFormat;
    typedef std::map<String, UI::BackendSettingsWidget*> BackendIdToSettings;
    BackendIdToSettings BackendSettings;
  };
}

namespace UI
{
  SetupConversionDialog::SetupConversionDialog(QWidget& parent)
    : QDialog(&parent)
  {
  }

  SetupConversionDialog::Ptr SetupConversionDialog::Create(QWidget& parent)
  {
    return SetupConversionDialog::Ptr(new SetupConversionDialogImpl(parent));
  }

  bool GetConversionParameters(QWidget& parent, Playlist::Item::Conversion::Options& opts)
  {
    const SetupConversionDialog::Ptr dialog = SetupConversionDialog::Create(parent);
    return dialog->Execute(opts);
  }
}
