/**
 *
 * @file
 *
 * @brief Conversion setup dialog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "setup_conversion.h"
#include "filename_template.h"
#include "flac_settings.h"
#include "mp3_settings.h"
#include "ogg_settings.h"
#include "setup_conversion.ui.h"
#include "supp/options.h"
#include "supported_formats.h"
#include "ui/state.h"
#include "ui/tools/filedialog.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <io/api.h>
#include <io/providers_parameters.h>
#include <parameters/merged_accessor.h>
#include <sound/backends_parameters.h>
// qt includes
#include <QtCore/QThread>
#include <QtGui/QCloseEvent>
#include <QtWidgets/QPushButton>

namespace
{
  enum
  {
    TEMPLATE_PAGE = 0,
    FORMAT_PAGE = 1,
    SETTINGS_PAGE = 2
  };

  const uint_t MULTITHREAD_BUFFERS_COUNT = 1000;  // 20 sec usually

  bool HasMultithreadEnvironment()
  {
    return QThread::idealThreadCount() > 1;
  }

  Playlist::Item::Conversion::Options::Ptr CreateOptions(StringView type, const QString& filenameTemplate,
                                                         Parameters::Accessor::Ptr params)
  {
    return MakePtr<Playlist::Item::Conversion::Options>(String{type}, FromQString(filenameTemplate), std::move(params));
  }

  class SetupConversionDialogImpl
    : public UI::SetupConversionDialog
    , private UI::Ui_SetupConversionDialog
  {
  public:
    explicit SetupConversionDialogImpl(QWidget& parent)
      : UI::SetupConversionDialog(parent)
      , Options(GlobalOptions::Instance().Get())
      , TargetTemplate(UI::FilenameTemplateWidget::Create(*this))
      , TargetFormat(UI::SupportedFormatsWidget::Create(*this))
    {
      // setup self
      setupUi(this);
      State = UI::State::Create(*this);
      toolBox->insertItem(TEMPLATE_PAGE, TargetTemplate, QString());
      toolBox->insertItem(FORMAT_PAGE, TargetFormat, QString());

      AddBackendSettingsWidget(&UI::CreateMP3SettingsWidget);
      AddBackendSettingsWidget(&UI::CreateOGGSettingsWidget);
      AddBackendSettingsWidget(&UI::CreateFLACSettingsWidget);

      Require(connect(TargetTemplate, &UI::FilenameTemplateWidget::SettingsChanged, this,
                      &SetupConversionDialogImpl::UpdateDescriptions));
      Require(connect(TargetFormat, &UI::SupportedFormatsWidget::SettingsChanged, this,
                      &SetupConversionDialogImpl::UpdateDescriptions));

      Require(connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept));
      Require(connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject));

      toolBox->setCurrentIndex(TEMPLATE_PAGE);
      useMultithreading->setEnabled(HasMultithreadEnvironment());
      Parameters::BooleanValue::Bind(*useMultithreading, *Options, Parameters::ZXTune::Sound::Backends::File::BUFFERS,
                                     false, MULTITHREAD_BUFFERS_COUNT);

      UpdateDescriptions();
      State->Load();
    }

    Playlist::Item::Conversion::Options::Ptr Execute() override
    {
      if (exec())
      {
        return CreateOptions(TargetFormat->GetSelectedId(), TargetTemplate->GetFilenameTemplate(),
                             GlobalOptions::Instance().GetSnapshot());
      }
      else
      {
        return {};
      }
    }

    // QWidgets virtuals
    void closeEvent(QCloseEvent* event) override
    {
      State->Save();
      event->accept();
    }

  private:
    void UpdateDescriptions()
    {
      UpdateTargetDescription();
      UpdateFormatDescription();
      UpdateSettingsDescription();
    }

    void AddBackendSettingsWidget(UI::BackendSettingsWidget* factory(QWidget&))
    {
      QWidget* const settingsWidget = toolBox->widget(SETTINGS_PAGE);
      UI::BackendSettingsWidget* const result = factory(*settingsWidget);
      formatSettingsLayout->addWidget(result);
      Require(connect(result, &UI::BackendSettingsWidget::SettingsChanged, this,
                      &SetupConversionDialogImpl::UpdateDescriptions));
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
      for (auto& wid : BackendSettings)
      {
        wid.second->setVisible(false);
      }
      const auto it = BackendSettings.find(type);
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
    using BackendIdToSettings = std::map<String, UI::BackendSettingsWidget*>;
    BackendIdToSettings BackendSettings;
  };

  QString GetDefaultFilename(const Playlist::Item::Data& item)
  {
    const auto& filePath = item.GetFilePath();
    const auto id = IO::ResolveUri(filePath);
    return ToQString(id->Filename());
  }

  QString FixExtension(const QString& filename, const QString& extension)
  {
    const int pos = filename.indexOf('.');
    if (pos != -1)
    {
      return filename.left(pos + 1) + extension;
    }
    else
    {
      return filename + '.' + extension;
    }
  }

  class Formats
  {
  public:
    explicit Formats(const QString& type)
    {
      AddRawType(type);
      AddSoundTypes();
    }

    const QStringList& GetFilters() const
    {
      return Filters;
    }

    String GetType(int idx) const
    {
      return Types[idx];
    }

  private:
    void AddRawType(const QString& type)
    {
      Types.emplace_back("");
      Filters << MakeFilter(type);
    }

    void AddSoundTypes()
    {
      const Strings::Array& types = UI::SupportedFormatsWidget::GetSoundTypes();
      for (const auto& type : types)
      {
        Types.push_back(type);
        Filters << MakeFilter(ToQString(type));
      }
    }

    static QString MakeFilter(const QString& type)
    {
      return QString::fromLatin1("%1 (*.%1)").arg(type);
    }

  private:
    Strings::Array Types;
    QStringList Filters;
  };

  Parameters::Accessor::Ptr CreateSaveAsParameters()
  {
    // force simpliest mode
    const Parameters::Accessor::Ptr base = GlobalOptions::Instance().GetSnapshot();
    const Parameters::Container::Ptr overriden = Parameters::Container::Create();
    overriden->SetValue(Parameters::ZXTune::IO::Providers::File::OVERWRITE_EXISTING, 1);
    overriden->SetValue(Parameters::ZXTune::IO::Providers::File::SANITIZE_NAMES, 0);
    overriden->SetValue(Parameters::ZXTune::IO::Providers::File::CREATE_DIRECTORIES, 0);
    return Parameters::CreateMergedAccessor(overriden, base);
  }
}  // namespace

namespace UI
{
  SetupConversionDialog::SetupConversionDialog(QWidget& parent)
    : QDialog(&parent)
  {}

  SetupConversionDialog::Ptr SetupConversionDialog::Create(QWidget& parent)
  {
    return MakePtr<SetupConversionDialogImpl>(parent);
  }

  Playlist::Item::Conversion::Options::Ptr GetExportParameters(QWidget& parent)
  {
    QString nameTemplate;
    if (UI::GetFilenameTemplate(parent, nameTemplate))
    {
      return CreateOptions(String(), nameTemplate, GlobalOptions::Instance().Get());
    }
    else
    {
      return {};
    }
  }

  Playlist::Item::Conversion::Options::Ptr GetConvertParameters(QWidget& parent)
  {
    const SetupConversionDialog::Ptr dialog = SetupConversionDialog::Create(parent);
    return dialog->Execute();
  }

  Playlist::Item::Conversion::Options::Ptr GetSaveAsParameters(const Playlist::Item::Data& item)
  {
    const auto& state = item.GetState();
    if (!state.IsReady() || state.GetIfError())
    {
      return {};
    }
    const QString type = ToQString(item.GetType()).toLower();
    const Formats formats(type);
    // QFileDialog automatically change extension only when filter is selected
    QString filename = FixExtension(GetDefaultFilename(item), type);
    int typeIndex = 0;
    if (UI::SaveFileDialog(QString(), type, formats.GetFilters(), filename, &typeIndex))
    {
      return CreateOptions(formats.GetType(typeIndex), filename, CreateSaveAsParameters());
    }
    else
    {
      return {};
    }
  }
}  // namespace UI
