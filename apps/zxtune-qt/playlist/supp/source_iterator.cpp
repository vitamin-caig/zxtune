/*
Abstract:
  Iterator data source implementations

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "source.h"
#include "ui/utils.h"
//common includes
#include <error.h>
#include <logging.h>

namespace
{
  class IteratorSource : public Playlist::ScannerSource
  {
  public:
    IteratorSource(Playlist::ScannerCallback& callback, Playlist::Item::Data::Iterator::Ptr iterator, int countHint)
      : Callback(callback)
      , Iterator(iterator)
      , CountHint(countHint)
    {
    }

    virtual unsigned Resolve()
    {
      return CountHint == -1 ? 0 : CountHint;
    }

    virtual void Process()
    {
      for (unsigned curItemNum = 0; Iterator->IsValid() && !Callback.IsCanceled(); ++curItemNum, Iterator->Next())
      {
        const Playlist::Item::Data::Ptr curItem = Iterator->Get();
        Callback.OnItem(curItem);
        const unsigned curProgress = CountHint == -1
          ? curItemNum % 100
          : curItemNum * 100 / CountHint;
        Callback.OnProgress(curProgress, curItemNum);
      }
    }
  private:
    Playlist::ScannerCallback& Callback;
    const Playlist::Item::Data::Iterator::Ptr Iterator;
    const int CountHint;
  };
}

namespace Playlist
{
  ScannerSource::Ptr ScannerSource::CreateIteratorSource(ScannerCallback& callback, Playlist::Item::Data::Iterator::Ptr iterator, int countHint)
  {
    return ScannerSource::Ptr(new IteratorSource(callback, iterator, countHint));
  }
}
