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

//! @brief Namespace is used to keep parameters-working related types and functions
namespace Parameters
{
  class PropertyChangedCallback
  {
  public:
    virtual ~PropertyChangedCallback() {}

    virtual void OnPropertyChanged(const NameType& name) const = 0;
  };

  Container::Ptr CreatePropertyTrackedContainer(Container::Ptr delegate, const PropertyChangedCallback& callback);
}

#endif //PARAMETERS_TRACKING_H_DEFINED
