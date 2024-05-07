/**
 *
 * @file
 *
 * @brief Thread-related utils implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/supp/thread_utils.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

bool MainThread::IsCurrent()
{
  auto instance = QCoreApplication::instance();
  return instance && instance->thread() == QThread::currentThread();
}
