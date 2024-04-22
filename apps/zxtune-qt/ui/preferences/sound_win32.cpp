/**
 *
 * @file
 *
 * @brief Win32 settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound_win32.h"
#include "sound_win32.ui.h"
#include "supp/options.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
// library includes
#include <debug/log.h>
#include <sound/backends/win32.h>
#include <sound/backends_parameters.h>

namespace
{
  const Debug::Stream Dbg("UI::Preferences::Win32");
}

namespace
{
  struct Device
  {
    Device() = default;

    explicit Device(const Sound::Win32::Device& in)
      : Name(ToQString(in.Name()))
      , Id(in.Id())
    {}

    QString Name;
    int_t Id = 0;
  };

  class Win32OptionsWidget
    : public UI::Win32SettingsWidget
    , public Ui::Win32Options
  {
  public:
    explicit Win32OptionsWidget(QWidget& parent)
      : UI::Win32SettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      FillDevices();
      SelectDevice();

      using namespace Parameters::ZXTune::Sound::Backends::Win32;
      Parameters::IntegerValue::Bind(*buffers, *Options, BUFFERS, BUFFERS_DEFAULT);
      Require(connect(devices, &QComboBox::currentTextChanged, this, &Win32OptionsWidget::DeviceNameChanged));
    }

    String GetBackendId() const override
    {
      static const Char ID[] = {'w', 'i', 'n', '3', '2', '\0'};
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
      UI::Win32SettingsWidget::changeEvent(event);
    }

  private:
    void DeviceNameChanged(const QString& name)
    {
      Dbg("Selecting device '{}'", FromQString(name));
      DeviceChanged([&name](const Device& dev) { return dev.Name == name; });
    }

    void SelectDevice()
    {
      using namespace Parameters::ZXTune::Sound::Backends::Win32;
      const auto curDevice = Parameters::GetInteger(*Options, DEVICE, DEVICE_DEFAULT);
      DeviceChanged([curDevice](const Device& dev) { return dev.Id == curDevice; });
    }

    template<class F>
    void DeviceChanged(const F& fun)
    {
      const auto it = std::find_if(Devices.begin(), Devices.end(), fun);
      if (it != Devices.end())
      {
        devices->setCurrentIndex(it - Devices.begin());
        Options->SetValue(Parameters::ZXTune::Sound::Backends::Win32::DEVICE, it->Id);
      }
      else
      {
        devices->setCurrentIndex(-1);
      }
    }

    void FillDevices()
    {
      using namespace Sound;
      for (const auto availableDevices = Win32::EnumerateDevices(); availableDevices->IsValid();
           availableDevices->Next())
      {
        const Win32::Device::Ptr cur = availableDevices->Get();
        Devices.emplace_back(*cur);
        devices->addItem(Devices.back().Name);
      }
    }

  private:
    const Parameters::Container::Ptr Options;
    using DevicesArray = std::vector<Device>;
    DevicesArray Devices;
  };
}  // namespace
namespace UI
{
  Win32SettingsWidget::Win32SettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {}

  BackendSettingsWidget* Win32SettingsWidget::Create(QWidget& parent)
  {
    return new Win32OptionsWidget(parent);
  }
}  // namespace UI
