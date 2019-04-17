/**
* 
* @file
*
* @brief  Analyzer factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/state.h>
#include <module/analyzer.h>

namespace Sound
{
  struct Chunk;
}

namespace Module
{
  Analyzer::Ptr CreateStubAnalyzer();
  Analyzer::Ptr CreateAnalyzer(Devices::StateSource::Ptr state);
  
  class SoundAnalyzer : public Analyzer
  {
  public:
    using Ptr = std::shared_ptr<SoundAnalyzer>;
    
    virtual void AddSoundData(const Sound::Chunk& data) = 0;
  };
  
  SoundAnalyzer::Ptr CreateSoundAnalyzer();
}
