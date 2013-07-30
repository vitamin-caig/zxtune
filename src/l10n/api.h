/**
*
* @file     api.h
* @brief    Localization support API
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef L10N_API_H_DEFINED
#define L10N_API_H_DEFINED

#ifndef NO_L10N
#include "src/api_stub.h"
#else
#include "src/api_real.h"
#endif

namespace L10n
{
  inline const char* translate(const char* txt)
  {
    return txt;
  }
}

#endif //L10N_API_H_DEFINED
