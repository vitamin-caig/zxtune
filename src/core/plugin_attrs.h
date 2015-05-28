/**
*
* @file
*
* @brief  Plugins attributes
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

namespace ZXTune
{
  /*
    Type - Module - Device type - AY38910
         |        |             - Turbosound
         |        |             - Beeper
         |        |             - YM2203
         |        |             - TURBOFM
         |        |             - DAC
         |        |             - SAA1099
         |        |             - MOS6581
         |        |             - SPC700
         |        |             - Multi
         |        |
         |        - Conversions - OUT
         |                      - PSG
         |                      - YM
         |                      - ZX50
         |                      - TXT
         |                      - AYDUMP
         |                      - FYM
         |
         - Container - User type - Disk image
                     |           - Snapshot
                     |           - Archive
                     |           - Compressor
                     |           - Decompiler
                     |           - MultitrackModule
                     |           - Scaner
                     |           
                     - Traits - Plain
                              - OnceApplied
                              - Multifile
                              - Directories
  */
  
  namespace Capabilities
  {
    namespace Category
    {
      enum
      {
        MODULE    = 0x00000000,
        CONTAINER = 0x10000000,
      
        MASK      = 0xf0000000
      };
    }
    
    namespace Module
    {
      namespace Type
      {
        enum
        {
          TRACK    = 0x00000000,
          STREAM   = 0x00000001,
          EMULATED = 0x00000002,
          MULTI    = 0x00000003,
          
          MASK     = 0x0000000f
        };
      }
      
      namespace Device
      {
        enum
        {
          AY38910    = 0x00000010,
          TURBOSOUND = 0x00000020,
          BEEPER     = 0x00000040,
          YM2203     = 0x00000080,
          TURBOFM    = 0x00000100,
          DAC        = 0x00000200,
          SAA1099    = 0x00000400,
          MOS6581    = 0x00000800,
          SPC700     = 0x00001000,
          MULTI      = 0x00002000,
          RP2A0X     = 0x00004000,

          MASK       = 0x0000fff0
        };
      }
      
      namespace Conversion
      {
        enum
        {
          OUT        = 0x00010000,
          PSG        = 0x00020000,
          YM         = 0x00040000,
          ZX50       = 0x00080000,
          TXT        = 0x00100000,
          AYDUMP     = 0x00200000,
          FYM        = 0x00400000,

          MASK       = 0x00ff0000
        };
      }
    }
    
    namespace Container
    {
      namespace Type
      {
        enum
        {
          ARCHIVE    = 0x00000000,
          COMPRESSOR = 0x00000001,
          SNAPSHOT   = 0x00000002,
          DISKIMAGE  = 0x00000003,
          DECOMPILER = 0x00000004,
          MULTITRACK = 0x00000005,
          SCANER     = 0x00000006,
          
          MASK       = 0x0000000f
        };
      }
      
      namespace Traits
      {
        enum
        {
          //! Container may have directory structure
          DIRECTORIES = 0x00000010,
          //! Use plain transformation, can be covered by scaner
          PLAIN       = 0x00000020,
          //! Plugin should be applied only once
          ONCEAPPLIED = 0x00000040,
          
          MASK        = 0x000000f0
        };
      }
    }
  }
}
