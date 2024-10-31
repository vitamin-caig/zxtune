/**
 *
 * @file
 *
 * @brief  Frequency tables internal interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "core/freq_tables.h"

#include "string_view.h"

namespace Module
{
  // getting frequency table data by name
  // throw Error in case of problem
  void GetFreqTable(StringView id, FrequencyTable& result);
}  // namespace Module
