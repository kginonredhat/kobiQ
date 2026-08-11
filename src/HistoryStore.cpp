#include "HistoryStore.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtGlobal>

HistoryStore::HistoryStore(int maxItems)
  : m_maxItems(qMax(1, maxItems))
{
}

void HistoryStore::setMaxItems(int maxItems)
{
  m_maxItems = qMax(1, maxItems);
  const int before = m_items.size();
  trim();
  if (m_items.size() != before)
    save();
}

QString HistoryStore::storePath() const
{
  const QString dir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(dir);
  return dir + QStringLiteral("/history.json");
}

QStringList HistoryStore::candidateStorePaths() const
{
  QStringList paths;
  paths << storePath();

  // Legacy history paths from earlier project name (migration only).
  const QString home = QDir::homePath();
  paths << home + QStringLiteral("/.local/share/clipditto/clipditto/history.json");
  paths << home + QStringLiteral("/.local/share/clipditto/history.json");

  paths.removeDuplicates();
  return paths;
}

QList<ClipItem> HistoryStore::readItemsFromFile(const QString &path) const
{
  QList<ClipItem> items;
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return items;

  const auto doc = QJsonDocument::fromJson(file.readAll());
  if (!doc.isArray())
    return items;

  for (const auto &value : doc.array()) {
    const auto obj = value.toObject();
    ClipItem item;
    item.id = obj.value(QStringLiteral("id")).toString();
    item.kind = obj.value(QStringLiteral("kind")).toString() == QLatin1String("image")
                    ? ClipKind::Image
                    : ClipKind::Text;
    item.text = obj.value(QStringLiteral("text")).toString();
    item.imagePng =
        QByteArray::fromBase64(obj.value(QStringLiteral("imagePng")).toString().toUtf8());
    item.createdAt = QDateTime::fromString(
        obj.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
    item.preview = obj.value(QStringLiteral("preview")).toString();
    if (item.id.isEmpty())
      item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (item.preview.isEmpty() && item.kind == ClipKind::Text)
      item.preview = previewForText(item.text);
    items.append(item);
  }
  return items;
}

void HistoryStore::load()
{
  m_items.clear();

  QString bestPath;
  QList<ClipItem> bestItems;
  qint64 bestMtime = -1;

  for (const QString &path : candidateStorePaths()) {
    QFileInfo info(path);
    if (!info.exists() || !info.isFile())
      continue;
    const auto items = readItemsFromFile(path);
    if (items.isEmpty())
      continue;

    const qint64 mtime = info.lastModified().toSecsSinceEpoch();
    const bool betterCount = items.size() > bestItems.size();
    const bool sameCountNewer =
        items.size() == bestItems.size() && mtime >= bestMtime;
    if (bestItems.isEmpty() || betterCount || sameCountNewer) {
      bestItems = items;
      bestPath = path;
      bestMtime = mtime;
    }
  }

  m_items = bestItems;
  trim();

  // Always write into the current app path so kill/restart uses one file.
  if (!m_items.isEmpty() && bestPath != storePath())
    save();
  else if (!m_items.isEmpty() && QFileInfo(storePath()).size() == 0)
    save();
}

void HistoryStore::save() const
{
  const QString path = storePath();
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly))
    return;

  QJsonArray array;
  for (const auto &item : m_items) {
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), item.id);
    obj.insert(QStringLiteral("kind"),
               item.kind == ClipKind::Image ? QStringLiteral("image")
                                            : QStringLiteral("text"));
    obj.insert(QStringLiteral("text"), item.text);
    obj.insert(QStringLiteral("imagePng"),
               QString::fromLatin1(item.imagePng.toBase64()));
    obj.insert(QStringLiteral("createdAt"), item.createdAt.toString(Qt::ISODate));
    obj.insert(QStringLiteral("preview"), item.preview);
    array.append(obj);
  }

  file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
  file.commit(); // atomic replace
}

bool HistoryStore::exportToFile(const QString &path, QString *error) const
{
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    if (error)
      *error = QStringLiteral("Could not write file:\n%1").arg(path);
    return false;
  }

  QJsonArray array;
  for (const auto &item : m_items) {
    QJsonObject obj;
    obj.insert(QStringLiteral("id"), item.id);
    obj.insert(QStringLiteral("kind"),
               item.kind == ClipKind::Image ? QStringLiteral("image")
                                            : QStringLiteral("text"));
    obj.insert(QStringLiteral("text"), item.text);
    obj.insert(QStringLiteral("imagePng"),
               QString::fromLatin1(item.imagePng.toBase64()));
    obj.insert(QStringLiteral("createdAt"), item.createdAt.toString(Qt::ISODate));
    obj.insert(QStringLiteral("preview"), item.preview);
    array.append(obj);
  }

  const QByteArray payload = QJsonDocument(array).toJson(QJsonDocument::Indented);
  if (file.write(payload) != payload.size() || !file.commit()) {
    if (error)
      *error = QStringLiteral("Failed while writing export file.");
    return false;
  }
  return true;
}

bool HistoryStore::importFromFile(const QString &path, bool merge, QString *error)
{
  const auto imported = readItemsFromFile(path);
  if (imported.isEmpty()) {
    // Distinguish unreadable vs empty valid file.
    QFile file(path);
    if (!file.exists()) {
      if (error)
        *error = QStringLiteral("File not found:\n%1").arg(path);
      return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
      if (error)
        *error = QStringLiteral("Could not read file:\n%1").arg(path);
      return false;
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) {
      if (error)
        *error = QStringLiteral("Not a valid kobiQ history file.");
      return false;
    }
    if (error)
      *error = QStringLiteral("The file contains no clipboard items.");
    return false;
  }

  if (!merge) {
    m_items = imported;
  } else {
    QList<ClipItem> merged = imported;
    for (const auto &existing : m_items) {
      bool duplicate = false;
      for (const auto &item : merged) {
        if (existing.kind == item.kind) {
          if (existing.kind == ClipKind::Text && existing.text == item.text) {
            duplicate = true;
            break;
          }
          if (existing.kind == ClipKind::Image && existing.imagePng == item.imagePng) {
            duplicate = true;
            break;
          }
        }
      }
      if (!duplicate)
        merged.append(existing);
    }
    m_items = merged;
  }

  // Fresh ids are fine; keep imported ids unless empty (already handled in reader).
  trim();
  save();
  return true;
}

QList<ClipItem> HistoryStore::filtered(const QString &query) const
{
  QList<ClipItem> out;
  for (const auto &item : m_items) {
    if (item.matches(query))
      out.append(item);
  }
  return out;
}

QString HistoryStore::previewForText(const QString &text) const
{
  auto preview = text.simplified();
  if (preview.size() > 120)
    preview = preview.left(117) + QStringLiteral("...");
  return preview;
}

void HistoryStore::trim()
{
  while (m_items.size() > m_maxItems)
    m_items.removeLast();
}

bool HistoryStore::addText(const QString &text)
{
  if (text.isEmpty())
    return false;
  if (!m_items.isEmpty() && m_items.first().kind == ClipKind::Text
      && m_items.first().text == text)
    return false;

  ClipItem item;
  item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  item.kind = ClipKind::Text;
  item.text = text;
  item.createdAt = QDateTime::currentDateTimeUtc();
  item.preview = previewForText(text);
  m_items.prepend(item);
  trim();
  save();
  return true;
}

bool HistoryStore::addImage(const QImage &image)
{
  if (image.isNull())
    return false;

  QByteArray png;
  QBuffer buffer(&png);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");

  if (!m_items.isEmpty() && m_items.first().kind == ClipKind::Image
      && m_items.first().imagePng == png)
    return false;

  ClipItem item;
  item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
  item.kind = ClipKind::Image;
  item.imagePng = png;
  item.createdAt = QDateTime::currentDateTimeUtc();
  item.preview = QStringLiteral("[Image %1×%2]")
                     .arg(image.width())
                     .arg(image.height());
  m_items.prepend(item);
  trim();
  save();
  return true;
}

bool HistoryStore::removeById(const QString &id)
{
  for (int i = 0; i < m_items.size(); ++i) {
    if (m_items.at(i).id == id) {
      m_items.removeAt(i);
      save();
      return true;
    }
  }
  return false;
}

void HistoryStore::clear()
{
  m_items.clear();
  save();
}

ClipItem HistoryStore::findById(const QString &id) const
{
  for (const auto &item : m_items) {
    if (item.id == id)
      return item;
  }
  return {};
}
