/**
 *
 * @file
 *
 * @brief  VortexTracker-based chiptunes support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/aym/aym_base_track.h"
// library includes
#include <formats/chiptune/aym/protracker3.h>

namespace Module::Vortex
{
  // Frequency table enumeration, compatible with binary format (PT3.x)
  enum NoteTable
  {
    PROTRACKER,
    SOUNDTRACKER,
    ASM,
    REAL,
    NATURAL
  };

  String GetFreqTable(NoteTable table, uint_t version);

  using Formats::Chiptune::ProTracker3::Sample;
  using Formats::Chiptune::ProTracker3::Ornament;

  // supported commands set and their parameters
  enum Commands
  {
    // no parameters
    EMPTY,
    // period,delta
    GLISS,
    // period,delta,target note
    GLISS_NOTE,
    // offset
    SAMPLEOFFSET,
    // offset
    ORNAMENTOFFSET,
    // ontime,offtime
    VIBRATE,
    // period,delta
    SLIDEENV,
    // no parameters
    NOENVELOPE,
    // r13,period
    ENVELOPE,
    // base
    NOISEBASE,
  };

  class ModuleData : public AYM::ModuleData<OrderList, Sample, Ornament>
  {
  public:
    using RWPtr = std::shared_ptr<ModuleData>;
    using Ptr = std::shared_ptr<const ModuleData>;

    ModuleData() = default;

    uint_t Version = 6;
  };

  AYM::DataRenderer::Ptr CreateDataRenderer(ModuleData::Ptr data, uint_t trackChannelStart);
}  // namespace Module::Vortex
