#include "ClipboardMonitor.h"
#include "GlobalHotkey.h"
#include "HistoryStore.h"
#include "MainWindow.h"
#include "PasteHelper.h"
#include "SingleInstance.h"
#include "TrayController.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QMessageBox>
#include <QSettings>

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("kobiQ"));
  QApplication::setOrganizationName(QStringLiteral("kobiQ"));
  QApplication::setApplicationDisplayName(QStringLiteral("kobiQ"));
  QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
  QApplication::setQuitOnLastWindowClosed(false);

  QCommandLineParser parser;
  parser.setApplicationDescription(
      QStringLiteral("Local-first clipboard manager inspired by Ditto"));
  parser.addHelpOption();
  parser.addVersionOption();
  QCommandLineOption toggleOption(QStringLiteral("toggle"),
                                  QStringLiteral("Show/hide the history window"));
  parser.addOption(toggleOption);
  parser.process(app);

  SingleInstance instance(QStringLiteral("kobiq-single-instance"));
  if (!instance.tryBecomePrimary()) {
    if (!instance.sendToggleToPrimary()) {
      QMessageBox::warning(
          nullptr, QStringLiteral("kobiQ"),
          QStringLiteral("kobiQ is already running, but could not reach it."));
      return 1;
    }
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

  QObject::connect(&monitor, &ClipboardMonitor::historyChanged, &window,
                   &MainWindow::refresh);
  QObject::connect(&instance, &SingleInstance::toggleRequested, &window,
                   &MainWindow::toggleVisible);
  QObject::connect(&hotkey, &GlobalHotkey::activated, &window,
                   &MainWindow::toggleVisible);

  hotkey.registerHotkey();
  window.setWindowTitle(QStringLiteral("kobiQ"));
  // Keep quiet on start; tray is enough. Opening via --toggle still works.
  if (parser.isSet(toggleOption))
    window.toggleVisible();

  return app.exec();
}
