/**
*
* @file     control.h
* @brief    L10n support control
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef L10N_CONTROL_H_DEFINED
#define L10N_CONTROL_H_DEFINED

//library includes
#include <l10n/api.h>

namespace L10n
{
  void LoadTranslationsFromResources(Library& lib);
}

#endif //L10N_CONTROL_H_DEFINED
