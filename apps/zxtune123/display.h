/*
Abstract:
  Display component interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE123_DISPLAY_H_DEFINED
#define ZXTUNE123_DISPLAY_H_DEFINED

//library includes
#include <core/module_player.h>
#include <sound/backend.h>
#include <time/stamp.h>
//std includes
#include <memory>

//forward declarations
namespace boost
{
  namespace program_options
  {
    class options_description;
  }
}

class DisplayComponent
{
public:
  typedef std::auto_ptr<DisplayComponent> Ptr;

  virtual ~DisplayComponent() {}

  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;

  virtual void Message(const String& msg) = 0;
  virtual void SetModule(Module::Holder::Ptr module, Sound::Backend::Ptr player, Time::Microseconds frameDuration) = 0;

  // begin frame, returns current frame number
  virtual uint_t BeginFrame(Sound::PlaybackControl::State state) = 0;
  virtual void EndFrame() = 0;

  static Ptr Create();
};

#endif //ZXTUNE123_DISPLAY_H_DEFINED
