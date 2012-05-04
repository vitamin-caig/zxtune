/*
Abstract:
  File dialog implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "filedialog.h"
#include "supp/options.h"
#include "ui/utils.h"
//common includes
#include <parameters.h>
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtGui/QFileDialog>

namespace
{
  const Char CURRENT_DIR_PARAMETER[] = {'z', 'x', 't', 'u', 'n', 'e', '-', 'q', 't', '.', 'u', 'i', '.', 'b', 'r', 'o', 'w', 's', 'e', '_', 'd', 'i', 'r', '\0'};

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

  class GuiFileDialog : public FileDialog
  {
  public:
    explicit GuiFileDialog(Parameters::Container::Ptr params)
      : Params(params)
    {
      Dialog.setViewMode(QFileDialog::Detail);
      Dialog.setOption(QFileDialog::DontUseNativeDialog, true);
      Dialog.setOption(QFileDialog::HideNameFilterDetails, true);
    }

    virtual bool OpenSingleFile(const QString& title, const QString& filters, QString& file)
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

    virtual bool OpenMultipleFiles(const QString& title, const QString& filters, QStringList& files)
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

    virtual bool OpenFolder(const QString& title, QString& folder)
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

    virtual bool SaveFile(const QString& title, const QString& suffix, const QStringList& filters, QString& filename, int* usedFilter)
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
      Dialog.setDirectory(RestoreDir());
      if (QDialog::Accepted == Dialog.exec())
      {
        StoreDir(Dialog.directory().absolutePath());
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

    QString RestoreDir() const
    {
      String dir;
      if (Params->FindValue(CURRENT_DIR_PARAMETER, dir))
      {
        return ToQString(dir);
      }
      return QDir::currentPath();
    }

    void StoreDir(const QString& dir)
    {
      Params->SetValue(CURRENT_DIR_PARAMETER, FromQString(dir));
    }
  private:
    const Parameters::Container::Ptr Params;
    QFileDialog Dialog;
  };

  //TODO: temporary workaround
  class DynamicFileDialog : public FileDialog
  {
  public:
    virtual bool OpenSingleFile(const QString& title, const QString& filters, QString& file)
    {
      return GetDelegate()->OpenSingleFile(title, filters, file);
    }

    virtual bool OpenMultipleFiles(const QString& title, const QString& filters, QStringList& files)
    {
      return GetDelegate()->OpenMultipleFiles(title, filters, files);
    }

    virtual bool OpenFolder(const QString& title, QString& folder)
    {
      return GetDelegate()->OpenFolder(title, folder);
    }

    virtual bool SaveFile(const QString& title, const QString& suffix, const QStringList& filters, QString& filename, int* usedFilter)
    {
      return GetDelegate()->SaveFile(title, suffix, filters, filename, usedFilter);
    }
  private:
    boost::shared_ptr<FileDialog> GetDelegate()
    {
      return boost::make_shared<GuiFileDialog>(GlobalOptions::Instance().Get());
    }
  };
}

FileDialog& FileDialog::Instance()
{
  static DynamicFileDialog instance;
  return instance;
}
