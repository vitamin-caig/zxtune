/**
 *
 * @file
 *
 * @brief Playlist context menu interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtCore/QObject>  // IWYU pragma: export
#include <QtCore/QString>

namespace Playlist::UI
{
  class ItemsContextMenu : public QObject
  {
    Q_OBJECT
  protected:
    explicit ItemsContextMenu(QObject& parent);

  public:
    virtual void PlaySelected() const = 0;
    virtual void RemoveSelected() const = 0;
    virtual void CropSelected() const = 0;
    virtual void GroupSelected() const = 0;
    virtual void RemoveAllDuplicates() const = 0;
    virtual void RemoveDuplicatesOfSelected() const = 0;
    virtual void RemoveDuplicatesInSelected() const = 0;
    virtual void RemoveAllUnavailable() const = 0;
    virtual void RemoveUnavailableInSelected() const = 0;
    virtual void SelectAllRipOffs() const = 0;
    virtual void SelectRipOffsOfSelected() const = 0;
    virtual void SelectRipOffsInSelected() const = 0;
    virtual void SelectSameTypesOfSelected() const = 0;
    virtual void SelectSameFilesOfSelected() const = 0;
    virtual void CopyPathToClipboard() const = 0;
    virtual void ShowAllStatistic() const = 0;
    virtual void ShowStatisticOfSelected() const = 0;
    virtual void ExportAll() const = 0;
    virtual void ExportSelected() const = 0;
    virtual void ConvertSelected() const = 0;
    virtual void SaveAsSelected() const = 0;
    virtual void SelectFound() const = 0;
    virtual void SelectFoundInSelected() const = 0;
    virtual void ShowPropertiesOfSelected() const = 0;
    virtual void ShuffleAll() const = 0;
  };
}  // namespace Playlist::UI
