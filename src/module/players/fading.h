/**
* 
* @file
*
* @brief  Fading support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <module/information.h>
#include <module/state.h>
#include <parameters/accessor.h>
#include <sound/gainer.h>

namespace Module
{
  Sound::GainSource::Ptr CreateGainSource(Parameters::Accessor::Ptr params, Information::Ptr info, State::Ptr status);
  
  inline Sound::Receiver::Ptr CreateFadingReceiver(Parameters::Accessor::Ptr params, Information::Ptr info, State::Ptr status, Sound::Receiver::Ptr target)
  {
    return Sound::CreateGainer(CreateGainSource(std::move(params), std::move(info), std::move(status)), std::move(target));
  }
}
