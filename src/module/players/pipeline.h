/**
 *
 * @file
 *
 * @brief  Sound pipeline support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/renderer.h"
#include "parameters/accessor.h"

namespace Module
{
  class Holder;

  /* Creates wrapper that applies:
   - loop control
   - gain
   - fadein/fadeout
   - silence detection

   Samplerate is taken from globalParams.
   Other properties are taken from holder.Parameters, globalParams in specified order
  */
  Renderer::Ptr CreatePipelinedRenderer(const Holder& holder, Parameters::Accessor::Ptr globalParams);

  Renderer::Ptr CreatePipelinedRenderer(const Holder& holder, uint_t samplerate,
                                        Parameters::Accessor::Ptr globalParams);
}  // namespace Module
