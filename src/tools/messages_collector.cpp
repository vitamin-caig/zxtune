/*
Abstract:
  Messages collector implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <messages_collector.h>

#include <boost/algorithm/string/join.hpp>

namespace
{
  class MessagesCollectorImpl : public Log::MessagesCollector
  {
  public:
    MessagesCollectorImpl()
    {
    }

    virtual void AddMessage(const String& message)
    {
      Messages.push_back(message);
    }

    virtual bool HasMessages() const
    {
      return !Messages.empty();
    }

    virtual String GetMessages(Char delimiter) const
    {
      return boost::join(Messages, String(1, delimiter));
    }

  private:
    StringList Messages;
    std::size_t TotalSize;
  };
}

namespace Log
{
  MessagesCollector::Ptr MessagesCollector::Create()
  {
    return MessagesCollector::Ptr(new MessagesCollectorImpl());
  }
}
