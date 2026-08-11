#include "SingleInstance.h"

#include "InstancePaths.h"

#include <QAbstractSocket>
#include <QDir>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcessEnvironment>

namespace {

bool connectToPrimary(QLocalSocket &socket, const QString &socketPath, int timeoutMs)
{
  socket.connectToServer(socketPath);
  return socket.waitForConnected(timeoutMs);
}

} // namespace

bool SingleInstance::isPrimaryRunning()
{
  QLocalSocket socket;
  return connectToPrimary(socket, InstancePaths::socketPath(), 300);
}

bool SingleInstance::requestShow()
{
  QLocalSocket socket;
  const QString socketPath = InstancePaths::socketPath();
  if (!connectToPrimary(socket, socketPath, 500))
    return false;

  const QString token = []() {
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString activation = env.value(QStringLiteral("XDG_ACTIVATION_TOKEN"));
    if (!activation.isEmpty())
      return activation;
    return env.value(QStringLiteral("DESKTOP_STARTUP_ID"));
  }();
  QByteArray message = "show";
  if (!token.isEmpty())
    message += '\n' + token.toUtf8();
  socket.write(message);
  socket.flush();
  socket.waitForBytesWritten(500);
  socket.waitForReadyRead(500);
  socket.disconnectFromServer();
  return socket.readAll().startsWith("ok");
}

SingleInstance::SingleInstance(QObject *parent)
  : QObject(parent)
  , m_socketPath(InstancePaths::socketPath())
{
}

SingleInstance::~SingleInstance()
{
  if (m_server) {
    m_server->close();
    QLocalServer::removeServer(m_socketPath);
  }
}

bool SingleInstance::tryBecomePrimary()
{
  if (isPrimaryRunning())
    return false;

  QDir().mkpath(InstancePaths::socketDir());

  m_server = new QLocalServer(this);
  QLocalServer::removeServer(m_socketPath);
  if (!m_server->listen(m_socketPath)) {
    if (m_server->serverError() == QAbstractSocket::AddressInUseError)
      QLocalServer::removeServer(m_socketPath);
    if (!m_server->listen(m_socketPath))
      return false;
  }

  QFile pathFile(InstancePaths::socketDir() + QStringLiteral("/instance.path"));
  if (pathFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    pathFile.write(m_socketPath.toUtf8());

  connect(m_server, &QLocalServer::newConnection, this, [this]() {
    QLocalSocket *socket = m_server->nextPendingConnection();
    if (!socket)
      return;

    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
      if (socket->property("kobiqHandled").toBool())
        return;

      const QByteArray data = socket->readAll();
      if (data.isEmpty())
        return;

      QString token;
      if (data.startsWith("show")) {
        const int nl = data.indexOf('\n');
        if (nl >= 0)
          token = QString::fromUtf8(data.mid(nl + 1)).trimmed();
      } else if (!data.contains("toggle")) {
        return;
      }

      socket->setProperty("kobiqHandled", true);
      qInfo("kobiQ show requested (token: %s)", token.isEmpty() ? "empty" : "present");
      emit showRequested(token);

      socket->write("ok\n");
      socket->flush();
      socket->disconnectFromServer();
    });
  });
  return true;
}

bool SingleInstance::sendShowToPrimary()
{
  return requestShow();
}
