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

#include "apps/zxtune-qt/playlist/supp/conversion.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QDialog>  // IWYU pragma: export

#include <memory>

class QWidget;

namespace Playlist::Item
{
  class Data;
}  // namespace Playlist::Item

namespace UI
{
  class SetupConversionDialog : public QDialog
  {
    Q_OBJECT
  protected:
    explicit SetupConversionDialog(QWidget& parent);

  public:
    using Ptr = std::shared_ptr<SetupConversionDialog>;

    static Ptr Create(QWidget& parent);

    virtual Playlist::Item::Conversion::Options::Ptr Execute() = 0;
  };

  // raw format
  Playlist::Item::Conversion::Options::Ptr GetExportParameters(QWidget& parent);
  // sound formats (TBD dumps)
  Playlist::Item::Conversion::Options::Ptr GetConvertParameters(QWidget& parent);
  // universal for single item
  Playlist::Item::Conversion::Options::Ptr GetSaveAsParameters(const Playlist::Item::Data& item);
}  // namespace UI
