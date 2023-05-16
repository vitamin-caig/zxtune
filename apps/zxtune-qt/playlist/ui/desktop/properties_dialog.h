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
#include "playlist/supp/model.h"
#include "playlist/supp/operations_search.h"
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
    typedef std::shared_ptr<PropertiesDialog> Ptr;

    static Ptr Create(QWidget& parent, Item::Data::Ptr item);
  private slots:
    virtual void ButtonClicked(QAbstractButton* button) = 0;
  signals:
    void ResetToDefaults();
  };

  void ExecutePropertiesDialog(QWidget& parent, Model::Ptr model, Playlist::Model::IndexSet::Ptr scope);
}  // namespace Playlist::UI
