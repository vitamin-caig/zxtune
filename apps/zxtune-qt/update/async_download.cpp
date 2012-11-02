/*
Abstract:
  Async download helper implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "async_download.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/utils.h"
//common includes
#include <error.h>
//library includes
#include <io/api.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace Async
{
  class DownloadOperation : public Operation
  {
  public:
    DownloadOperation(const String& src, DownloadCallback& cb, Parameters::Accessor::Ptr params)
      : Src(src)
      , Callback(cb)
      , Params(params)
    {
    }

    virtual Error Prepare()
    {
      return Error();//TODO: check availability
    }

    virtual Error Execute()
    {
      try
      {
        const Binary::Container::Ptr res = IO::OpenData(Src, *Params, Callback);
        Callback.Complete(res);
        return Error();
      }
      catch (const Error& err)
      {
        Callback.Failed();
        return err;
      }
    }
  private:
    const String Src;
    DownloadCallback& Callback;
    const Parameters::Accessor::Ptr Params;
  };

  Activity::Ptr CreateDownloadActivity(const QUrl& url, DownloadCallback& cb)
  {
    const String path = FromQString(url.toString());
    const Parameters::Accessor::Ptr params = GlobalOptions::Instance().Get();
    const Async::Operation::Ptr op = boost::make_shared<DownloadOperation>(path, boost::ref(cb), params);
    return Async::Activity::Create(op);
  }
}
