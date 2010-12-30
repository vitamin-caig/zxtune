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
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
//qt includes
#include <QtGui/QFileDialog>

namespace
{
  class FileDialogImpl : public FileDialog
  {
  public:
    explicit FileDialogImpl(QWidget& parent)
      : Dialog(&parent, QString(), QDir::currentPath())
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

    virtual bool SaveFile(const QString& title, const QString& suffix, const QString& filter, QString& filename)
    {
      Dialog.setWindowTitle(title);
      Dialog.setDefaultSuffix(suffix);
      Dialog.setNameFilter(filter);
      Dialog.selectFile(filename);
      Dialog.setFileMode(QFileDialog::AnyFile);
      Dialog.setOption(QFileDialog::ShowDirsOnly, false);
      SetRWMode();
      if (ProcessDialog())
      {
        const QStringList& files = Dialog.selectedFiles();
        filename = files.front();
        return true;
      }
      return false;
    }
  private:
    bool ProcessDialog()
    {
      return QDialog::Accepted == Dialog.exec();
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
    QFileDialog Dialog;
  };
}

FileDialog::Ptr FileDialog::Create(QWidget& parent)
{
  return boost::make_shared<FileDialogImpl>(boost::ref(parent));
}
