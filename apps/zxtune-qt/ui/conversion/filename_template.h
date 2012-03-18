/*
Abstract:
  Filename template building widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_FILENAME_TEMPLATE_H_DEFINED
#define ZXTUNE_QT_UI_FILENAME_TEMPLATE_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class FilenameTemplateWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit FilenameTemplateWidget(QWidget& parent);
  public:
    static FilenameTemplateWidget* Create(QWidget& parent);

    virtual QString GetFilenameTemplate() const = 0;
  private slots:
    virtual void OnBrowseDirectory() = 0;
    virtual void OnClickHint(const QString& hint) = 0;
  };

  bool GetFilenameTemplate(QWidget& parent, QString& result);
}

#endif //ZXTUNE_QT_UI_FILENAMTE_TEMPLATE_H_DEFINED
