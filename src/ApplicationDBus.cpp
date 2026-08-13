#include "ApplicationDBus.h"

QString ApplicationDBus::activationTokenFrom(const QVariantMap &platformData)
{
  return platformData.value(QStringLiteral("activation-token")).toString();
}

ApplicationDBus::ApplicationDBus(QObject *parent)
  : QObject(parent)
{
}

void ApplicationDBus::Activate(const QVariantMap &platformData)
{
  const QString token = activationTokenFrom(platformData);
  qInfo("kobiQ D-Bus activate (token: %s)", token.isEmpty() ? "empty" : "present");
  emit showRequested(token);
}

void ApplicationDBus::Open(const QStringList &, const QVariantMap &platformData)
{
  emit showRequested(activationTokenFrom(platformData));
}

void ApplicationDBus::ActivateAction(const QString &, const QVariantList &,
                                     const QVariantMap &platformData)
{
  emit showRequested(activationTokenFrom(platformData));
}
