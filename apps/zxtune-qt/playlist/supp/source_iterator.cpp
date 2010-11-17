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
  class IteratorSource : public ScannerSource
  {
  public:
    IteratorSource(ScannerCallback& callback, Playitem::Iterator::Ptr iterator, int countHint)
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
        const Playitem::Ptr curItem = Iterator->Get();
        Callback.OnPlayitem(curItem);
        const unsigned curProgress = CountHint == -1
          ? curItemNum % 100
          : curItemNum * 100 / CountHint;
        Callback.OnProgress(curProgress, curItemNum);
      }
    }
  private:
    ScannerCallback& Callback;
    const Playitem::Iterator::Ptr Iterator;
    const int CountHint;
  };
}

ScannerSource::Ptr ScannerSource::CreateIteratorSource(ScannerCallback& callback, Playitem::Iterator::Ptr iterator, int countHint)
{
  return ScannerSource::Ptr(new IteratorSource(callback, iterator, countHint));
}
