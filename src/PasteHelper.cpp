#include "PasteHelper.h"

#include "ClipboardMonitor.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

PasteHelper::PasteHelper(ClipboardMonitor *monitor, QObject *parent)
  : QObject(parent)
  , m_monitor(monitor)
{
}

bool PasteHelper::isWayland()
{
  return QGuiApplication::platformName().contains(QStringLiteral("wayland"),
                                                  Qt::CaseInsensitive);
}

void PasteHelper::copyToClipboard(const ClipItem &item)
{
  putOnClipboard(item);
}

static QMimeData *mimeForItem(const ClipItem &item)
{
  auto *mime = new QMimeData();
  if (item.kind == ClipKind::Image) {
    QImage image;
    image.loadFromData(item.imagePng, "PNG");
    if (!image.isNull())
      mime->setImageData(image);
    mime->setData(QStringLiteral("image/png"), item.imagePng);
    return mime;
  }

  mime->setText(item.text);
  mime->setData(QStringLiteral("text/plain"), item.text.toUtf8());
  return mime;
}

void PasteHelper::putOnClipboardQt(const ClipItem &item)
{
  auto *clipboard = QGuiApplication::clipboard();
  clipboard->setMimeData(mimeForItem(item), QClipboard::Clipboard);
  if (clipboard->supportsSelection())
    clipboard->setMimeData(mimeForItem(item), QClipboard::Selection);
}

bool PasteHelper::runWlCopy(const QByteArray &bytes, const QString &mimeType, bool primary)
{
  const QString bin = QStandardPaths::findExecutable(QStringLiteral("wl-copy"));
  if (bin.isEmpty())
    return false;

  QStringList args;
  if (primary)
    args << QStringLiteral("--primary");
  args << QStringLiteral("--type") << mimeType;

  QProcess proc;
  proc.start(bin, args, QIODevice::WriteOnly);
  if (!proc.waitForStarted(500))
    return false;

  if (!bytes.isEmpty()) {
    qint64 offset = 0;
    while (offset < bytes.size()) {
      const qint64 n = proc.write(bytes.constData() + offset, bytes.size() - offset);
      if (n <= 0)
        break;
      offset += n;
      proc.waitForBytesWritten(500);
    }
    if (offset != bytes.size()) {
      proc.kill();
      proc.waitForFinished(200);
      return false;
    }
  }

  proc.closeWriteChannel();
  if (!proc.waitForFinished(3000)) {
    proc.kill();
    proc.waitForFinished(200);
    return false;
  }
  return proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0;
}

bool PasteHelper::offerWithWlCopy(const ClipItem &item)
{
  QByteArray bytes;
  QString mimeType;
  if (item.kind == ClipKind::Image) {
    bytes = item.imagePng;
    mimeType = QStringLiteral("image/png");
  } else {
    bytes = item.text.toUtf8();
    mimeType = QStringLiteral("text/plain;charset=utf-8");
  }

  const bool clipboard = runWlCopy(bytes, mimeType, false);
  const bool primary = runWlCopy(bytes, mimeType, true);
  return clipboard || primary;
}

void PasteHelper::putOnClipboard(const ClipItem &item)
{
  if (m_monitor)
    m_monitor->ignoreChangesFor(1000);

  // Set Qt's clipboard while we still have keyboard focus. On Wayland this
  // offer is often dropped when the window is hidden, so also hand the bytes
  // to wl-copy, which keeps serving paste requests after we hide.
  putOnClipboardQt(item);
  QGuiApplication::sync();

  if (isWayland())
    offerWithWlCopy(item);
}

void PasteHelper::simulatePaste()
{
  // On Wayland, synthetic key injection requires the Remote Desktop portal
  // ("Allow Remote Interaction") — skip it; clipboard content is already set.
  if (isWayland())
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
