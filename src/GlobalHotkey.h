#pragma once

#include <QObject>
#include <QString>

// Wayland cannot reliably grab global shortcuts inside the app.
// Prefer: Settings → Keyboard → Custom Shortcut → `kobiQ --toggle`
class GlobalHotkey : public QObject
{
  Q_OBJECT
public:
  explicit GlobalHotkey(QObject *parent = nullptr);

  bool registerHotkey();
  QString status() const { return m_status; }

signals:
  void activated();

private:
  QString m_status;
};
