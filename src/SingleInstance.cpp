#include "SingleInstance.h"

#include <QLocalServer>
#include <QLocalSocket>

SingleInstance::SingleInstance(const QString &key, QObject *parent)
  : QObject(parent)
  , m_key(key)
{
}

SingleInstance::~SingleInstance()
{
  if (m_server) {
    m_server->close();
    QLocalServer::removeServer(m_key);
  }
}

bool SingleInstance::tryBecomePrimary()
{
  QLocalServer::removeServer(m_key);
  m_server = new QLocalServer(this);
  if (!m_server->listen(m_key))
    return false;

  connect(m_server, &QLocalServer::newConnection, this, [this]() {
    QLocalSocket *socket = m_server->nextPendingConnection();
    if (!socket)
      return;
    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
      const QByteArray data = socket->readAll();
      if (data.contains("toggle"))
        emit toggleRequested();
      socket->disconnectFromServer();
      socket->deleteLater();
    });
  });
  return true;
}

bool SingleInstance::sendToggleToPrimary()
{
  QLocalSocket socket;
  socket.connectToServer(m_key);
  if (!socket.waitForConnected(500))
    return false;
  socket.write("toggle");
  socket.flush();
  socket.waitForBytesWritten(500);
  socket.disconnectFromServer();
  return true;
}
