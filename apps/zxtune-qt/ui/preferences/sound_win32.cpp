/**
* 
* @file
*
* @brief Win32 settings pane implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sound_win32.h"
#include "sound_win32.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <debug/log.h>
#include <sound/backends_parameters.h>
#include <sound/backends/win32.h>
//boost includes
#include <boost/bind.hpp>
//text includes
#include "text/text.h"

namespace
{
  const Debug::Stream Dbg("UI::Preferences::Win32");
}

namespace
{
  struct Device
  {
    Device()
      : Id()
    {
    }

    explicit Device(const Sound::Win32::Device& in)
      : Name(ToQStringFromLocal(in.Name()))
      , Id(in.Id())
    {
    }

    QString Name;
    int_t Id;
  };

  class Win32OptionsWidget : public UI::Win32SettingsWidget
                           , public Ui::Win32Options
  {
  public:
    explicit Win32OptionsWidget(QWidget& parent)
      : UI::Win32SettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      FillDevices();
      SelectDevice();

      using namespace Parameters::ZXTune::Sound::Backends::Win32;
      Parameters::IntegerValue::Bind(*buffers, *Options, BUFFERS, BUFFERS_DEFAULT);
      Require(connect(devices, SIGNAL(currentIndexChanged(const QString&)), SLOT(DeviceChanged(const QString&))));
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

    void DeviceChanged(const QString& name) override
    {
      Dbg("Selecting device '%1%'", LocalFromQString(name));
      DeviceChanged(boost::bind(&Device::Name, _1) == name);
    }

    //QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::Win32SettingsWidget::changeEvent(event);
    }
  private:
    void SelectDevice()
    {
      using namespace Parameters::ZXTune::Sound::Backends::Win32;
      Parameters::IntType curDevice = DEVICE_DEFAULT;
      Options->FindValue(DEVICE, curDevice);
      DeviceChanged(boost::bind(&Device::Id, _1) == curDevice);
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
      for (Win32::Device::Iterator::Ptr availableDevices = Win32::EnumerateDevices();
        availableDevices->IsValid(); availableDevices->Next())
      {
        const Win32::Device::Ptr cur = availableDevices->Get();
        Devices.push_back(Device(*cur));
        devices->addItem(Devices.back().Name);
      }
    }
  private:
    const Parameters::Container::Ptr Options;
    typedef std::vector<Device> DevicesArray;
    DevicesArray Devices;
  };
}
namespace UI
{
  Win32SettingsWidget::Win32SettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {
  }

  BackendSettingsWidget* Win32SettingsWidget::Create(QWidget& parent)
  {
    return new Win32OptionsWidget(parent);
  }
}
