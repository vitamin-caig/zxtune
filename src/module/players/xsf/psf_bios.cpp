/**
* 
* @file
*
* @brief  HighlyExperimental BIOS access implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/xsf/psf_bios.h"
//library includes
#include <binary/data_adapter.h>

namespace Module
{
  namespace PSF
  {
    const Binary::Data& GetSCPH10000HeBios()
    {
      static const uint32_t RAW[] =
      {
#include "module/players/xsf/scph10000_he.inc"
      };
      static const Binary::DataAdapter ADAPTED(RAW, sizeof(RAW));
      return ADAPTED;
    }
  }
}
