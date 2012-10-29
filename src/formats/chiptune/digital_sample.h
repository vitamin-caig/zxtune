/*
Abstract:
  Digital samples helper interfaces

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <binary/data.h>

namespace Formats
{
  namespace Chiptune
  {
    Binary::Data::Ptr Convert4BitSample(const Binary::Data& sample);
    Binary::Data::Ptr Unpack4BitSample(const Binary::Data& sample);
  }
}
