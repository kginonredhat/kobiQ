#pragma once

#include "HistoryStore.h"

#include <QObject>

class ClipboardMonitor;

class PasteHelper : public QObject
{
  Q_OBJECT
public:
  PasteHelper(ClipboardMonitor *monitor, QObject *parent = nullptr);

  void pasteItem(const ClipItem &item);

private:
  void putOnClipboard(const ClipItem &item);
  void simulatePaste();

  ClipboardMonitor *m_monitor;
};
