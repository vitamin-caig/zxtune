/**
 *
 * @file
 *
 * @brief Supported formats pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/conversion/supported_formats.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/conversion/parameters.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "supported_formats.ui.h"

#include "sound/service.h"
#include "strings/map.h"

#include "contract.h"
#include "string_view.h"

#include <QtWidgets/QRadioButton>

namespace
{
  class FileBackendsSet
  {
  public:
    explicit FileBackendsSet(Parameters::Accessor::Ptr options)
    {
      const auto service = Sound::CreateFileService(std::move(options));
      for (const auto& info : service->EnumerateBackends())
      {
        Ids.emplace(info->Id(), info->Status());
      }
    }

    Strings::Array GetAvailable() const
    {
      Strings::Array result;
      for (const auto& id : Ids)
      {
        if (!id.second)
        {
          result.push_back(id.first);
        }
      }
      return result;
    }

    Error GetStatus(StringView id) const
    {
      return Ids.Get(id);
    }

  private:
    Strings::ValueMap<Error> Ids;
  };

  const auto TYPE_WAV = "wav"sv;
  const auto TYPE_MP3 = "mp3"sv;
  const auto TYPE_OGG = "ogg"sv;
  const auto TYPE_FLAC = "flac"sv;

  class SupportedFormats
    : public UI::SupportedFormatsWidget
    , private Ui::SupportedFormats
  {
  public:
    explicit SupportedFormats(QWidget& parent)
      : UI::SupportedFormatsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , Backends(Options)
    {
      // setup self
      setupUi(this);

      Buttons[TYPE_WAV] = selectWAV;
      Buttons[TYPE_MP3] = selectMP3;
      Buttons[TYPE_OGG] = selectOGG;
      Buttons[TYPE_FLAC] = selectFLAC;

      std::for_each(Buttons.begin(), Buttons.end(), [this](auto button) { SetupButton(button); });
      // fixup
      for (const auto& id2b : Buttons)
      {
        if (id2b.second->isChecked())
        {
          selectWAV->setChecked(!id2b.second->isEnabled());
          return;
        }
      }
      selectWAV->setChecked(true);
    }

    StringView GetSelectedId() const override
    {
      for (const auto& id2b : Buttons)
      {
        if (id2b.second->isChecked())
        {
          return id2b.first;
        }
      }
      return {};
    }

    QString GetDescription() const override
    {
      for (const auto& id2b : Buttons)
      {
        if (id2b.second->isChecked())
        {
          return id2b.second->text();
        }
      }
      return {};
    }

  private:
    using IdToButton = std::map<StringView, QRadioButton*>;

    void SetupButton(IdToButton::value_type but)
    {
      Require(connect(but.second, &QRadioButton::toggled, this, [this](bool) { emit SettingsChanged(); }));
      if (const Error status = Backends.GetStatus(but.first))
      {
        but.second->setEnabled(false);
        but.second->setToolTip(ToQString(status.GetText()));
      }
      else
      {
        but.second->setEnabled(true);
      }
      Parameters::ExclusiveValue::Bind(*but.second, *Options, Parameters::ZXTuneQT::UI::Export::TYPE, but.first);
    }

  private:
    const Parameters::Container::Ptr Options;
    const FileBackendsSet Backends;
    IdToButton Buttons;
  };
}  // namespace

namespace UI
{
  SupportedFormatsWidget::SupportedFormatsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  SupportedFormatsWidget* SupportedFormatsWidget::Create(QWidget& parent)
  {
    return new SupportedFormats(parent);
  }

  Strings::Array SupportedFormatsWidget::GetSoundTypes()
  {
    return FileBackendsSet(GlobalOptions::Instance().Get()).GetAvailable();
  }
}  // namespace UI
