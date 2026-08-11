#pragma once

#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;

class SingleInstance : public QObject
{
  Q_OBJECT
public:
  explicit SingleInstance(const QString &key, QObject *parent = nullptr);
  ~SingleInstance() override;

  bool tryBecomePrimary();
  bool sendToggleToPrimary();

signals:
  void toggleRequested();

private:
  QString m_key;
  QLocalServer *m_server = nullptr;
};
