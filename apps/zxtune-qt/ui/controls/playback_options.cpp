/*
Abstract:
  Playback options widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playback_options.h"
#include "playback_options.ui.h"
#include "ui/tools/parameters_helpers.h"
//library includes
#include <sound/sound_parameters.h>

namespace
{
  class PlaybackOptionsImpl : public PlaybackOptions
                            , private Ui::PlaybackOptions
  {
  public:
    PlaybackOptionsImpl(QWidget& parent, Parameters::Container& params)
      : ::PlaybackOptions(parent)
      , Params(params)
    {
      //setup self
      setupUi(this);

      Parameters::BooleanValue::Bind(*actionLoop, Params, Parameters::ZXTune::Sound::LOOPED, false);
    }
  private:
    Parameters::Container& Params;
  };
}

PlaybackOptions::PlaybackOptions(QWidget& parent) : QWidget(&parent)
{
}

PlaybackOptions* PlaybackOptions::Create(QWidget& parent, Parameters::Container& params)
{
  return new PlaybackOptionsImpl(parent, params);
}
