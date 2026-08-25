#pragma once

#include "module/players/factory.h"

namespace Module::MTC
{
  Factory::Ptr CreateFactory(Factory::Ptr delegate);
}
