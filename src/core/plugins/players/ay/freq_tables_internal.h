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

// common includes
#include <string_view.h>
// library includes
#include <core/freq_tables.h>

namespace Module
{
  // getting frequency table data by name
  // throw Error in case of problem
  void GetFreqTable(StringView id, FrequencyTable& result);
}  // namespace Module
