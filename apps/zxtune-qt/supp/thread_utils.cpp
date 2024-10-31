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
#include "apps/zxtune-qt/supp/thread_utils.h"
// qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

bool MainThread::IsCurrent()
{
  return QThread::currentThread() == QCoreApplication::instance()->thread();
}
