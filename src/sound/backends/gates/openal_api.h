/**
*
* @file
*
* @brief  OpenAL subsystem API gate interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//platform-dependent includes
#include <AL/al.h>
#include <AL/alc.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Sound
{
  namespace OpenAl
  {
    class Api
    {
    public:
      typedef boost::shared_ptr<Api> Ptr;
      virtual ~Api() {}

      
      virtual ALCdevice* alcOpenDevice(const ALCchar* devicename) = 0;
      virtual ALCboolean alcCloseDevice(ALCdevice* device) = 0;
      virtual ALCcontext* alcCreateContext(ALCdevice* device, ALCint* attrlist) = 0;
      virtual ALCboolean alcMakeContextCurrent(ALCcontext* context) = 0;
      virtual ALCcontext* alcGetCurrentContext() = 0;
      virtual void alcDestroyContext(ALCcontext* context) = 0;
      virtual void alGenBuffers(ALsizei n, ALuint* buffers) = 0;
      virtual void alDeleteBuffers(ALsizei n, ALuint* buffers) = 0;
      virtual void alBufferData(ALuint buffer, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq) = 0;
      virtual void alGenSources(ALsizei n, ALuint* sources) = 0;
      virtual void alDeleteSources(ALsizei n, ALuint *sources) = 0;
      virtual void alSourceQueueBuffers(ALuint source, ALsizei n, ALuint* buffers) = 0;
      virtual void alSourceUnqueueBuffers(ALuint source, ALsizei n, ALuint* buffers) = 0;
      virtual void alSourcePlay(ALuint source) = 0;
      virtual void alSourceStop(ALuint source) = 0;
      virtual void alSourcePause(ALuint source) = 0;
      virtual void alGetSourcei(ALuint source, ALenum pname, ALint* value) = 0;
      virtual void alSourcef(ALuint source, ALenum pname, ALfloat value) = 0;
      virtual void alGetSourcef(ALuint source, ALenum pname, ALfloat* value) = 0;
      virtual const ALchar* alGetString(ALenum param) = 0;
      virtual const ALCchar* alcGetString(ALCdevice* device, ALenum param) = 0;
      virtual ALenum alGetError(void) = 0;
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();

  }
}
