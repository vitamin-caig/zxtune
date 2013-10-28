/**
*
* @file     debug_log.h
* @brief    Debug logging functions interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef DEBUG_LOG_H_DEFINED
#define DEBUG_LOG_H_DEFINED

#ifdef NO_DEBUG_LOGS
#include "src/log_stub.h"
#else
#include "src/log_real.h"
#endif

#endif //DEBUG_LOG_H_DEFINED
