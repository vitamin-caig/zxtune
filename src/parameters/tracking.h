/**
*
* @file     parameters/tracking.h
* @brief    Parameters tracking functionality
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PARAMETERS_TRACKING_H_DEFINED
#define PARAMETERS_TRACKING_H_DEFINED

//library includes
#include <parameters/container.h>

namespace Parameters
{
  Container::Ptr CreatePreChangePropertyTrackedContainer(Container::Ptr delegate, Modifier& callback);
  Container::Ptr CreatePostChangePropertyTrackedContainer(Container::Ptr delegate, Modifier& callback);
}

#endif //PARAMETERS_TRACKING_H_DEFINED
