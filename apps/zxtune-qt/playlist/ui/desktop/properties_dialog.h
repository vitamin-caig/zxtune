/**
 *
 * @file
 *
 * @brief Playlist item properties dialog interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/model.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QDialog>  // IWYU pragma: export

#include <memory>

class QWidget;

namespace Playlist::Item
{
  class Data;
}  // namespace Playlist::Item

namespace Playlist::UI
{
  class PropertiesDialog : public QDialog
  {
    Q_OBJECT
  protected:
    explicit PropertiesDialog(QWidget& parent);

  public:
    using Ptr = std::shared_ptr<PropertiesDialog>;

    static Ptr Create(QWidget& parent, const Item::Data& item);
  signals:
    void ResetToDefaults();
  };

  void ExecutePropertiesDialog(QWidget& parent, Model::Ptr model, const Playlist::Model::IndexSet& scope);
}  // namespace Playlist::UI
