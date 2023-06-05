/**
 *
 * @file
 *
 * @brief DirectSound settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound_dsound.h"
#include "sound_dsound.ui.h"
#include "supp/options.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
// library includes
#include <debug/log.h>
#include <sound/backends/dsound.h>
#include <sound/backends_parameters.h>

namespace
{
  const Debug::Stream Dbg("UI::Preferences::DirectSound");
}

namespace
{
  class DirectSoundOptionsWidget
    : public UI::DirectSoundSettingsWidget
    , public Ui::DirectSoundOptions
  {
  public:
    explicit DirectSoundOptionsWidget(QWidget& parent)
      : UI::DirectSoundSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      FillDevices();
      SelectDevice();

      using namespace Parameters::ZXTune::Sound::Backends::DirectSound;
      Parameters::IntegerValue::Bind(*latency, *Options, LATENCY, LATENCY_DEFAULT);
      Require(connect(devices, SIGNAL(currentIndexChanged(const QString&)), SLOT(DeviceChanged(const QString&))));
    }

    String GetBackendId() const override
    {
      static const Char ID[] = {'d', 's', 'o', 'u', 'n', 'd', '\0'};
      return ID;
    }

    QString GetDescription() const override
    {
      return nameGroup->title();
    }

    void DeviceChanged(const QString& name) override
    {
      const auto& id = FromQString(name);
      Dbg("Selecting device '{}'", id);
      const auto it = std::find_if(Devices.begin(), Devices.end(),
                                   [&name, &id](const Device& dev) { return dev.Name == name || dev.Id == id; });
      if (it != Devices.end())
      {
        devices->setCurrentIndex(it - Devices.begin());
        Options->SetValue(Parameters::ZXTune::Sound::Backends::DirectSound::DEVICE, it->Id);
      }
      else
      {
        devices->setCurrentIndex(-1);
      }
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::DirectSoundSettingsWidget::changeEvent(event);
    }

  private:
    void SelectDevice()
    {
      using namespace Parameters::ZXTune::Sound::Backends::DirectSound;
      String curDevice;
      Options->FindValue(DEVICE, curDevice);
      DeviceChanged(ToQString(curDevice));
    }

    void FillDevices()
    {
      using namespace Sound;
      for (DirectSound::Device::Iterator::Ptr availableDevices = DirectSound::EnumerateDevices();
           availableDevices->IsValid(); availableDevices->Next())
      {
        const DirectSound::Device::Ptr cur = availableDevices->Get();
        Devices.emplace_back(*cur);
        devices->addItem(Devices.back().Name);
      }
    }

  private:
    const Parameters::Container::Ptr Options;

    struct Device
    {
      Device() = default;

      explicit Device(const Sound::DirectSound::Device& in)
        : Name(ToQString(in.Name()))
        , Id(in.Id())
      {}

      QString Name;
      String Id;
    };

    using DevicesArray = std::vector<Device>;
    DevicesArray Devices;
  };
}  // namespace
namespace UI
{
  DirectSoundSettingsWidget::DirectSoundSettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {}

  BackendSettingsWidget* DirectSoundSettingsWidget::Create(QWidget& parent)
  {
    return new DirectSoundOptionsWidget(parent);
  }
}  // namespace UI
