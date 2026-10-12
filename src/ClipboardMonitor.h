#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QString>

class QClipboard;
class HistoryStore;

class ClipboardMonitor : public QObject
{
  Q_OBJECT
public:
  ClipboardMonitor(HistoryStore *store, QObject *parent = nullptr);
  ~ClipboardMonitor() override;

  void ignoreChangesFor(int milliseconds);

  // External ingest (GNOME Shell extension / D-Bus).
  void ingestText(const QString &text);
  void ingestImageNotify();

signals:
  void historyChanged();

private slots:
  void onClipboardChanged();
  void onWlPasteWatchReady();
  void onGPasteUpdate(const QString &action, const QString &uuid, qulonglong index);

private:
  bool shouldIgnore() const;
  void startWaylandFallbacks();
  void stopWlPasteWatch();
  void tryStartWlPasteWatch();
  void tryConnectGPaste();
  bool storeCurrentClipboard();
  bool storeText(const QString &text);
  bool storeImageFromPng(const QByteArray &png);
  void fetchImageViaWlPaste();

  HistoryStore *m_store;
  QClipboard *m_clipboard;
  QElapsedTimer m_ignoreTimer;
  int m_ignoreForMs = 0;
  QProcess *m_wlPasteWatch = nullptr;
  bool m_gpasteConnected = false;
};
