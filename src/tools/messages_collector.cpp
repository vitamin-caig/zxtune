/*
Abstract:
  Messages collector implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <messages_collector.h>
#include <string_helpers.h>

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

    virtual uint_t CountMessages() const
    {
      return Messages.size();
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
