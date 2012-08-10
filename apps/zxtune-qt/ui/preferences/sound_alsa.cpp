/*
Abstract:
  Alsa settings widget implementation

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
#include <debug_log.h>
#include <format.h>
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>
#include <sound/backends/alsa.h>
//boost includes
#include <boost/bind.hpp>
//text includes
#include "text/text.h"

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
    void (QStringList::*add)(const QString&) = &QStringList::append;
    std::for_each(input.begin(), input.end(),
      boost::bind(add, &result, boost::bind(&ToQString, _1)));
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

      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
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
      const String& id = FromQString(name);
      Dbg("Selecting device '%1%'", id);
      const DevicesArray::const_iterator it = std::find_if(Devices.begin(), Devices.end(),
        boost::bind(&Device::Name, _1) == name || boost::bind(&Device::Id, _1) == id);
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

    virtual void MixerChanged(const QString& name)
    {
      //while recreating combobox message with empty parameter can be passed
      if (name.size())
      {
        const String mixer = FromQString(name);
        Dbg("Selecting mixer '%1%'", mixer);
        Options->SetValue(Parameters::ZXTune::Sound::Backends::Alsa::MIXER, mixer);
      }
    }
  private:
    void SelectDevice()
    {
      using namespace Parameters::ZXTune::Sound::Backends::Alsa;
      String curDevice = DEVICE_DEFAULT;
      Options->FindValue(DEVICE, curDevice);
      //force fill mixers- signal is not called by previous function (even when connected before, why?)
      DeviceChanged(ToQString(curDevice));
    }

    void FillDevices()
    {
      using namespace ZXTune::Sound;
      for (Alsa::Device::Iterator::Ptr availableDevices = Alsa::EnumerateDevices();
        availableDevices->IsValid(); availableDevices->Next())
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
      Device()
      {
      }

      Device(const ZXTune::Sound::Alsa::Device& in)
        : Name(ToQString(Strings::Format(Text::SOUND_DEVICE_ON_CARD_FORMAT, in.Name(), in.CardName())))
        , Id(in.Id())
        , MixerNames(ToStringList(in.Mixers()))
      {
      }

      QString Name;
      String Id;
      QStringList MixerNames;
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
