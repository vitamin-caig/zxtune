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

//qt includes
#include <QtGui/QWidget>

class Error;

namespace Update
{
  class CheckOperation : public QObject
  {
    Q_OBJECT
  public:
    static CheckOperation* Create(QWidget& parent);
  public slots:
    virtual void Execute() = 0;
  private slots:
    virtual void ExecuteBackground() = 0;
  signals:
    void ErrorOccurred(const Error&);
  };
};
