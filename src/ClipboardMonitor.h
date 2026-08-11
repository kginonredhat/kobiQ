#pragma once

#include <QObject>

class QClipboard;
class HistoryStore;

class ClipboardMonitor : public QObject
{
  Q_OBJECT
public:
  ClipboardMonitor(HistoryStore *store, QObject *parent = nullptr);

signals:
  void historyChanged();

private slots:
  void onClipboardChanged();

private:
  HistoryStore *m_store;
  QClipboard *m_clipboard;
  bool m_ignoreNext = false;

public:
  void setIgnoreNext(bool ignore) { m_ignoreNext = ignore; }
};
