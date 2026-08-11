#include "GlobalHotkey.h"

#include <QGuiApplication>

GlobalHotkey::GlobalHotkey(QObject *parent)
  : QObject(parent)
{
}

bool GlobalHotkey::registerHotkey()
{
  const QString platform = QGuiApplication::platformName();
  if (platform.contains(QStringLiteral("wayland"), Qt::CaseInsensitive)) {
    m_status = QStringLiteral(
        "Wayland: bind a custom shortcut to `kobiQ --toggle` (Ctrl+`).");
    return false;
  }

  m_status = QStringLiteral(
      "Use tray click or `kobiQ --toggle`. Global grab coming later for X11.");
  return false;
}
