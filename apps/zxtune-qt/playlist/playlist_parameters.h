/*
Abstract:
  Playlist parameters definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_PARAMETERS_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_PARAMETERS_H_DEFINED

//common includes
#include <parameters.h>

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace Playlist
    {
      //@{
      //! @name Use deep scanning while processing input files

      //! Parameter name
      const Char DEEP_SCANNING[] =
      {
        'z','x','t','u','n','e','-','q','t','.','p','l','a','y','l','i','s','t','.','d','e','e','p','_','s','c','a','n','n','i','n','g','\0'
      };

      //! Default value
      const IntType DEEP_SCANNING_DEFAULT = 1;
      //@}

      //@{
      //! @name Loop playlist playback

      //! Parameter name
      const Char LOOPED[] =
      {
        'z','x','t','u','n','e','-','q','t','.','p','l','a','y','l','i','s','t','.','l','o','o','p','e','d','\0'
      };

      //! Default value
      const IntType LOOPED_DEFAULT = 0;
      //@}

      //@{
      //! @name Randomize playlist playback

      //! Parameter name
      const Char RANDOMIZED[] =
      {
        'z','x','t','u','n','e','-','q','t','.','p','l','a','y','l','i','s','t','.','r','a','n','d','o','m','i','z','e','d','\0'
      };

      const IntType RANDOMIZED_DEFAULT = 0;
      //@}
    }
  }
}
#endif //ZXTUNE_QT_PLAYLIST_PARAMETERS_H_DEFINED
