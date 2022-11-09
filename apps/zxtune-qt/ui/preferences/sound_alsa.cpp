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

  void SetComboValue(QComboBox& box, const String& val)
  {
    const QString& str = ToQString(val);
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
      Require(connect(mixers, SIGNAL(currentIndexChanged(const QString&)), SLOT(MixerChanged(const QString&))));
      Require(connect(devices, SIGNAL(currentIndexChanged(const QString&)), SLOT(DeviceChanged(const QString&))));
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

    void DeviceChanged(const QString& name) override
    {
      const String& id = FromQString(name);
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

    void MixerChanged(const QString& name) override
    {
      // while recreating combobox message with empty parameter can be passed
      if (name.size())
      {
        const String mixer = FromQString(name);
        Dbg("Selecting mixer '{}'", mixer);
        Options->SetValue(Parameters::ZXTune::Sound::Backends::Alsa::MIXER, mixer);
      }
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
      String curDevice = DEVICE_DEFAULT;
      Options->FindValue(DEVICE, curDevice);
      // force fill mixers- signal is not called by previous function (even when connected before, why?)
      DeviceChanged(ToQString(curDevice));
    }

    void FillDevices()
    {
      using namespace Sound;
      for (Alsa::Device::Iterator::Ptr availableDevices = Alsa::EnumerateDevices(); availableDevices->IsValid();
           availableDevices->Next())
      {
        const Alsa::Device::Ptr cur = availableDevices->Get();
        Devices.push_back(Device(*cur));
        devices->addItem(Devices.back().Name);
      }
    }

    void SelectMixer()
    {
      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
      String curMixer;
      Options->FindValue(MIXER, curMixer);
      SetComboValue(*mixers, curMixer);
    }

  private:
    const Parameters::Container::Ptr Options;

    struct Device
    {
      Device() {}

      Device(const Sound::Alsa::Device& in)
        : Name(QString::fromLatin1("%1 (%2)").arg(ToQString(in.Name())).arg(ToQString(in.CardName())))
        , Id(in.Id())
        , MixerNames(ToStringList(in.Mixers()))
      {}

      QString Name;
      String Id;
      QStringList MixerNames;
    };

    typedef std::vector<Device> DevicesArray;
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
