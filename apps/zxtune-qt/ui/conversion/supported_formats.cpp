/*
Abstract:
  Supported formats widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "supported_formats.h"
#include "supported_formats.ui.h"
#include "ui/utils.h"
//common includes
#include <string_helpers.h>
#include <tools.h>
//library includes
#include <sound/backend.h>
#include <sound/backend_attrs.h>
//std includes
#include <functional>
#include <set>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtGui/QRadioButton>

namespace
{
  class FileBackendsSet
  {
  public:
    FileBackendsSet()
    {
      using namespace ZXTune::Sound;
      for (BackendCreator::Iterator::Ptr backends = EnumerateBackends(); backends->IsValid(); backends->Next())
      {
        const BackendCreator::Ptr creator = backends->Get();
        if (0 != (creator->Capabilities() & CAP_TYPE_FILE))
        {
          Ids.insert(creator->Id());
        }
      }
    }

    bool IsAvailable(const String& id) const
    {
      return Ids.count(id);
    }
  private:
    std::set<String> Ids;
  };

  class SupportedFormats : public UI::SupportedFormatsWidget
                         , private Ui::SupportedFormats
  {
  public:
    explicit SupportedFormats(QWidget& parent)
      : UI::SupportedFormatsWidget(parent)
      , Backends()
    {
      //setup self
      setupUi(this);

      Buttons["wav"] = selectWAV;
      Buttons["mp3"] = selectMP3;
      Buttons["ogg"] = selectOGG;
      Buttons["flac"] = selectFLAC;

      std::for_each(Buttons.begin(), Buttons.end(),
        std::bind1st(std::mem_fun(&SupportedFormats::SetupButton), this));

      //TODO
      selectWAV->setChecked(true);
    }

    virtual String GetSelectedId() const
    {
      const IdToButton::const_iterator it =
        std::find_if(Buttons.begin(), Buttons.end(),
          boost::bind(&QRadioButton::isChecked, boost::bind(&IdToButton::value_type::second, _1)));
      if (it != Buttons.end())
      {
        return FromStdString(it->first);
      }
      else
      {
        return String();
      }
    }

    virtual QString GetDescription() const
    {
      const IdToButton::const_iterator it =
        std::find_if(Buttons.begin(), Buttons.end(),
          boost::bind(&QRadioButton::isChecked, boost::bind(&IdToButton::value_type::second, _1)));
      if (it != Buttons.end())
      {
        return it->second->text();
      }
      else
      {
        return QString();
      }
    }
  private:
    typedef std::map<std::string, QRadioButton*> IdToButton;

    void SetupButton(IdToButton::value_type but)
    {
      connect(but.second, SIGNAL(toggled(bool)), SIGNAL(SettingsChanged()));
      but.second->setEnabled(Backends.IsAvailable(FromStdString(but.first)));
    }
  private:
    const FileBackendsSet Backends;
    IdToButton Buttons;
  };
}

namespace UI
{
  SupportedFormatsWidget::SupportedFormatsWidget(QWidget& parent)
    : QWidget(&parent)
  {
  }

  SupportedFormatsWidget* SupportedFormatsWidget::Create(QWidget& parent)
  {
    return new SupportedFormats(parent);
  }
}
