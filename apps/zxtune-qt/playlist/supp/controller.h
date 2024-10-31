/**
 *
 * @file
 *
 * @brief Playlist controller interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/playlist/io/container.h"
#include "apps/zxtune-qt/playlist/supp/data_provider.h"
#include "apps/zxtune-qt/playlist/supp/model.h"
#include "apps/zxtune-qt/playlist/supp/scanner.h"

#include <tools/iterators.h>

#include <QtCore/QObject>

namespace Playlist
{
  namespace Item
  {
    // dynamic part
    enum State
    {
      STOPPED,
      PAUSED,
      PLAYING,
      ERROR,
    };

    enum Playorder
    {
      LOOPED = 1,
      RANDOMIZED = 2
    };

    class StateCallback
    {
    public:
      virtual ~StateCallback() = default;

      virtual State GetState(const QModelIndex& index) const = 0;
    };

    class Iterator : public QObject
    {
      Q_OBJECT
    protected:
      explicit Iterator(QObject& parent);

    public:
      using Ptr = Iterator*;

      // access
      virtual unsigned GetIndex() const = 0;
      virtual State GetState() const = 0;
      // change
      virtual void SetState(State state) = 0;
      // navigate
      virtual bool Next(unsigned playorderMode) = 0;
      virtual bool Prev(unsigned playorderMode) = 0;

      // select item without activation
      virtual void Select(unsigned idx) = 0;

      // navigate
      virtual void Reset() = 0;
      virtual void Reset(unsigned idx) = 0;
    signals:
      void Activated(Playlist::Item::Data::Ptr);
      void ItemActivated(Playlist::Item::Data::Ptr);
      void ItemActivated(unsigned idx);
      void Deactivated();
    };
  }  // namespace Item

  class TextNotification
  {
  public:
    using Ptr = std::shared_ptr<const TextNotification>;
    virtual ~TextNotification() = default;

    virtual QString Category() const = 0;
    virtual QString Text() const = 0;
    virtual QString Details() const = 0;
  };

  class Controller : public QObject
  {
    Q_OBJECT
  public:
    using Ptr = std::shared_ptr<Controller>;
    using Iterator = ObjectIterator<Ptr>;

    static Ptr Create(const QString& name, Item::DataProvider::Ptr provider);

    virtual QString GetName() const = 0;
    virtual void SetName(const QString& name) = 0;
    virtual Scanner::Ptr GetScanner() const = 0;
    virtual Model::Ptr GetModel() const = 0;
    virtual Item::Iterator::Ptr GetIterator() const = 0;
    virtual void Shutdown() = 0;
    virtual void ShowNotification(Playlist::TextNotification::Ptr notification) = 0;
  signals:
    void Renamed(const QString& name);
  };
}  // namespace Playlist
