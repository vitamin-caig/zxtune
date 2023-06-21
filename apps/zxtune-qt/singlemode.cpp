/**
 *
 * @file
 *
 * @brief Single instance support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "singlemode.h"
#include "app_parameters.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
// library includes
#include <debug/log.h>
#include <platform/version/api.h>
// qt includes
#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <memory>

namespace
{
  const Debug::Stream Dbg("SingleInstance");
}

namespace
{
  const QString SERVER_NAME(Platform::Version::PROGRAM_NAME);

  const QDataStream::Version STREAM_VERSION = QDataStream::Qt_4_6;  // compatible with up to 4.9

  class StubModeDispatcher : public SingleModeDispatcher
  {
  public:
    template<class It>
    StubModeDispatcher(It cmdBegin, It cmdEnd)
    {
      const QDir curDir;
      for (auto it = cmdBegin; it != cmdEnd; ++it)
      {
        const auto arg = ToQString(*it);
        if (arg.startsWith("--"))
        {
          Cmdline << arg;
        }
        else
        {
          Cmdline << curDir.cleanPath(curDir.absoluteFilePath(arg));
        }
      }
    }

    bool StartMaster() override
    {
      return true;
    }

    QStringList GetCmdline() const override
    {
      return Cmdline;
    }

  protected:
    QStringList Cmdline;
  };

  class SocketBasedSingleModeDispatcher : public StubModeDispatcher
  {
  public:
    template<class It>
    SocketBasedSingleModeDispatcher(It cmdBegin, It cmdEnd)
      : StubModeDispatcher(cmdBegin, cmdEnd)
    {}

    bool StartMaster() override
    {
      QLocalSocket socket;
      socket.connectToServer(SERVER_NAME, QLocalSocket::WriteOnly);
      if (socket.waitForConnected(500))
      {
        Dbg("Connected to existing server. Sending cmdline with {} args", Cmdline.size());
        SendDataTo(socket);
        return false;
      }
      else
      {
        StartLocalServer();
        return true;
      }
    }

  private:
    void SlaveStarted()
    {
      while (QLocalSocket* conn = Server->nextPendingConnection())
      {
        const std::unique_ptr<QLocalSocket> holder(conn);
        QStringList cmdline;
        ReadDataFrom(*holder, cmdline);
        Dbg("Slave passed cmdline '{}'", FromQString(cmdline.join(" ")));
        emit OnSlaveStarted(cmdline);
      }
    }

  private:
    class SocketHelper
    {
    public:
      explicit SocketHelper(QLocalSocket& sock)
        : Sock(sock)
      {}

      void Write(const QByteArray& data)
      {
        const quint32 size = data.size();
        Write(&size, sizeof(size));
        Write(data.data(), size);
      }

      void Read(QByteArray& data)
      {
        quint32 size = 0;
        Read(&size, sizeof(size));
        data.resize(size);
        Read(data.data(), size);
      }

    private:
      void Write(const void* data, quint64 size)
      {
        const char* ptr = static_cast<const char*>(data);
        quint64 rest = size;
        while (rest != 0)
        {
          if (const quint64 written = Sock.write(ptr, rest))
          {
            ptr += written;
            rest -= written;
          }
          else
          {
            Sock.waitForBytesWritten();
          }
        }
        Sock.waitForBytesWritten();
      }

      void Read(void* data, quint64 size)
      {
        char* ptr = static_cast<char*>(data);
        quint64 rest = size;
        while (rest != 0)
        {
          if (const quint64 read = Sock.read(ptr, rest))
          {
            ptr += read;
            rest -= read;
          }
          else
          {
            Sock.waitForReadyRead();
          }
        }
      }

    private:
      QLocalSocket& Sock;
    };

    void SendDataTo(QLocalSocket& socket)
    {
      QByteArray blob;
      QDataStream stream(&blob, QIODevice::WriteOnly);
      stream.setVersion(STREAM_VERSION);
      stream << Cmdline;
      stream.device()->seek(0);
      SocketHelper out(socket);
      out.Write(blob);
    }

    static void ReadDataFrom(QLocalSocket& socket, QStringList& result)
    {
      SocketHelper in(socket);
      QByteArray blob;
      in.Read(blob);
      QDataStream stream(&blob, QIODevice::ReadOnly);
      stream.setVersion(STREAM_VERSION);
      stream >> result;
    }

    void StartLocalServer()
    {
      Server = std::make_unique<QLocalServer>(this);
      Require(
          connect(Server.get(), &QLocalServer::newConnection, this, &SocketBasedSingleModeDispatcher::SlaveStarted));
      while (!Server->listen(SERVER_NAME))
      {
        if (Server->serverError() == QAbstractSocket::AddressInUseError && QLocalServer::removeServer(SERVER_NAME))
        {
          Dbg("Try to restore from previously crashed session");
          continue;
        }
        else
        {
          Dbg("Failed to start local server");
          break;
        }
      }
    }

  private:
    std::unique_ptr<QLocalServer> Server;
  };
}  // namespace

SingleModeDispatcher::Ptr SingleModeDispatcher::Create(const Parameters::Accessor& params, Strings::Array argv)
{
  Parameters::IntType val = Parameters::ZXTuneQT::SINGLE_INSTANCE_DEFAULT;
  params.FindValue(Parameters::ZXTuneQT::SINGLE_INSTANCE, val);
  auto cmdBegin = argv.begin();
  ++cmdBegin;
  const auto cmdEnd = argv.end();
  if (val != 0)
  {
    Dbg("Working in single instance mode");
    return new SocketBasedSingleModeDispatcher(cmdBegin, cmdEnd);
  }
  else
  {
    Dbg("Working in multiple instances mode");
    return new StubModeDispatcher(cmdBegin, cmdEnd);
  }
}
