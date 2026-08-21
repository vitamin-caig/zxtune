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

#include "binary/compression/zlib.h"

namespace Module::PSF
{
  Binary::Data::Ptr GetSCPH10000HeBios()
  {
    static const uint8_t PACKED[] = {
#include "module/players/xsf/scph10000_he.inc"
    };
    return Binary::Compression::Zlib::Decompress(Binary::View{PACKED, sizeof(PACKED)}, 524288);
  }
}  // namespace Module::PSF
