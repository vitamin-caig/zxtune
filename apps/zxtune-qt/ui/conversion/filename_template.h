/**
 *
 * @file
 *
 * @brief Filename template building widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

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
  signals:
    void SettingsChanged();
  };

  bool GetFilenameTemplate(QWidget& parent, QString& result);
}  // namespace UI
