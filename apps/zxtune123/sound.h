/*
Abstract:
  Sound component declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE123_SOUND_H_DEFINED
#define ZXTUNE123_SOUND_H_DEFINED

//common includes
#include <parameters.h>
//library includes
#include <sound/backend.h>
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

class SoundComponent
{
public:
  virtual ~SoundComponent() {}
  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;
  virtual void ParseParameters() = 0;
  virtual void Initialize() = 0;
  // functional part
  virtual ZXTune::Sound::Backend::Ptr CreateBackend(ZXTune::Module::Holder::Ptr module) = 0;

  //parameters
  virtual uint_t GetFrameDuration() const = 0;

  static std::auto_ptr<SoundComponent> Create(Parameters::Container::Ptr configParams);
};

#endif //ZXTUNE123_SOUND_H_DEFINED
