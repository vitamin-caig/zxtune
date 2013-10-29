/**
* 
* @file
*
* @brief Playlist import interfaces
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "container.h"
#include "playlist/supp/data_provider.h"

class QString;
class QStringList;

namespace Playlist
{
  namespace IO
  {
    //common
    Container::Ptr Open(Item::DataProvider::Ptr provider, const QString& filename);
    //specific
    Container::Ptr OpenAYL(Item::DataProvider::Ptr provider, const QString& filename);
    Container::Ptr OpenXSPF(Item::DataProvider::Ptr provider, const QString& filename);

    Container::Ptr OpenPlainList(Item::DataProvider::Ptr provider, const QStringList& uris);
  }
}
