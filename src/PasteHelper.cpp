#include "PasteHelper.h"

#include "ClipboardMonitor.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QProcess>
#include <QTimer>

PasteHelper::PasteHelper(ClipboardMonitor *monitor, QObject *parent)
  : QObject(parent)
  , m_monitor(monitor)
{
}

void PasteHelper::copyToClipboard(const ClipItem &item)
{
  putOnClipboard(item);
}

void PasteHelper::putOnClipboard(const ClipItem &item)
{
  auto *clipboard = QGuiApplication::clipboard();
  if (m_monitor)
    m_monitor->setIgnoreNext(true);

  if (item.kind == ClipKind::Image) {
    auto *mime = new QMimeData();
    QImage image;
    image.loadFromData(item.imagePng, "PNG");
    mime->setImageData(image);
    // Ownership of mime transfers to the clipboard for Clipboard mode.
    clipboard->setMimeData(mime, QClipboard::Clipboard);
    // Also put on Primary Selection (middle-click paste) when supported.
    if (clipboard->supportsSelection()) {
      auto *selectionMime = new QMimeData();
      selectionMime->setImageData(image);
      clipboard->setMimeData(selectionMime, QClipboard::Selection);
    }
    return;
  }

  clipboard->setText(item.text, QClipboard::Clipboard);
  // Also put on Primary Selection (middle-click paste) when supported.
  if (clipboard->supportsSelection())
    clipboard->setText(item.text, QClipboard::Selection);
}

void PasteHelper::simulatePaste()
{
  // On Wayland, synthetic key injection requires the Remote Desktop portal
  // ("Allow Remote Interaction") — skip it; clipboard content is already set.
  if (QGuiApplication::platformName().contains(QStringLiteral("wayland"),
                                               Qt::CaseInsensitive))
    return;

  // Best-effort auto-paste. Works well on X11 with xdotool.
  if (QProcess::execute(QStringLiteral("xdotool"),
                        {QStringLiteral("key"), QStringLiteral("--clearmodifiers"),
                         QStringLiteral("ctrl+v")})
      == 0)
    return;

  QProcess::execute(QStringLiteral("ydotool"),
                    {QStringLiteral("key"), QStringLiteral("29:1"),
                     QStringLiteral("47:1"), QStringLiteral("47:0"),
                     QStringLiteral("29:0")});
}

void PasteHelper::pasteItem(const ClipItem &item)
{
  copyToClipboard(item);
  QTimer::singleShot(80, this, [this]() { simulatePaste(); });
}
