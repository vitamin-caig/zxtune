/**
*
* @file
*
* @brief  PulseAudio subsystem API gate interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//platform-dependent includes
#include <pulse/simple.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Sound
{
  namespace PulseAudio
  {
    class Api
    {
    public:
      typedef boost::shared_ptr<Api> Ptr;
      virtual ~Api() {}

      
      virtual const char* pa_get_library_version(void) = 0;
      virtual const char* pa_strerror(int error) = 0;
      virtual pa_simple* pa_simple_new(const char* server, const char* name, pa_stream_direction_t dir, const char* dev, const char* stream, const pa_sample_spec* ss, const pa_channel_map* map, const pa_buffer_attr* attr, int* error) = 0;
      virtual int pa_simple_write(pa_simple* s, const void* data, size_t bytes, int* error) = 0;
      virtual int pa_simple_flush(pa_simple* s, int* error) = 0;
      virtual void pa_simple_free(pa_simple* s) = 0;
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();

  }
}
