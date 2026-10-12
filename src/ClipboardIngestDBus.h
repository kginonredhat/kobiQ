#pragma once

#include <QObject>
#include <QString>

class ClipboardMonitor;

// D-Bus surface used by the GNOME Shell extension (and tests) to push
// clipboard contents into kobiQ when Wayland data-control is unavailable.
class ClipboardIngestDBus : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kobiq.Clipboard")

public:
  explicit ClipboardIngestDBus(ClipboardMonitor *monitor, QObject *parent = nullptr);

public slots:
  // contentType: "text" or "image". For text, content is UTF-8 text.
  // For image, content may be empty (monitor will try to fetch image/png).
  void NewEntry(const QString &contentType, const QString &content);

private:
  ClipboardMonitor *m_monitor;
};
