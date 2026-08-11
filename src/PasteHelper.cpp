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

void PasteHelper::putOnClipboard(const ClipItem &item)
{
  auto *clipboard = QGuiApplication::clipboard();
  auto *mime = new QMimeData();

  if (item.kind == ClipKind::Image) {
    QImage image;
    image.loadFromData(item.imagePng, "PNG");
    mime->setImageData(image);
  } else {
    mime->setText(item.text);
  }

  if (m_monitor)
    m_monitor->setIgnoreNext(true);
  clipboard->setMimeData(mime);
}

void PasteHelper::simulatePaste()
{
  // Best-effort auto-paste. Works well on X11 with xdotool.
  // On Wayland this may no-op; content is still on the clipboard.
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
  putOnClipboard(item);
  // Give the previous window a moment to regain focus after we hide.
  QTimer::singleShot(80, this, [this]() { simulatePaste(); });
}
