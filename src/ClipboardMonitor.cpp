#include "ClipboardMonitor.h"

#include "HistoryStore.h"

#include <QClipboard>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QStandardPaths>
#include <QtGlobal>

ClipboardMonitor::ClipboardMonitor(HistoryStore *store, QObject *parent)
  : QObject(parent)
  , m_store(store)
  , m_clipboard(QGuiApplication::clipboard())
{
  connect(m_clipboard, &QClipboard::dataChanged, this,
          &ClipboardMonitor::onClipboardChanged);
  startWaylandFallbacks();
}

ClipboardMonitor::~ClipboardMonitor()
{
  stopWlPasteWatch();
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
  if (storeCurrentClipboard())
    emit historyChanged();
}

bool ClipboardMonitor::storeCurrentClipboard()
{
  const QMimeData *mime = m_clipboard->mimeData();
  if (!mime)
    return false;

  if (mime->hasImage()) {
    const QImage image = qvariant_cast<QImage>(mime->imageData());
    return m_store->addImage(image);
  }
  if (mime->hasText())
    return storeText(mime->text());
  return false;
}

bool ClipboardMonitor::storeText(const QString &text)
{
  if (shouldIgnore())
    return false;
  return m_store->addText(text);
}

bool ClipboardMonitor::storeImageFromPng(const QByteArray &png)
{
  if (shouldIgnore() || png.isEmpty())
    return false;
  QImage image;
  if (!image.loadFromData(png, "PNG"))
    return false;
  return m_store->addImage(image);
}

void ClipboardMonitor::ingestText(const QString &text)
{
  if (storeText(text))
    emit historyChanged();
}

void ClipboardMonitor::ingestImageNotify()
{
  // Prefer Qt clipboard if the offer is still readable; otherwise wl-paste.
  if (!shouldIgnore()) {
    const QMimeData *mime = m_clipboard->mimeData();
    if (mime && mime->hasImage()) {
      if (m_store->addImage(qvariant_cast<QImage>(mime->imageData()))) {
        emit historyChanged();
        return;
      }
    }
  }
  fetchImageViaWlPaste();
}

void ClipboardMonitor::fetchImageViaWlPaste()
{
  const QString bin = QStandardPaths::findExecutable(QStringLiteral("wl-paste"));
  if (bin.isEmpty())
    return;

  auto *proc = new QProcess(this);
  connect(proc, &QProcess::finished, this, [this, proc](int code, QProcess::ExitStatus st) {
    proc->deleteLater();
    if (st != QProcess::NormalExit || code != 0)
      return;
    if (storeImageFromPng(proc->readAllStandardOutput()))
      emit historyChanged();
  });
  proc->start(bin, {QStringLiteral("-n"), QStringLiteral("-t"), QStringLiteral("image/png")});
}

void ClipboardMonitor::startWaylandFallbacks()
{
  const bool wayland = QGuiApplication::platformName().contains(
      QStringLiteral("wayland"), Qt::CaseInsensitive);
  if (!wayland)
    return;

  // On compositors with data-control (KDE, wlroots), wl-paste --watch works.
  // On GNOME it fails immediately; we then rely on the Shell extension / GPaste.
  tryStartWlPasteWatch();
  tryConnectGPaste();
}

void ClipboardMonitor::tryStartWlPasteWatch()
{
  const QString bin = QStandardPaths::findExecutable(QStringLiteral("wl-paste"));
  if (bin.isEmpty())
    return;

  stopWlPasteWatch();
  m_wlPasteWatch = new QProcess(this);
  m_wlPasteWatch->setProcessChannelMode(QProcess::SeparateChannels);
  // Print clipboard text (or a marker) on each change; we re-read via Qt/wl-paste.
  connect(m_wlPasteWatch, &QProcess::readyReadStandardOutput, this,
          &ClipboardMonitor::onWlPasteWatchReady);
  connect(m_wlPasteWatch, &QProcess::readyReadStandardError, this, [this]() {
    if (!m_wlPasteWatch)
      return;
    const QByteArray err = m_wlPasteWatch->readAllStandardError();
    if (err.contains("data-control")) {
      qInfo("wl-paste --watch unavailable (no data-control). "
            "On GNOME, enable the kobiQ Clipboard Monitor Shell extension.");
      stopWlPasteWatch();
    }
  });
  connect(m_wlPasteWatch, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
    // If watch exits quickly, do not restart in a loop on GNOME.
    if (m_wlPasteWatch)
      m_wlPasteWatch->deleteLater();
    m_wlPasteWatch = nullptr;
  });

  // Use a tiny helper command: emit a line each time selection changes.
  m_wlPasteWatch->start(bin, {QStringLiteral("--watch"), QStringLiteral("echo"),
                              QStringLiteral("__kobiq_clip__")});
}

void ClipboardMonitor::stopWlPasteWatch()
{
  if (!m_wlPasteWatch)
    return;
  m_wlPasteWatch->disconnect(this);
  m_wlPasteWatch->kill();
  m_wlPasteWatch->waitForFinished(300);
  m_wlPasteWatch->deleteLater();
  m_wlPasteWatch = nullptr;
}

void ClipboardMonitor::onWlPasteWatchReady()
{
  if (!m_wlPasteWatch || shouldIgnore()) {
    if (m_wlPasteWatch)
      m_wlPasteWatch->readAllStandardOutput();
    return;
  }
  m_wlPasteWatch->readAllStandardOutput();
  if (storeCurrentClipboard()) {
    emit historyChanged();
    return;
  }
  // Text-only fallback through a one-shot wl-paste (works on many compositors).
  const QString bin = QStandardPaths::findExecutable(QStringLiteral("wl-paste"));
  if (bin.isEmpty())
    return;
  QProcess once;
  once.start(bin, {QStringLiteral("-n")});
  if (!once.waitForFinished(1000) || once.exitCode() != 0)
    return;
  if (storeText(QString::fromUtf8(once.readAllStandardOutput())))
    emit historyChanged();
}

void ClipboardMonitor::tryConnectGPaste()
{
  QDBusConnection bus = QDBusConnection::sessionBus();
  // GPaste's Shell extension already sees every copy on GNOME. When it is
  // active, reuse its Update signal so history works even before our own
  // extension is enabled.
  const bool ok = bus.connect(QStringLiteral("org.gnome.GPaste"),
                              QStringLiteral("/org/gnome/GPaste"),
                              QStringLiteral("org.gnome.GPaste2"),
                              QStringLiteral("Update"), this,
                              SLOT(onGPasteUpdate(QString, QString, qulonglong)));
  m_gpasteConnected = ok;
  if (ok)
    qInfo("Listening to GPaste clipboard updates as a GNOME fallback.");
}

void ClipboardMonitor::onGPasteUpdate(const QString &action, const QString &uuid,
                                      qulonglong)
{
  if (shouldIgnore())
    return;

  // Typical actions: Replace, Add, … — ignore deletes/clears.
  const QString a = action.toLower();
  if (a.contains(QLatin1String("delete")) || a.contains(QLatin1String("empty"))
      || a.contains(QLatin1String("clear")))
    return;

  QDBusInterface iface(QStringLiteral("org.gnome.GPaste"),
                       QStringLiteral("/org/gnome/GPaste"),
                       QStringLiteral("org.gnome.GPaste2"),
                       QDBusConnection::sessionBus());
  if (!iface.isValid())
    return;

  auto ingestId = [&](const QString &id) -> bool {
    if (id.isEmpty())
      return false;
    QDBusReply<QString> kind = iface.call(QStringLiteral("GetElementKind"), id);
    if (kind.isValid()) {
      const QString k = kind.value().toLower();
      if (k.contains(QLatin1String("image"))) {
        fetchImageViaWlPaste(); // emits historyChanged asynchronously
        return true;
      }
      if (!k.isEmpty() && !k.contains(QLatin1String("text")))
        return false;
    }
    QDBusReply<QString> raw = iface.call(QStringLiteral("GetRawElement"), id);
    if (!raw.isValid())
      return false;
    if (storeText(raw.value()))
      emit historyChanged();
    return true;
  };

  if (ingestId(uuid))
    return;

  QDBusMessage msg = QDBusMessage::createMethodCall(
      QStringLiteral("org.gnome.GPaste"), QStringLiteral("/org/gnome/GPaste"),
      QStringLiteral("org.gnome.GPaste2"), QStringLiteral("GetElementAtIndex"));
  msg << qulonglong(0);
  const QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
  if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() < 2)
    return;

  // Returns (s uuid, s display). Prefer raw content.
  if (ingestId(reply.arguments().at(0).toString()))
    return;

  if (storeText(reply.arguments().at(1).toString()))
    emit historyChanged();
}
