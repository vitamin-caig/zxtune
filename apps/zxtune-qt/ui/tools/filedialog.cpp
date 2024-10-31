/**
 *
 * @file
 *
 * @brief File dialog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/tools/filedialog.h"

#include "apps/zxtune-qt/ui/state.h"

#include "string_view.h"

#include <QtCore/QList>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>

namespace
{
  const auto FILEDIALOG_NAMESPACE = "FileDialog"sv;

  int GetFilterIndex(const QStringList& filters, const QString& filter)
  {
    for (int idx = 0; idx != filters.length(); ++idx)
    {
      if (filters[idx].startsWith(filter))
      {
        return idx;
      }
    }
    return -1;
  }

  class FileDialog
  {
  public:
    FileDialog()
      : State(UI::State::Create(FILEDIALOG_NAMESPACE))
    {
      State->AddWidget(Dialog);
      State->Load();
      Dialog.setViewMode(QFileDialog::Detail);
      Dialog.setOption(QFileDialog::DontUseNativeDialog, true);
      Dialog.setOption(QFileDialog::HideNameFilterDetails, true);
    }

    bool OpenSingleFile(const QString& title, const QString& filters, QString& file)
    {
      Dialog.setWindowTitle(title);
      Dialog.setNameFilter(filters);
      Dialog.setFileMode(QFileDialog::ExistingFile);
      Dialog.setOption(QFileDialog::ShowDirsOnly, false);
      SetROMode();

      if (ProcessDialog())
      {
        const QStringList& files = Dialog.selectedFiles();
        file = files.front();
        return true;
      }
      return false;
    }

    bool OpenMultipleFiles(const QString& title, const QString& filters, QStringList& files)
    {
      Dialog.setWindowTitle(title);
      Dialog.setNameFilter(filters);
      Dialog.setFileMode(QFileDialog::ExistingFiles);
      Dialog.setOption(QFileDialog::ShowDirsOnly, false);
      SetROMode();

      if (ProcessDialog())
      {
        files = Dialog.selectedFiles();
        return true;
      }
      return false;
    }

    bool OpenFolder(const QString& title, QString& folder)
    {
      Dialog.setWindowTitle(title);
      Dialog.setFileMode(QFileDialog::Directory);
      Dialog.setOption(QFileDialog::ShowDirsOnly, true);
      SetROMode();

      if (ProcessDialog())
      {
        const QStringList& folders = Dialog.selectedFiles();
        folder = folders.front();
        return true;
      }
      return false;
    }

    bool SaveFile(const QString& title, const QString& suffix, const QStringList& filters, QString& filename,
                  int* usedFilter)
    {
      Dialog.setWindowTitle(title);
      Dialog.setDefaultSuffix(suffix);
      Dialog.setNameFilters(filters);
      Dialog.selectFile(filename);
      Dialog.setFileMode(QFileDialog::AnyFile);
      Dialog.setOption(QFileDialog::ShowDirsOnly, false);
      SetRWMode();
      if (ProcessDialog())
      {
        const QStringList& files = Dialog.selectedFiles();
        filename = files.front();
        if (usedFilter)
        {
          const QString& selectedFilter = Dialog.selectedNameFilter();
          const QStringList& availableFilters = Dialog.nameFilters();
          *usedFilter = GetFilterIndex(availableFilters, selectedFilter);
        }
        return true;
      }
      return false;
    }

  private:
    bool ProcessDialog()
    {
      if (QDialog::Accepted == Dialog.exec())
      {
        State->Save();
        return true;
      }
      return false;
    }

    void SetROMode()
    {
      Dialog.setOption(QFileDialog::ReadOnly, true);
      Dialog.setAcceptMode(QFileDialog::AcceptOpen);
    }

    void SetRWMode()
    {
      Dialog.setOption(QFileDialog::ReadOnly, false);
      Dialog.setAcceptMode(QFileDialog::AcceptSave);
    }

  private:
    const UI::State::Ptr State;
    QFileDialog Dialog;
  };
}  // namespace

namespace UI
{
  bool OpenSingleFileDialog(const QString& title, const QString& filters, QString& file)
  {
    return FileDialog().OpenSingleFile(title, filters, file);
  }

  bool OpenMultipleFilesDialog(const QString& title, const QString& filters, QStringList& files)
  {
    return FileDialog().OpenMultipleFiles(title, filters, files);
  }

  bool OpenFolderDialog(const QString& title, QString& folder)
  {
    return FileDialog().OpenFolder(title, folder);
  }

  bool SaveFileDialog(const QString& title, const QString& suffix, const QStringList& filters, QString& filename,
                      int* usedFilter)
  {
    return FileDialog().SaveFile(title, suffix, filters, filename, usedFilter);
  }
}  // namespace UI
