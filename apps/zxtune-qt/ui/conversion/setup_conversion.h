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
#include "playlist/supp/data.h"
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

    virtual Playlist::Item::Conversion::Options::Ptr Execute() = 0;
  private slots:
    virtual void UpdateDescriptions() = 0;
  };

  //raw format
  Playlist::Item::Conversion::Options::Ptr GetExportParameters(QWidget& parent);
  //sound formats (TBD dumps)
  Playlist::Item::Conversion::Options::Ptr GetConvertParameters(QWidget& parent);
  //universal for single item
  Playlist::Item::Conversion::Options::Ptr GetSaveAsParameters(Playlist::Item::Data::Ptr item);
}
