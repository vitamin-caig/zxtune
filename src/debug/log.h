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
#include "src/log_stub.h"
#else
#include "src/log_real.h"
#endif
