/**
* 
* @file
*
* @brief Conversion setup dialog interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/accessor.h>
//library includes
#include <sound/service.h>
//qt includes
#include <QtGui/QDialog>

namespace UI
{
  class SetupConversionDialog : public QDialog
  {
    Q_OBJECT
  protected:
    explicit SetupConversionDialog(QWidget& parent);
  public:
    typedef boost::shared_ptr<SetupConversionDialog> Ptr;

    static Ptr Create(QWidget& parent);

    virtual Sound::Service::Ptr Execute(String& type) = 0;
  private slots:
    virtual void UpdateDescriptions() = 0;
  };

  Sound::Service::Ptr GetConversionService(QWidget& parent, String& type);
}
