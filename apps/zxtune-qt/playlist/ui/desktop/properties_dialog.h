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

// local includes
#include "apps/zxtune-qt/playlist/supp/model.h"
#include "apps/zxtune-qt/playlist/supp/operations_search.h"
// qt includes
#include <QtWidgets/QDialog>

class QAbstractButton;
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
