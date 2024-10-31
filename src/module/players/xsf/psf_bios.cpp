/**
 *
 * @file
 *
 * @brief  HighlyExperimental BIOS access implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/players/xsf/psf_bios.h"

#include "types.h"

namespace Module::PSF
{
  Binary::View GetSCPH10000HeBios()
  {
    static const uint32_t RAW[] = {
#include "module/players/xsf/scph10000_he.inc"  // IWYU pragma: keep
    };
    return Binary::View{RAW, sizeof(RAW)};
  }
}  // namespace Module::PSF
