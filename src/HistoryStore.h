#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QImage>
#include <QList>
#include <QString>
#include <QUuid>

enum class ClipKind { Text, Image };

struct ClipItem {
  QString id;
  ClipKind kind = ClipKind::Text;
  QString text;
  QByteArray imagePng;
  QDateTime createdAt;
  QString preview;

  bool matches(const QString &query) const
  {
    if (query.trimmed().isEmpty())
      return true;
    return preview.contains(query, Qt::CaseInsensitive)
        || text.contains(query, Qt::CaseInsensitive);
  }
};

class HistoryStore
{
public:
  explicit HistoryStore(int maxItems = 1000);

  void load();
  void save() const;
  bool exportToFile(const QString &path, QString *error = nullptr) const;
  // merge=true keeps existing items and prepends imported ones (deduped by content).
  bool importFromFile(const QString &path, bool merge, QString *error = nullptr);

  QList<ClipItem> items() const { return m_items; }
  QList<ClipItem> filtered(const QString &query) const;

  bool addText(const QString &text);
  bool addImage(const QImage &image);
  bool removeById(const QString &id);
  void clear();

  ClipItem findById(const QString &id) const;

  int maxItems() const { return m_maxItems; }
  void setMaxItems(int maxItems);

private:
  QString storePath() const;
  QStringList candidateStorePaths() const;
  QList<ClipItem> readItemsFromFile(const QString &path) const;
  QString previewForText(const QString &text) const;
  void trim();

  int m_maxItems;
  QList<ClipItem> m_items;
};
