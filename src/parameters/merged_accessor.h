/**
*
* @file     parameters/merged_accessor.h
* @brief    Factories for accessor-based adapters
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PARAMETERS_MERGED_ACCESSOR_H_DEFINED
#define PARAMETERS_MERGED_ACCESSOR_H_DEFINED

//library includes
#include <parameters/accessor.h>

namespace Parameters
{
  // All accessed properties are prioritized by the first one
  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second);
  Accessor::Ptr CreateMergedAccessor(Accessor::Ptr first, Accessor::Ptr second, Accessor::Ptr third);
}

#endif //PARAMETERS_MERGED_ACCESSOR_H_DEFINED
