/**
 *
 * @file
 *
 * @brief Update checking interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtCore/QObject>

class Error;
class QWidget;

namespace Update
{
  class CheckOperation : public QObject
  {
    Q_OBJECT
  public:
    static CheckOperation* Create(QWidget& parent);

    virtual void Execute() = 0;
  };
};  // namespace Update
