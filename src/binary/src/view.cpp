/**
 *
 * @file
 *
 * @brief  View implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "binary/view.h"

#include "binary/data.h"

namespace Binary
{
  // outline ctor to allow forward declaration of Data
  View::View(const Data& data)
    : View(data.Start(), data.Size())
  {}
}  // namespace Binary
