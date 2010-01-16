/*
Abstract:
  Sound component declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  
  This file is a part of zxtune123 application based on zxtune library
*/
#ifndef ZXTUNE123_SOUND_H_DEFINED
#define ZXTUNE123_SOUND_H_DEFINED

#include <parameters.h>
#include <sound/backend.h>

#include <memory>

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
  // throw
  virtual void Initialize() = 0;
  // functional part
  virtual ZXTune::Sound::Backend& GetBackend() = 0;
  
  static std::auto_ptr<SoundComponent> Create(Parameters::Map& globalParams);
};

#endif //ZXTUNE123_SOUND_H_DEFINED
