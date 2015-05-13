/**
* 
* @file
*
* @brief DirectSound settings pane implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sound_dsound.h"
#include "sound_dsound.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <debug/log.h>
#include <sound/backends_parameters.h>
#include <sound/backends/dsound.h>
//boost includes
#include <boost/bind.hpp>
//text includes
#include "text/text.h"

namespace
{
  const Debug::Stream Dbg("UI::Preferences::DirectSound");
}

namespace
{
  class DirectSoundOptionsWidget : public UI::DirectSoundSettingsWidget
                                 , public Ui::DirectSoundOptions
  {
  public:
    explicit DirectSoundOptionsWidget(QWidget& parent)
      : UI::DirectSoundSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      FillDevices();
      SelectDevice();

      using namespace Parameters::ZXTune::Sound::Backends::DirectSound;
      Parameters::IntegerValue::Bind(*latency, *Options, LATENCY, LATENCY_DEFAULT);
      Require(connect(devices, SIGNAL(currentIndexChanged(const QString&)), SLOT(DeviceChanged(const QString&))));
    }

    virtual Parameters::Container::Ptr GetSettings() const
    {
      //TODO
      return Parameters::Container::Ptr();
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'d', 's', 'o', 'u', 'n', 'd', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      return nameGroup->title();
    }

    virtual void DeviceChanged(const QString& name)
    {
      const String& id = LocalFromQString(name);
      Dbg("Selecting device '%1%'", id);
      const DevicesArray::const_iterator it = std::find_if(Devices.begin(), Devices.end(),
        boost::bind(&Device::Name, _1) == name || boost::bind(&Device::Id, _1) == id);
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

    //QWidget
    virtual void changeEvent(QEvent* event)
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
        Devices.push_back(Device(*cur));
        devices->addItem(Devices.back().Name);
      }
    }
  private:
    const Parameters::Container::Ptr Options;

    struct Device
    {
      Device()
      {
      }

      explicit Device(const Sound::DirectSound::Device& in)
        : Name(ToQStringFromLocal(in.Name()))
        , Id(in.Id())
      {
      }

      QString Name;
      String Id;
    };

    typedef std::vector<Device> DevicesArray;
    DevicesArray Devices;
  };
}
namespace UI
{
  DirectSoundSettingsWidget::DirectSoundSettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {
  }

  BackendSettingsWidget* DirectSoundSettingsWidget::Create(QWidget& parent)
  {
    return new DirectSoundOptionsWidget(parent);
  }
}
