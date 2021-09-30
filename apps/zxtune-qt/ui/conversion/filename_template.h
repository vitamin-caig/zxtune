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

// qt includes
#include <QtWidgets/QWidget>

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
  signals:
    void SettingsChanged();
  };

  bool GetFilenameTemplate(QWidget& parent, QString& result);
}  // namespace UI
