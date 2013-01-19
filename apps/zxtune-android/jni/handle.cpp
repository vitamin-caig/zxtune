/*
Abstract:
  Handle operating functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "data.h"
#include "debug.h"
#include "module.h"
#include "player.h"
#include "zxtune.h"

JNIEXPORT void JNICALL Java_app_zxtune_ZXTune_Handle_1Close
  (JNIEnv* /*env*/, jclass /*self*/, jint handle)
{
  if (Data::Storage::Instance().Fetch(handle))
  {
    Dbg("Released data (handle=%1%)", handle);
  }
  else if (Module::Storage::Instance().Fetch(handle))
  {
    Dbg("Released module (handle=%1%)", handle);
  }
  else if (Player::Storage::Instance().Fetch(handle))
  {
    Dbg("Released player (handle=%1%)", handle);
  }
}
