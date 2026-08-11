#include "TrayController.h"

#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTimer>

static QIcon kobiQIcon()
{
  QIcon icon;
  icon.addFile(QStringLiteral(":/icons/kobiQ-tray.png"));
  icon.addFile(QStringLiteral(":/icons/kobiQ.svg"));
  return icon;
}

TrayController::TrayController(MainWindow *window, QObject *parent)
  : QObject(parent)
  , m_window(window)
{
  const QIcon icon = kobiQIcon();
  m_window->setWindowIcon(icon);
  QApplication::setWindowIcon(icon);

  // Defer tray setup so a missing StatusNotifier service on GNOME cannot
  // block startup / window the window from appearing.
  QTimer::singleShot(0, this, &TrayController::setupTray);
}

void TrayController::setupTray()
{
  if (!QSystemTrayIcon::isSystemTrayAvailable())
    return;

  m_tray = new QSystemTrayIcon(this);
  m_tray->setIcon(kobiQIcon());
  m_tray->setToolTip(QStringLiteral("kobiQ — clipboard history"));

  m_menu = new QMenu();
  auto *showAction = m_menu->addAction(QStringLiteral("Show history"));
  m_menu->addSeparator();
  auto *exitAction = m_menu->addAction(QStringLiteral("Exit"));
  m_tray->setContextMenu(m_menu);

  connect(showAction, &QAction::triggered, m_window, &MainWindow::showAndFocus);
  connect(exitAction, &QAction::triggered, m_window, &MainWindow::exitApplication);
  connect(m_tray, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger
                || reason == QSystemTrayIcon::DoubleClick)
              m_window->showAndFocus();
          });

  m_tray->show();
}
