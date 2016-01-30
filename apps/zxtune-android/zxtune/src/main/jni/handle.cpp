/**
* 
* @file
*
* @brief Handle operating functions
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "debug.h"
#include "module.h"
#include "player.h"
#include "zxtune.h"

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Handle_1Close
  (JNIEnv* /*env*/, jclass /*self*/, jint handle)
{
  if (Module::Storage::Instance().Fetch(handle))
  {
    Dbg("Released module (handle=%1%)", handle);
  }
  else if (Player::Storage::Instance().Fetch(handle))
  {
    Dbg("Released player (handle=%1%)", handle);
  }
}
