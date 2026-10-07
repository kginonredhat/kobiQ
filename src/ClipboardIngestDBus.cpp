#include "ClipboardIngestDBus.h"

#include "ClipboardMonitor.h"

ClipboardIngestDBus::ClipboardIngestDBus(ClipboardMonitor *monitor, QObject *parent)
  : QObject(parent)
  , m_monitor(monitor)
{
}

void ClipboardIngestDBus::NewEntry(const QString &contentType, const QString &content)
{
  if (!m_monitor)
    return;

  if (contentType == QLatin1String("text")) {
    m_monitor->ingestText(content);
    return;
  }

  if (contentType == QLatin1String("image")) {
    m_monitor->ingestImageNotify();
    return;
  }
}
