#pragma once

#include <QObject>

class MainWindow;
class QSystemTrayIcon;
class QMenu;

class TrayController : public QObject
{
  Q_OBJECT
public:
  TrayController(MainWindow *window, QObject *parent = nullptr);

private:
  void setupTray();

  MainWindow *m_window;
  QSystemTrayIcon *m_tray = nullptr;
  QMenu *m_menu = nullptr;
};
