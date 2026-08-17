#pragma once

#include <QElapsedTimer>
#include <QObject>

class QClipboard;
class HistoryStore;

class ClipboardMonitor : public QObject
{
  Q_OBJECT
public:
  ClipboardMonitor(HistoryStore *store, QObject *parent = nullptr);
  void ignoreChangesFor(int milliseconds);

signals:
  void historyChanged();

private slots:
  void onClipboardChanged();

private:
  bool shouldIgnore() const;

  HistoryStore *m_store;
  QClipboard *m_clipboard;
  QElapsedTimer m_ignoreTimer;
  int m_ignoreForMs = 0;
};
