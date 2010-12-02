/*
Abstract:
  Playlist entity and view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_SUPP_DATA_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_DATA_H_DEFINED

//common includes
#include <iterator.h>
#include <parameters.h>
//library includes
#include <core/module_holder.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Playlist
{
  namespace Item
  {
    class Data
    {
    public:
      typedef boost::shared_ptr<const Data> Ptr;
      typedef ObjectIterator<Data::Ptr> Iterator;

      virtual ~Data() {}

      //common
      virtual ZXTune::Module::Holder::Ptr GetModule() const = 0;
      virtual Parameters::Container::Ptr GetAdjustedParameters() const = 0;
      //playlist-related
      virtual bool IsValid() const = 0;
      virtual String GetType() const = 0;
      virtual String GetTitle() const = 0;
      virtual unsigned GetDurationValue() const = 0;
      virtual String GetDurationString() const = 0;
      virtual String GetTooltip() const = 0;
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_DATA_H_DEFINED
