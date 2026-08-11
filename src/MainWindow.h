#pragma once

#include <QEvent>
#include <QMainWindow>
#include <QPoint>
#include <QRect>

class HistoryStore;
class PasteHelper;
class QLineEdit;
class QListWidget;
class QLabel;
class QPushButton;
class QWidget;

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  MainWindow(HistoryStore *store, PasteHelper *pasteHelper, QWidget *parent = nullptr);

public slots:
  void refresh();
  void toggleVisible();

protected:
  void showEvent(QShowEvent *event) override;
  void hideEvent(QHideEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void onFilterChanged(const QString &text);
  void pasteCurrent();
  void deleteCurrent();
  void clearHistory();
  void openSettings();
  void exportHistory();
  void importHistory();

private:
  enum ResizeEdge {
    EdgeNone = 0,
    EdgeLeft = 1,
    EdgeRight = 2,
    EdgeTop = 4,
    EdgeBottom = 8,
    EdgeTopLeft = EdgeTop | EdgeLeft,
    EdgeTopRight = EdgeTop | EdgeRight,
    EdgeBottomLeft = EdgeBottom | EdgeLeft,
    EdgeBottomRight = EdgeBottom | EdgeRight
  };

  void rebuildList();
  void updateStatus();
  void restoreGeometrySettings();
  void saveGeometrySettings();
  QString selectedId() const;
  ResizeEdge hitTestEdges(const QPoint &pos) const;
  void updateResizeCursor(ResizeEdge edges);
  void applyResize(const QPoint &globalPos);
  bool isDragHandle(QObject *watched) const;
  bool handleWindowMouse(QObject *watched, QMouseEvent *mouse, QEvent::Type type);

  HistoryStore *m_store;
  PasteHelper *m_pasteHelper;
  QWidget *m_titleBar = nullptr;
  QLabel *m_titleLabel = nullptr;
  QLineEdit *m_filter = nullptr;
  QListWidget *m_list = nullptr;
  QLabel *m_status = nullptr;
  QPushButton *m_menuBtn = nullptr;
  QString m_query;

  ResizeEdge m_resizeEdges = EdgeNone;
  bool m_resizing = false;
  bool m_dragging = false;
  QPoint m_dragOffset;
  QPoint m_resizeOrigin;
  QRect m_resizeStartGeom;
  static constexpr int kResizeMargin = 6;
};
