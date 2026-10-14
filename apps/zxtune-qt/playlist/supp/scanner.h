/**
 *
 * @file
 *
 * @brief Scanner interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/supp/data_provider.h"

#include <QtCore/QThread>

#include <optional>

namespace Playlist
{
  struct ScanStatus
  {
    unsigned DoneFiles = 0;
    unsigned FoundFiles = 0;
    QString CurrentFile;
    bool SearchFinished = false;
    unsigned Progress = 0;
    QString Message;
  };

  class Scanner : public QObject
  {
    Q_OBJECT
  protected:
    explicit Scanner(QObject& parent);

  public:
    using Ptr = Scanner*;

    static Ptr Create(QObject& parent, Item::DataProvider::Ptr provider);

    virtual void AddItems(const QStringList& items) = 0;
    virtual void PasteItems(const QStringList& items) = 0;
    virtual void Pause(bool pause) = 0;
    virtual void Stop() = 0;
    virtual std::optional<ScanStatus> GetStatus() const = 0;
  signals:
    // for UI
    void ScanStarted();
    void ScanStopped();
    // for BL
    void ItemFound(Playlist::Item::Data::Ptr item);
    void ItemsFound(Playlist::Item::Collection::Ptr items);
    void ErrorOccurred(const Error& e);
  };
}  // namespace Playlist
