/*
Abstract:
  ALSA settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "sound_alsa.h"
#include "sound_alsa.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>
#include <sound/backends/alsa.h>
//boost includes
#include <boost/bind.hpp>

namespace
{
  const std::string THIS_MODULE("UI::Preferences::ALSA");
}

namespace
{
  template<class T>
  QStringList ToStringList(const T& input)
  {
    QStringList result;
    std::for_each(input.begin(), input.end(),
      boost::bind(boost::mem_fn<void, QStringList, const QString&>(&QStringList::append), &result, boost::bind(&ToQString, _1)));
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

  class AlsaOptionsWidget : public UI::AlsaSettingsWidget
                          , public Ui::AlsaOptions
  {
  public:
    explicit AlsaOptionsWidget(QWidget& parent)
      : UI::AlsaSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      FillDevices();
      SelectDevice();

      using namespace Parameters::ZXTune::Sound::Backends::ALSA;
      Parameters::IntegerValue::Bind(*buffers, *Options, BUFFERS, BUFFERS_DEFAULT);
      Require(connect(mixers, SIGNAL(currentIndexChanged(const QString&)), SLOT(MixerChanged(const QString&))));
      Require(connect(devices, SIGNAL(currentIndexChanged(const QString&)), SLOT(DeviceChanged(const QString&))));
    }

    virtual Parameters::Container::Ptr GetSettings() const
    {
      //TODO
      return Parameters::Container::Ptr();
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'a', 'l', 's', 'a', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      return nameGroup->title();
    }

    virtual void DeviceChanged(const QString& name)
    {
      Log::Debug(THIS_MODULE, "Selecting device '%1%'", FromQString(name));
      const DevicesArray::const_iterator it = std::find_if(Devices.begin(), Devices.end(),
        boost::bind(&Device::Name, _1) == name);
      if (it != Devices.end())
      {
        mixers->clear();
        mixers->addItems(it->MixerNames);
        Options->SetValue(Parameters::ZXTune::Sound::Backends::ALSA::DEVICE, FromQString(name));
        SelectMixer();
      }
    }

    virtual void MixerChanged(const QString& name)
    {
      //while recreating combobox message with empty parameter can be passed
      if (name.size())
      {
        const String mixer = FromQString(name);
        Log::Debug(THIS_MODULE, "Selecting mixer '%1%'", mixer);
        Options->SetValue(Parameters::ZXTune::Sound::Backends::ALSA::MIXER, mixer);
      }
    }
  private:
    void SelectDevice()
    {
      using namespace Parameters::ZXTune::Sound::Backends::ALSA;
      String curDevice = DEVICE_DEFAULT;
      Options->FindValue(DEVICE, curDevice);
      SetComboValue(*devices, curDevice);
      //force fill mixers- signal is not called by previous function (even when connected before, why?)
      DeviceChanged(ToQString(curDevice));
    }

    void FillDevices()
    {
      const StringArray& availDevices = ZXTune::Sound::ALSA::EnumerateDevices();
      Devices.resize(availDevices.size());
      std::transform(availDevices.begin(), availDevices.end(), Devices.begin(), &Device::Create);
      devices->addItems(ToStringList(availDevices));
    }

    void SelectMixer()
    {
      using namespace Parameters::ZXTune::Sound::Backends::ALSA;
      String curMixer;
      Options->FindValue(MIXER, curMixer);
      SetComboValue(*mixers, curMixer);
    }
  private:
    const Parameters::Container::Ptr Options;

    struct Device
    {
      QString Name;
      QStringList MixerNames;

      static Device Create(const String& name)
      {
        Device dev;
        dev.Name = ToQString(name);
        dev.MixerNames = ToStringList(ZXTune::Sound::ALSA::EnumerateMixers(name));
        return dev;
      }
    };

    typedef std::vector<Device> DevicesArray;
    DevicesArray Devices;
  };
}
namespace UI
{
  AlsaSettingsWidget::AlsaSettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {
  }

  BackendSettingsWidget* AlsaSettingsWidget::Create(QWidget& parent)
  {
    return new AlsaOptionsWidget(parent);
  }
}
