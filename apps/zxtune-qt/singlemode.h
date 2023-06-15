/**
 *
 * @file
 *
 * @brief Single instance support interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <parameters/accessor.h>
#include <strings/array.h>
// qt includes
#include <QtCore/QObject>
#include <QtCore/QPointer>

class SingleModeDispatcher : public QObject
{
  Q_OBJECT
public:
  using Ptr = QPointer<SingleModeDispatcher>;
  static Ptr Create(const Parameters::Accessor& params, Strings::Array argv);

  virtual bool StartMaster() = 0;
  virtual QStringList GetCmdline() const = 0;
signals:
  void OnSlaveStarted(const QStringList& cmdline);

private slots:
  virtual void SlaveStarted() = 0;
};
