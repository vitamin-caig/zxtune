/**
 *
 * @file
 *
 * @brief  HighlyExperimental BIOS access implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/xsf/psf_bios.h"

namespace Module::PSF
{
  Binary::View GetSCPH10000HeBios()
  {
    static const uint32_t RAW[] = {
#include "module/players/xsf/scph10000_he.inc"
    };
    static const Binary::View ADAPTED(RAW, sizeof(RAW));
    return ADAPTED;
  }
}  // namespace Module::PSF
