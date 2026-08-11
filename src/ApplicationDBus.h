#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class ApplicationDBus : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Application")

public:
  explicit ApplicationDBus(QObject *parent = nullptr);

public slots:
  void Activate(const QVariantMap &platformData);
  void Open(const QStringList &uris, const QVariantMap &platformData);
  void ActivateAction(const QString &actionName, const QVariantList &parameter,
                      const QVariantMap &platformData);

signals:
  void showRequested(const QString &activationToken);

private:
  static QString activationTokenFrom(const QVariantMap &platformData);
};
