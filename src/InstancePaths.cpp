#include "InstancePaths.h"

#include <QDir>
#include <QStandardPaths>

namespace InstancePaths {

QString socketDir()
{
  const QString runtime =
      QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
  return runtime + QStringLiteral("/kobiq");
}

QString socketPath()
{
  return socketDir() + QStringLiteral("/instance.sock");
}

} // namespace InstancePaths
