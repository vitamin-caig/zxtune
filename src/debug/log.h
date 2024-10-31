/**
 *
 * @file
 *
 * @brief  Debug logging functions interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#ifdef NO_DEBUG_LOGS
#  include "debug/src/log_stub.h"
#else
#  include "debug/src/log_real.h"
#endif
