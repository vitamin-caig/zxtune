/**
 *
 * @file
 *
 * @brief  State iterators support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/iterator.h"
// library includes
#include <module/loop.h>

namespace Module
{
  void SeekIterator(Iterator& iter, State::Ptr state, Time::AtMillisecond request)
  {
    if (request < state->At())
    {
      iter.Reset();
    }
    while (state->At() < request && iter.IsValid())
    {
      iter.NextFrame({});
    }
  }
}  // namespace Module
