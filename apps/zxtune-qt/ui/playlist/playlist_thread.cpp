/*
Abstract:
  Playitems process thread implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist_thread.h"
#include "playlist_thread_moc.h"
#include "playlist_ui.h"
#include <apps/base/moduleitem.h>
//common includes
#include <error.h>
#include <logging.h>
//qt includes
#include <QtCore/QMutex>
//std includes
#include <ctime>
//boost includes
#include <boost/bind.hpp>

namespace
{
  class ProcessThreadImpl : public ProcessThread
  {
  public:
    explicit ProcessThreadImpl(QWidget* owner)
      : Canceled(false)
      , ScanDisplayed(false)
    {
      setParent(owner);
    }

    virtual void AddItemPath(const String& path)
    {
      QMutexLocker lock(&QueueLock);
      if (Queue.empty() && !this->isRunning())
      {
        this->start();
      }
      Queue.push_back(path);
    }

    virtual void Cancel()
    {
      Canceled = true;
      this->wait();
    }
    
    virtual void run()
    {
      Canceled = false;
      Parameters::Map params;
      OnScanStart();
      for (;;)
      {
        StringArray pathes;
        {
          QMutexLocker lock(&QueueLock);
          if (Queue.empty())
          {
            break;
          }
          pathes.swap(Queue);
        }
        if (const Error& e = ProcessModuleItems(pathes, params,
            0, //filter
            boost::bind(&ProcessThreadImpl::DispatchProgress, this, _1), //logger
            boost::bind(&ProcessThreadImpl::OnProcessItem, this, _1)))
        {
          //TODO: check and show error
          e.GetText();
        }
      }
      OnScanStop();
    }
  private:
    void DispatchProgress(const Log::MessageData& msg)
    {
      if (0 == msg.Level)
      {
        OnProgress(msg);
      }
    }

    bool OnProcessItem(const ModuleItem& item)
    {
      OnGetItem(item);
      return !Canceled;
    }

  private:
    QMutex QueueLock;
    StringArray Queue;
    //TODO: possibly use events
    volatile bool Canceled;
    bool ScanDisplayed;
  };
}

ProcessThread* ProcessThread::Create(QWidget* owner)
{
  assert(owner);
  return new ProcessThreadImpl(owner);
}
