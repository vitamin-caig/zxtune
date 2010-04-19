/**
*
* @file      sound/sound_parameters.h
* @brief     Render parameters names
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#ifndef __SOUND_PARAMETERS_H_DEFINED__
#define __SOUND_PARAMETERS_H_DEFINED__

//common includes
#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    //! @brief %Sound parameters namespace
    namespace Sound
    {
      //@{
      //! @name %Sound frequency in Hz
      
      //! Default value- 44.1kHz
      const IntType FREQUENCY_DEFAULT = 44100;
      //! Parameter name
      const Char FREQUENCY[] =
      {
        'z','x','t','u','n','e','.','s','o','u','n','d','.','f','r','e','q','u','e','n','c','y','\0'
      };
      //@}
      
      //@{
      //! @name PSG clockrate in Hz
      
      //! Default value- 1.75MHz
      const IntType CLOCKRATE_DEFAULT = 1750000;
      //! Parameter name
      const Char CLOCKRATE[] =
      {
        'z','x','t','u','n','e','.','s','o','u','n','d','.','c','l','o','c','k','r','a','t','e','\0'
      };
      //@}
      
      //@{
      //! @name Frame duration in microseconds
      
      //! Default value- 20mS (50Hz)
      const IntType FRAMEDURATION_DEFAULT = 20000;
      //! Parameter name
      const Char FRAMEDURATION[] =
      {
        'z','x','t','u','n','e','.','s','o','u','n','d','.','f','r','a','m','e','d','u','r','a','t','i','o','n','\0'
      };
      
      //@{
      //! @name Loop mode
      //! @see ZXTune#Sound#LoopMode

      //! Default value (no looping)
      const IntType LOOPMODE_DEFAULT = 0;
      //! Parameter name
      const Char LOOPMODE[] =
      {
        'z','x','t','u','n','e','.','s','o','u','n','d','.','l','o','o','p','m','o','d','e','\0'
      };
      //@}
    }
  }
}

#endif //__SOUND_PARAMETERS_H_DEFINED__
