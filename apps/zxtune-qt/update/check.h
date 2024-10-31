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
#include <QtCore/QString>

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
