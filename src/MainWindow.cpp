#include "MainWindow.h"

#include "HistoryStore.h"
#include "PasteHelper.h"

#include <QAction>
#include <QDateTime>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QIcon>
#include <QImage>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QShowEvent>
#include <QSize>
#include <QSizeGrip>
#include <QStandardPaths>
#include <QWindow>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(HistoryStore *store, PasteHelper *pasteHelper, QWidget *parent)
  : QMainWindow(parent)
  , m_store(store)
  , m_pasteHelper(pasteHelper)
{
  setWindowTitle(QStringLiteral("kobiQ"));
  setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
  setMinimumSize(360, 320);
  setMouseTracking(true);
  restoreGeometrySettings();

  auto *central = new QWidget(this);
  setCentralWidget(central);
  auto *layout = new QVBoxLayout(central);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);

  auto *titleBar = new QWidget(central);
  titleBar->setObjectName(QStringLiteral("titleBar"));
  titleBar->setCursor(Qt::OpenHandCursor);
  titleBar->setMouseTracking(true);
  m_titleBar = titleBar;
  auto *titleRow = new QHBoxLayout(titleBar);
  titleRow->setContentsMargins(0, 0, 0, 0);
  titleRow->setSpacing(6);

  m_menuBtn = new QPushButton(QStringLiteral("☰"), titleBar);
  m_menuBtn->setObjectName(QStringLiteral("menuButton"));
  m_menuBtn->setFixedSize(32, 32);
  m_menuBtn->setCursor(Qt::PointingHandCursor);
  m_menuBtn->setToolTip(QStringLiteral("Menu"));
  m_menuBtn->setFlat(true);

  auto *menu = new QMenu(m_menuBtn);
  auto *settingsAction = menu->addAction(QStringLiteral("Settings…"));
  menu->addSeparator();
  auto *exportAction = menu->addAction(QStringLiteral("Export history…"));
  auto *importAction = menu->addAction(QStringLiteral("Import history…"));
  m_menuBtn->setMenu(menu);

  m_titleLabel = new QLabel(QStringLiteral("kobiQ"), titleBar);
  QFont titleFont = m_titleLabel->font();
  titleFont.setPointSize(14);
  titleFont.setBold(true);
  m_titleLabel->setFont(titleFont);
  m_titleLabel->setCursor(Qt::OpenHandCursor);
  m_titleLabel->setToolTip(QStringLiteral("Drag to move window"));

  auto *closeBtn = new QPushButton(QStringLiteral("×"), titleBar);
  closeBtn->setObjectName(QStringLiteral("closeButton"));
  closeBtn->setFixedSize(32, 32);
  closeBtn->setCursor(Qt::PointingHandCursor);
  closeBtn->setToolTip(QStringLiteral("Hide to tray"));
  closeBtn->setFlat(true);

  titleRow->addWidget(m_menuBtn);
  titleRow->addWidget(m_titleLabel);
  titleRow->addStretch();
  titleRow->addWidget(closeBtn);

  m_filter = new QLineEdit(central);
  m_filter->setPlaceholderText(QStringLiteral("Search clipboard history…"));
  m_filter->setClearButtonEnabled(true);

  m_list = new QListWidget(central);
  m_list->setAlternatingRowColors(true);
  m_list->setUniformItemSizes(true);
  m_list->setIconSize(QSize(48, 48));
  m_list->setSpacing(2);

  auto *buttons = new QHBoxLayout();
  auto *pasteBtn = new QPushButton(QStringLiteral("Paste"), central);
  auto *deleteBtn = new QPushButton(QStringLiteral("Delete"), central);
  auto *clearBtn = new QPushButton(QStringLiteral("Clear"), central);
  buttons->addWidget(pasteBtn);
  buttons->addWidget(deleteBtn);
  buttons->addStretch();
  buttons->addWidget(clearBtn);

  m_status = new QLabel(central);
  m_status->setStyleSheet(QStringLiteral("color: #666;"));

  auto *footer = new QHBoxLayout();
  footer->addWidget(m_status, 1);
  auto *grip = new QSizeGrip(central);
  grip->setToolTip(QStringLiteral("Drag to resize"));
  footer->addWidget(grip, 0, Qt::AlignBottom | Qt::AlignRight);

  layout->addWidget(titleBar);
  layout->addWidget(m_filter);
  layout->addWidget(m_list, 1);
  layout->addLayout(buttons);
  layout->addLayout(footer);

  connect(closeBtn, &QPushButton::clicked, this, &MainWindow::hide);
  connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
  connect(exportAction, &QAction::triggered, this, &MainWindow::exportHistory);
  connect(importAction, &QAction::triggered, this, &MainWindow::importHistory);
  connect(m_filter, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
  connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
    pasteCurrent();
  });
  connect(pasteBtn, &QPushButton::clicked, this, &MainWindow::pasteCurrent);
  connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::deleteCurrent);
  connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearHistory);

  m_filter->installEventFilter(this);
  m_list->installEventFilter(this);
  central->installEventFilter(this);
  titleBar->installEventFilter(this);
  m_titleLabel->installEventFilter(this);
  central->setMouseTracking(true);

  setStyleSheet(QStringLiteral(
      "QMainWindow, QWidget { background: #f7f5f1; color: #1c1b19; }"
      "QLineEdit { padding: 8px; border: 1px solid #cfc8bc; border-radius: 6px; "
      "background: #fff; }"
      "QListWidget { border: 1px solid #cfc8bc; border-radius: 6px; background: #fff; }"
      "QListWidget::item { padding: 6px; min-height: 52px; }"
      "QListWidget::item:selected { background: #2f5d50; color: #fff; }"
      "QPushButton { padding: 6px 12px; border: 1px solid #cfc8bc; border-radius: 6px; "
      "background: #efeae2; }"
      "QPushButton:hover { background: #e4ddd2; }"
      "QPushButton#closeButton { padding: 0; border: none; border-radius: 6px; "
      "background: transparent; font-size: 20px; font-weight: bold; color: #5c564c; }"
      "QPushButton#closeButton:hover { background: #e8dfd4; color: #1c1b19; }"
      "QPushButton#menuButton { padding: 0; border: none; border-radius: 6px; "
      "background: transparent; font-size: 18px; color: #5c564c; }"
      "QPushButton#menuButton:hover { background: #e8dfd4; color: #1c1b19; }"
      "QPushButton#menuButton::menu-indicator { image: none; width: 0; }"
      "QWidget#titleBar { background: transparent; }"));

  refresh();
}

void MainWindow::refresh()
{
  rebuildList();
}

void MainWindow::toggleVisible()
{
  if (isVisible()) {
    hide();
    return;
  }

  show();
  raise();
  activateWindow();
  m_filter->setFocus();
  m_filter->selectAll();
}

void MainWindow::showEvent(QShowEvent *event)
{
  QMainWindow::showEvent(event);
  refresh();
  m_filter->setFocus();
}

void MainWindow::hideEvent(QHideEvent *event)
{
  saveGeometrySettings();
  QMainWindow::hideEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape) {
    hide();
    return;
  }
  QMainWindow::keyPressEvent(event);
}

MainWindow::ResizeEdge MainWindow::hitTestEdges(const QPoint &pos) const
{
  const int x = pos.x();
  const int y = pos.y();
  const int w = width();
  const int h = height();
  int edges = EdgeNone;

  if (x <= kResizeMargin)
    edges |= EdgeLeft;
  else if (x >= w - kResizeMargin)
    edges |= EdgeRight;
  if (y <= kResizeMargin)
    edges |= EdgeTop;
  else if (y >= h - kResizeMargin)
    edges |= EdgeBottom;

  return static_cast<ResizeEdge>(edges);
}

void MainWindow::updateResizeCursor(ResizeEdge edges)
{
  switch (edges) {
  case EdgeLeft:
  case EdgeRight:
    setCursor(Qt::SizeHorCursor);
    break;
  case EdgeTop:
  case EdgeBottom:
    setCursor(Qt::SizeVerCursor);
    break;
  case EdgeTopLeft:
  case EdgeBottomRight:
    setCursor(Qt::SizeFDiagCursor);
    break;
  case EdgeTopRight:
  case EdgeBottomLeft:
    setCursor(Qt::SizeBDiagCursor);
    break;
  default:
    unsetCursor();
    break;
  }
}

void MainWindow::applyResize(const QPoint &globalPos)
{
  const QPoint delta = globalPos - m_resizeOrigin;
  QRect geo = m_resizeStartGeom;

  if (m_resizeEdges & EdgeLeft)
    geo.setLeft(m_resizeStartGeom.left() + delta.x());
  if (m_resizeEdges & EdgeRight)
    geo.setRight(m_resizeStartGeom.right() + delta.x());
  if (m_resizeEdges & EdgeTop)
    geo.setTop(m_resizeStartGeom.top() + delta.y());
  if (m_resizeEdges & EdgeBottom)
    geo.setBottom(m_resizeStartGeom.bottom() + delta.y());

  if (geo.width() < minimumWidth()) {
    if (m_resizeEdges & EdgeLeft)
      geo.setLeft(geo.right() - minimumWidth() + 1);
    else
      geo.setWidth(minimumWidth());
  }
  if (geo.height() < minimumHeight()) {
    if (m_resizeEdges & EdgeTop)
      geo.setTop(geo.bottom() - minimumHeight() + 1);
    else
      geo.setHeight(minimumHeight());
  }

  setGeometry(geo);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
  if (handleWindowMouse(this, event, event->type())) {
    event->accept();
    return;
  }
  QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
  if (handleWindowMouse(this, event, event->type())) {
    event->accept();
    return;
  }
  QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
  if (handleWindowMouse(this, event, event->type())) {
    event->accept();
    return;
  }
  QMainWindow::mouseReleaseEvent(event);
}

bool MainWindow::isDragHandle(QObject *watched) const
{
  return watched == m_titleBar || watched == m_titleLabel;
}

bool MainWindow::handleWindowMouse(QObject *watched, QMouseEvent *mouse,
                                   QEvent::Type type)
{
  QWidget *widget = qobject_cast<QWidget *>(watched);
  if (!widget)
    return false;

  const QPoint windowPos = (watched == this) ? mouse->pos()
                                             : widget->mapTo(this, mouse->pos());

  if (type == QEvent::MouseButtonPress && mouse->button() == Qt::LeftButton) {
    m_resizeEdges = hitTestEdges(windowPos);
    if (m_resizeEdges != EdgeNone) {
      // Wayland: compositor-driven resize. X11: fallback to manual resize.
      if (windowHandle()) {
        Qt::Edges edges;
        if (m_resizeEdges & EdgeLeft)
          edges |= Qt::LeftEdge;
        if (m_resizeEdges & EdgeRight)
          edges |= Qt::RightEdge;
        if (m_resizeEdges & EdgeTop)
          edges |= Qt::TopEdge;
        if (m_resizeEdges & EdgeBottom)
          edges |= Qt::BottomEdge;
        if (windowHandle()->startSystemResize(edges))
          return true;
      }
      m_resizing = true;
      m_dragging = false;
      m_resizeOrigin = mouse->globalPosition().toPoint();
      m_resizeStartGeom = geometry();
      return true;
    }

    if (isDragHandle(watched)) {
      // Wayland ignores QWidget::move(); ask the compositor to move us.
      if (windowHandle() && windowHandle()->startSystemMove())
        return true;

      m_dragging = true;
      m_resizing = false;
      m_dragOffset =
          mouse->globalPosition().toPoint() - frameGeometry().topLeft();
      setCursor(Qt::ClosedHandCursor);
      return true;
    }
    return false;
  }

  if (type == QEvent::MouseMove) {
    if (m_resizing) {
      applyResize(mouse->globalPosition().toPoint());
      return true;
    }
    if (m_dragging) {
      move(mouse->globalPosition().toPoint() - m_dragOffset);
      return true;
    }

    const ResizeEdge edges = hitTestEdges(windowPos);
    if (edges != EdgeNone) {
      updateResizeCursor(edges);
      return watched == this || watched == centralWidget() || isDragHandle(watched);
    }
    if (isDragHandle(watched)) {
      setCursor(Qt::OpenHandCursor);
      return false;
    }
    if (watched == this || watched == centralWidget())
      unsetCursor();
    return false;
  }

  if (type == QEvent::MouseButtonRelease && mouse->button() == Qt::LeftButton) {
    if (m_resizing || m_dragging) {
      m_resizing = false;
      m_dragging = false;
      m_resizeEdges = EdgeNone;
      if (isDragHandle(watched))
        setCursor(Qt::OpenHandCursor);
      else
        unsetCursor();
      saveGeometrySettings();
      return true;
    }
  }

  return false;
}

void MainWindow::restoreGeometrySettings()
{
  QSettings settings;
  const QSize size =
      settings.value(QStringLiteral("windowSize"), QSize(460, 520)).toSize();
  resize(size.expandedTo(minimumSize()));

  if (settings.contains(QStringLiteral("windowPos"))) {
    const QPoint pos = settings.value(QStringLiteral("windowPos")).toPoint();
    move(pos);
  } else if (auto *screen = QGuiApplication::primaryScreen()) {
    const QRect geo = screen->availableGeometry();
    move(geo.center().x() - width() / 2, geo.center().y() - height() / 2);
  }
}

void MainWindow::saveGeometrySettings()
{
  QSettings settings;
  settings.setValue(QStringLiteral("windowSize"), size());
  settings.setValue(QStringLiteral("windowPos"), pos());
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress
      || event->type() == QEvent::MouseButtonRelease) {
    auto *mouse = static_cast<QMouseEvent *>(event);
    if (handleWindowMouse(watched, mouse, event->type()))
      return true;
  }

  if (event->type() == QEvent::KeyPress) {
    auto *key = static_cast<QKeyEvent *>(event);
    if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
      pasteCurrent();
      return true;
    }
    if (key->key() == Qt::Key_Escape) {
      hide();
      return true;
    }
    if (watched == m_filter
        && (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up)) {
      m_list->setFocus();
      if (m_list->count() > 0 && m_list->currentRow() < 0)
        m_list->setCurrentRow(0);
    }
    if (key->key() == Qt::Key_Delete && watched == m_list) {
      deleteCurrent();
      return true;
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onFilterChanged(const QString &text)
{
  m_query = text;
  rebuildList();
}

void MainWindow::rebuildList()
{
  const auto items = m_store->filtered(m_query);
  m_list->clear();
  for (const auto &item : items) {
    auto *row = new QListWidgetItem(item.preview, m_list);
    row->setData(Qt::UserRole, item.id);
    row->setSizeHint(QSize(0, 56));

    if (item.kind == ClipKind::Image) {
      QImage image;
      if (image.loadFromData(item.imagePng, "PNG") && !image.isNull()) {
        const QPixmap thumb =
            QPixmap::fromImage(image).scaled(48, 48, Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        row->setIcon(QIcon(thumb));
      }
      row->setToolTip(item.preview);
    } else {
      row->setToolTip(item.text);
    }
  }
  if (m_list->count() > 0)
    m_list->setCurrentRow(0);
  updateStatus();
}

void MainWindow::updateStatus()
{
  m_status->setText(QStringLiteral("%1 / %2 item(s)")
                        .arg(m_store->filtered(m_query).size())
                        .arg(m_store->maxItems()));
}

void MainWindow::openSettings()
{
  bool ok = false;
  const int value = QInputDialog::getInt(
      this, QStringLiteral("Settings"),
      QStringLiteral("Maximum clipboard history items:"), m_store->maxItems(), 1,
      100000, 1, &ok);
  if (!ok)
    return;

  m_store->setMaxItems(value);
  QSettings settings;
  settings.setValue(QStringLiteral("maxItems"), value);
  rebuildList();
}

void MainWindow::exportHistory()
{
  const QString defaultName =
      QStringLiteral("kobiQ-history-%1.json")
          .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
  const QString startDir =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  const QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("Export history"), startDir + QLatin1Char('/') + defaultName,
      QStringLiteral("kobiQ history (*.json);;All files (*)"));
  if (path.isEmpty())
    return;

  QString error;
  if (!m_store->exportToFile(path, &error)) {
    QMessageBox::warning(this, QStringLiteral("Export failed"), error);
    return;
  }
  QMessageBox::information(
      this, QStringLiteral("Export complete"),
      QStringLiteral("Exported %1 item(s) to:\n%2")
          .arg(m_store->items().size())
          .arg(path));
}

void MainWindow::importHistory()
{
  const QString startDir =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  const QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("Import history"), startDir,
      QStringLiteral("kobiQ history (*.json);;All files (*)"));
  if (path.isEmpty())
    return;

  QMessageBox box(this);
  box.setWindowTitle(QStringLiteral("Import history"));
  box.setText(QStringLiteral("How should imported items be applied?"));
  box.setInformativeText(
      QStringLiteral("Merge keeps your current history and adds new items.\n"
                     "Replace discards the current history."));
  auto *mergeBtn = box.addButton(QStringLiteral("Merge"), QMessageBox::AcceptRole);
  auto *replaceBtn = box.addButton(QStringLiteral("Replace"), QMessageBox::DestructiveRole);
  box.addButton(QMessageBox::Cancel);
  box.exec();

  if (box.clickedButton() != mergeBtn && box.clickedButton() != replaceBtn)
    return;

  const bool merge = box.clickedButton() == mergeBtn;
  QString error;
  if (!m_store->importFromFile(path, merge, &error)) {
    QMessageBox::warning(this, QStringLiteral("Import failed"), error);
    return;
  }

  rebuildList();
  QMessageBox::information(
      this, QStringLiteral("Import complete"),
      QStringLiteral("History now has %1 item(s).").arg(m_store->items().size()));
}

QString MainWindow::selectedId() const
{
  auto *item = m_list->currentItem();
  if (!item)
    return {};
  return item->data(Qt::UserRole).toString();
}

void MainWindow::pasteCurrent()
{
  const QString id = selectedId();
  if (id.isEmpty())
    return;
  const ClipItem item = m_store->findById(id);
  if (item.id.isEmpty())
    return;
  hide();
  m_pasteHelper->pasteItem(item);
}

void MainWindow::deleteCurrent()
{
  const QString id = selectedId();
  if (id.isEmpty())
    return;
  m_store->removeById(id);
  rebuildList();
}

void MainWindow::clearHistory()
{
  const auto answer = QMessageBox::question(
      this, QStringLiteral("Clear history"),
      QStringLiteral("Delete all clipboard history?"));
  if (answer != QMessageBox::Yes)
    return;
  m_store->clear();
  rebuildList();
}
