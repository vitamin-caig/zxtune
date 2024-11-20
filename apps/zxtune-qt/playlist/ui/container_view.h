/**
 *
 * @file
 *
 * @brief Playlist container view interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/controller.h"
#include "apps/zxtune-qt/playlist/supp/data.h"

#include "parameters/container.h"

#include <QtWidgets/QWidget>

class QMenu;
class QStringList;

namespace Playlist::UI
{
  class ContainerView : public QWidget
  {
    Q_OBJECT
  protected:
    explicit ContainerView(QWidget& parent);

  public:
    // creator
    static ContainerView* Create(QWidget& parent, Parameters::Container::Ptr parameters);

    virtual void Setup() = 0;
    virtual void Teardown() = 0;

    virtual void Open(const QStringList& args) = 0;

    virtual QMenu* GetActionsMenu() const = 0;

    // navigate
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual void Finish() = 0;
    virtual void Next() = 0;
    virtual void Prev() = 0;
    // actions
    virtual void Clear() = 0;
    virtual void AddFiles() = 0;
    virtual void AddFolder() = 0;
    // playlist actions
    virtual void CreatePlaylist() = 0;
    virtual void LoadPlaylist() = 0;
    virtual void SavePlaylist() = 0;
    virtual void RenamePlaylist() = 0;

    virtual void CloseCurrentPlaylist() = 0;
    virtual void ClosePlaylist(int index) = 0;
  signals:
    void Activated(Playlist::Item::Data::Ptr);
    void ItemActivated(Playlist::Item::Data::Ptr);
    void Deactivated();
  };
}  // namespace Playlist::UI
