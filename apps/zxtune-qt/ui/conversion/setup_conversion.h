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

//local includes
#include "playlist/supp/conversion.h"
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

    virtual bool Execute(Playlist::Item::Conversion::Options& opts) = 0;
  private slots:
    virtual void UpdateDescriptions() = 0;
  };

  bool GetConversionParameters(QWidget& parent, Playlist::Item::Conversion::Options& opts);
}
