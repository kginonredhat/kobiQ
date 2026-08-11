#include "AppVersion.h"
#include "ApplicationDBus.h"
#include "ClipboardMonitor.h"
#include "GlobalHotkey.h"
#include "HistoryStore.h"
#include "InstancePaths.h"
#include "MainWindow.h"
#include "PasteHelper.h"
#include "SingleInstance.h"
#include "TrayController.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTimer>

int main(int argc, char *argv[])
{
  // Lightweight relaunch path: no QApplication, so GNOME won't show "kobiQ is ready".
  {
    QCoreApplication probe(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kobiQ"));
    if (SingleInstance::isPrimaryRunning()) {
      if (!SingleInstance::requestShow())
        return 1;
      return 0;
    }
  }

  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("kobiQ"));
  QApplication::setOrganizationName(QStringLiteral("kobiQ"));
  QApplication::setApplicationDisplayName(QStringLiteral("kobiQ"));
  QApplication::setApplicationVersion(kobiQVersionString());
  QApplication::setQuitOnLastWindowClosed(false);
  // Set after QApplication is constructed; avoids duplicate portal app-id registration.
  QTimer::singleShot(0, &app, []() {
    QGuiApplication::setDesktopFileName(QStringLiteral("kobiQ"));
  });

  QCommandLineParser parser;
  parser.setApplicationDescription(
      QStringLiteral("kobiQ clipboard manager for Linux"));
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption toggleOption(QStringLiteral("toggle"),
                                  QStringLiteral("Show the history window"));
  parser.addOption(toggleOption);
  parser.process(app);

  SingleInstance instance;
  if (!instance.tryBecomePrimary()) {
    if (!instance.sendShowToPrimary())
      return 1;
    return 0;
  }

  QSettings settings;
  const int maxItems = settings.value(QStringLiteral("maxItems"), 1000).toInt();

  HistoryStore store(maxItems);
  store.load();

  QObject::connect(&app, &QCoreApplication::aboutToQuit, [&store]() { store.save(); });

  ClipboardMonitor monitor(&store);
  PasteHelper pasteHelper(&monitor);
  MainWindow window(&store, &pasteHelper);
  TrayController tray(&window);
  GlobalHotkey hotkey;
  ApplicationDBus dbusApi;

  // Use a dedicated D-Bus name. Qt already registers the desktop app id ("kobiQ")
  // with the xdg-desktop-portal; reusing that name here causes registration failures.
  const QString dbusService = QStringLiteral("org.kobiq.Application");
  const QString dbusPath = QStringLiteral("/org/kobiq/Application");
  QDBusConnection sessionBus = QDBusConnection::sessionBus();
  if (!sessionBus.registerService(dbusService)) {
    qWarning("Could not register D-Bus service %s: %s", qPrintable(dbusService),
             qPrintable(sessionBus.lastError().message()));
  }
  sessionBus.registerObject(dbusPath, &dbusApi, QDBusConnection::ExportAllSlots);

  QObject::connect(&monitor, &ClipboardMonitor::historyChanged, &window,
                   &MainWindow::refresh);
  QObject::connect(&instance, &SingleInstance::showRequested, &window,
                   &MainWindow::showWithActivationToken, Qt::QueuedConnection);
  QObject::connect(&dbusApi, &ApplicationDBus::showRequested, &window,
                   &MainWindow::showWithActivationToken, Qt::QueuedConnection);
  QObject::connect(&hotkey, &GlobalHotkey::activated, &window,
                   &MainWindow::showAndFocus);

  hotkey.registerHotkey();
  window.setWindowTitle(kobiQVersionedTitle());
  qInfo("kobiQ socket: %s", qPrintable(InstancePaths::socketPath()));
  qInfo("GNOME shortcut command: kobiq-activate");
  window.showAndFocus();

  return app.exec();
}
