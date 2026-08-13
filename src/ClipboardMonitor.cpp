#include "ClipboardMonitor.h"

#include "HistoryStore.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QtGlobal>

ClipboardMonitor::ClipboardMonitor(HistoryStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
    , m_clipboard(QGuiApplication::clipboard())
{
  connect(m_clipboard, &QClipboard::dataChanged, this,
          &ClipboardMonitor::onClipboardChanged);
}

void ClipboardMonitor::ignoreChangesFor(int milliseconds)
{
  m_ignoreForMs = qMax(0, milliseconds);
  m_ignoreTimer.restart();
}

bool ClipboardMonitor::shouldIgnore() const
{
  return m_ignoreForMs > 0 && m_ignoreTimer.isValid()
         && !m_ignoreTimer.hasExpired(m_ignoreForMs);
}

void ClipboardMonitor::onClipboardChanged()
{
  if (shouldIgnore())
    return;

  const QMimeData *mime = m_clipboard->mimeData();
  if (!mime)
    return;

  bool changed = false;
  if (mime->hasImage()) {
    const QImage image = qvariant_cast<QImage>(mime->imageData());
    changed = m_store->addImage(image);
  } else if (mime->hasText()) {
    changed = m_store->addText(mime->text());
  }

  if (changed)
    emit historyChanged();
}
