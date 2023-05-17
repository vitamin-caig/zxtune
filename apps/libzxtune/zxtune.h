/**
 *
 * @file
 *
 * @brief Simple dynamic library interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32)
#  define ZXTUNE_API_EXPORT __declspec(dllexport)
#  define ZXTUNE_API_IMPORT __declspec(dllimport)
#elif __GNUC__ > 3
#  define ZXTUNE_API_EXPORT __attribute__((visibility("default")))
#  define ZXTUNE_API_IMPORT
#else
#  define ZXTUNE_API_EXPORT
#  define ZXTUNE_API_IMPORT
#endif

#ifndef ZXTUNE_API
#  define ZXTUNE_API ZXTUNE_API_IMPORT
#endif

  // universal handle type
  using ZXTuneHandle = const void*;

  // Common functions
  ZXTUNE_API const char* ZXTune_GetVersion();

  // Data operating
  ZXTUNE_API ZXTuneHandle ZXTune_CreateData(const void* data, size_t size);
  ZXTUNE_API void ZXTune_CloseData(ZXTuneHandle data);

  // Modules operating
  ZXTUNE_API ZXTuneHandle ZXTune_OpenModule(ZXTuneHandle data);
  ZXTUNE_API void ZXTune_CloseModule(ZXTuneHandle module);

  using ZXTuneModuleInfo = struct
  {
    int Positions;
    int LoopPosition;
    int Frames;
    int LoopFrame;
    int Channels;
  };

  ZXTUNE_API bool ZXTune_GetModuleInfo(ZXTuneHandle module, ZXTuneModuleInfo* info);

  // Players operating
  ZXTUNE_API ZXTuneHandle ZXTune_CreatePlayer(ZXTuneHandle module);
  ZXTUNE_API void ZXTune_DestroyPlayer(ZXTuneHandle player);
  // returns actually rendered bytes
  ZXTUNE_API int ZXTune_RenderSound(ZXTuneHandle player, void* buffer, size_t samples);
  ZXTUNE_API int ZXTune_SeekSound(ZXTuneHandle player, size_t sample);
  ZXTUNE_API bool ZXTune_ResetSound(ZXTuneHandle player);
  ZXTUNE_API bool ZXTune_GetPlayerParameterInt(ZXTuneHandle player, const char* paramName, int* paramValue);
  ZXTUNE_API bool ZXTune_SetPlayerParameterInt(ZXTuneHandle player, const char* paramName, int paramValue);

#ifdef __cplusplus
}  // extern
#endif
