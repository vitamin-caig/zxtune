/*
Abstract:
  Win32 settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "sound_win32.h"
#include "sound_win32.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <debug_log.h>
#include <format.h>
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>
#include <sound/backends/win32.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/function.hpp>
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
    {
    }

    explicit Device(const ZXTune::Sound::Win32::Device& in)
      : Name(ToQString(in.Name()))
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

    virtual Parameters::Container::Ptr GetSettings() const
    {
      //TODO
      return Parameters::Container::Ptr();
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'w', 'i', 'n', '3', '2', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      return nameGroup->title();
    }

    virtual void DeviceChanged(const QString& name)
    {
      Dbg("Selecting device '%1%'", FromQString(name));
      DeviceChanged(boost::bind(&Device::Name, _1) == name);
    }

    //QWidget
    virtual void changeEvent(QEvent* event)
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

    void DeviceChanged(const boost::function<bool(const Device&)>& fun)
    {
      const DevicesArray::const_iterator it = std::find_if(Devices.begin(), Devices.end(), fun);
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
      using namespace ZXTune::Sound;
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
