/**
 *
 * @file
 *
 * @brief ALSA settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound_alsa.h"
#include "sound_alsa.ui.h"
#include "supp/options.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <string_view.h>
// library includes
#include <debug/log.h>
#include <sound/backends/alsa.h>
#include <sound/backends_parameters.h>

namespace
{
  const Debug::Stream Dbg("UI::Preferences::Alsa");
}

namespace
{
  template<class T>
  QStringList ToStringList(const T& input)
  {
    QStringList result;
    for (const auto& item : input)
    {
      result.append(ToQString(item));
    }
    return result;
  }

  void SetComboValue(QComboBox& box, StringView val)
  {
    const auto& str = ToQString(val);
    const int items = box.count();
    for (int idx = 0; idx != items; ++idx)
    {
      const QString& item = box.itemText(idx);
      if (item == str)
      {
        box.setCurrentIndex(idx);
        return;
      }
    }
    box.setCurrentIndex(-1);
  }

  class AlsaOptionsWidget
    : public UI::AlsaSettingsWidget
    , public Ui::AlsaOptions
  {
  public:
    explicit AlsaOptionsWidget(QWidget& parent)
      : UI::AlsaSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      FillDevices();
      SelectDevice();

      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
      Parameters::IntegerValue::Bind(*latency, *Options, LATENCY, LATENCY_DEFAULT);
      Require(connect(mixers, &QComboBox::currentTextChanged, this, &AlsaOptionsWidget::MixerChanged));
      Require(connect(devices, &QComboBox::currentTextChanged, this, &AlsaOptionsWidget::DeviceChanged));
    }

    String GetBackendId() const override
    {
      static const Char ID[] = {'a', 'l', 's', 'a', '\0'};
      return ID;
    }

    QString GetDescription() const override
    {
      return nameGroup->title();
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::AlsaSettingsWidget::changeEvent(event);
    }

  private:
    void SelectDevice()
    {
      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
      const auto curDevice = Parameters::GetString(*Options, DEVICE, DEVICE_DEFAULT);
      // force fill mixers- signal is not called by previous function (even when connected before, why?)
      DeviceChanged(ToQString(curDevice));
    }

    void FillDevices()
    {
      using namespace Sound;
      for (const auto availableDevices = Alsa::EnumerateDevices(); availableDevices->IsValid();
           availableDevices->Next())
      {
        const Alsa::Device::Ptr cur = availableDevices->Get();
        Devices.emplace_back(*cur);
        devices->addItem(Devices.back().Name);
      }
    }

    void SelectMixer()
    {
      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
      const auto curMixer = Parameters::GetString(*Options, MIXER);
      SetComboValue(*mixers, curMixer);
    }

    void DeviceChanged(const QString& name)
    {
      const auto& id = FromQString(name);
      Dbg("Selecting device '{}'", id);
      const auto it = std::find_if(Devices.begin(), Devices.end(),
                                   [&name, &id](const Device& dev) { return dev.Name == name || dev.Id == id; });
      if (it != Devices.end())
      {
        devices->setCurrentIndex(it - Devices.begin());
        mixers->clear();
        mixers->addItems(it->MixerNames);
        Options->SetValue(Parameters::ZXTune::Sound::Backends::Alsa::DEVICE, it->Id);
        SelectMixer();
      }
      else
      {
        devices->setCurrentIndex(-1);
      }
    }

    void MixerChanged(const QString& name)
    {
      // while recreating combobox message with empty parameter can be passed
      if (name.size())
      {
        const String mixer = FromQString(name);
        Dbg("Selecting mixer '{}'", mixer);
        Options->SetValue(Parameters::ZXTune::Sound::Backends::Alsa::MIXER, mixer);
      }
    }

  private:
    const Parameters::Container::Ptr Options;

    struct Device
    {
      Device() = default;

      Device(const Sound::Alsa::Device& in)
        : Name(QString::fromLatin1("%1 (%2)").arg(ToQString(in.Name())).arg(ToQString(in.CardName())))
        , Id(in.Id())
        , MixerNames(ToStringList(in.Mixers()))
      {}

      QString Name;
      String Id;
      QStringList MixerNames;
    };

    using DevicesArray = std::vector<Device>;
    DevicesArray Devices;
  };
}  // namespace
namespace UI
{
  AlsaSettingsWidget::AlsaSettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {}

  BackendSettingsWidget* AlsaSettingsWidget::Create(QWidget& parent)
  {
    return new AlsaOptionsWidget(parent);
  }
}  // namespace UI
