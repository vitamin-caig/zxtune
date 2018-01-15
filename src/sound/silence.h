#pragma once

//library includes
#include <parameters/accessor.h>
#include <sound/receiver.h>

namespace Sound
{
  Receiver::Ptr CreateSilenceDetector(Parameters::Accessor::Ptr params, Receiver::Ptr delegate);
}
