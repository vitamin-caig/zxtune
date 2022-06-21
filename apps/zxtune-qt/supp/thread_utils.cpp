/**
 *
 * @file
 *
 * @brief Thread-related utils implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "thread_utils.h"
// common includes
#include <contract.h>
// qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

bool IOThread::IsCurrent()
{
  return QThread::currentThread() == Instance()->thread();
}

IOThread::IOThread()
{
  auto* thread = new QThread();
  moveToThread(thread);
  Require(QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater())));
  Require(QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), thread, SLOT(quit())));
  thread->setObjectName("IOThread");
  thread->start();
}

IOThread* IOThread::Instance()
{
  static IOThread INSTANCE;
  return &INSTANCE;
}
