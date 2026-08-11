#include "TrayController.h"

#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

static QIcon kobiQIcon()
{
  // Prefer raster for reliable tray rendering at tiny sizes.
  QIcon icon;
  icon.addFile(QStringLiteral(":/icons/kobiQ-tray.png"));
  icon.addFile(QStringLiteral(":/icons/kobiQ.svg"));
  return icon;
}

TrayController::TrayController(MainWindow *window, QObject *parent)
  : QObject(parent)
  , m_window(window)
{
  m_tray = new QSystemTrayIcon(this);
  const QIcon icon = kobiQIcon();
  m_tray->setIcon(icon);
  m_window->setWindowIcon(icon);
  QApplication::setWindowIcon(icon);
  m_tray->setToolTip(QStringLiteral("kobiQ — clipboard history"));

  m_menu = new QMenu();
  auto *showAction = m_menu->addAction(QStringLiteral("Show history"));
  auto *quitAction = m_menu->addAction(QStringLiteral("Quit"));
  m_tray->setContextMenu(m_menu);

  connect(showAction, &QAction::triggered, m_window, &MainWindow::toggleVisible);
  connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
  connect(m_tray, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger
                || reason == QSystemTrayIcon::DoubleClick)
              m_window->toggleVisible();
          });

  m_tray->show();
}
