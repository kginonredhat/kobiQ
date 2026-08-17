#pragma once

#include "HistoryStore.h"

#include <QByteArray>
#include <QObject>
#include <QString>

class ClipboardMonitor;

class PasteHelper : public QObject
{
  Q_OBJECT
public:
  PasteHelper(ClipboardMonitor *monitor, QObject *parent = nullptr);

  void copyToClipboard(const ClipItem &item);
  void pasteItem(const ClipItem &item);

private:
  void putOnClipboard(const ClipItem &item);
  void putOnClipboardQt(const ClipItem &item);
  bool offerWithWlCopy(const ClipItem &item);
  static bool runWlCopy(const QByteArray &bytes, const QString &mimeType, bool primary);
  void simulatePaste();
  static bool isWayland();

  ClipboardMonitor *m_monitor;
};
