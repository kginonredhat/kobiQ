#pragma once

#include <QObject>
#include <QString>

class QLocalServer;

class SingleInstance : public QObject
{
  Q_OBJECT
public:
  explicit SingleInstance(QObject *parent = nullptr);
  ~SingleInstance() override;

  static bool isPrimaryRunning();
  static bool requestShow();

  bool tryBecomePrimary();
  bool sendShowToPrimary();

signals:
  void showRequested(const QString &activationToken);

private:
  QString m_socketPath;
  QLocalServer *m_server = nullptr;
};
