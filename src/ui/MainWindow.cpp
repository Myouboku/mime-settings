#include "ui/MainWindow.h"

#include "models/MimeTypeFilterProxy.h"
#include "models/MimeTypeModel.h"
#include "ui/DetailsPane.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QSplitter>
#include <QStatusBar>
#include <QTreeView>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_service(&m_registry, &m_store) {
  m_registry.load();
  m_store.reload();
  buildUi();
  loadData();
}

void MainWindow::buildUi() {
  setWindowTitle("MIME Settings");
  resize(1100, 720);

  auto *splitter = new QSplitter(this);

  auto *leftPane = new QWidget(splitter);
  auto *leftLayout = new QVBoxLayout(leftPane);
  leftLayout->setContentsMargins(16, 16, 8, 16);
  leftLayout->setSpacing(8);

  m_search = new QLineEdit(leftPane);
  m_search->setPlaceholderText("Search MIME types or applications");

  m_table = new QTreeView(leftPane);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setAlternatingRowColors(true);
  m_table->setSortingEnabled(true);
  m_table->header()->setStretchLastSection(false);
  m_table->header()->setSectionResizeMode(QHeaderView::Interactive);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_table->setRootIsDecorated(true);
  m_table->setItemsExpandable(true);
  m_table->setExpandsOnDoubleClick(true);

  leftLayout->addWidget(m_search);
  leftLayout->addWidget(m_table, 1);

  m_details = new DetailsPane(&m_registry, splitter);

  splitter->addWidget(leftPane);
  splitter->addWidget(m_details);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);

  setCentralWidget(splitter);
  setStatusBar(new QStatusBar(this));

  m_model = new MimeTypeModel(&m_registry, this);
  m_proxy = new MimeTypeFilterProxy(this);
  m_proxy->setSourceModel(m_model);
  m_proxy->setDynamicSortFilter(true);
  m_proxy->sort(MimeTypeModel::MimeColumn, Qt::AscendingOrder);
  m_table->setModel(m_proxy);

  m_table->header()->setSectionResizeMode(
      MimeTypeModel::MimeColumn, QHeaderView::Interactive);
  m_table->header()->setSectionResizeMode(
      MimeTypeModel::DefaultAppColumn, QHeaderView::Interactive);
  m_table->header()->setSectionResizeMode(
      MimeTypeModel::DescriptionColumn, QHeaderView::Stretch);
  m_table->setColumnWidth(MimeTypeModel::MimeColumn, 240);
  m_table->setColumnWidth(MimeTypeModel::DefaultAppColumn, 220);

  connect(m_search, &QLineEdit::textChanged, this,
          [this](const QString &text) {
            m_proxy->setFilterText(text);
            if (!text.trimmed().isEmpty()) {
              m_table->expandAll();
            }
          });
  connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &MainWindow::onSelectionChanged);
  connect(m_details, &DetailsPane::requestSetDefault, this,
          &MainWindow::onRequestSetDefault);

  setStyleSheet(
      "QMainWindow { background: #f2f3f5; }"
      "DetailsPane { background: #f8f9fb; border: 1px solid #d6dbe0; "
      "border-radius: 10px; }"
      "QLineEdit { background: #ffffff; border: 1px solid #cfd4da; "
      "border-radius: 6px; "
      "padding: 6px 10px; }"
      "QTreeView { background: #ffffff; border: 1px solid #d6dbe0; "
      "border-radius: 8px; "
      "gridline-color: #e7eaee; }"
      "QHeaderView::section { background: #f6f7f9; padding: 6px; border: none; "
      "border-bottom: 1px solid #d6dbe0; }"
      "QListWidget { background: #ffffff; border: 1px solid #d6dbe0; "
      "border-radius: 8px; }"
      "QListWidget::item:selected { background: #e8efff; color: #1c2a45; }"
      "QPushButton { background: #2f6fed; color: white; border: none; "
      "border-radius: 6px; "
      "padding: 8px 14px; }"
      "QPushButton:disabled { background: #a5b8e6; }"
      "QSplitter::handle { background: #e2e6ea; }"
      "QLabel { color: #2d3035; }");
}

void MainWindow::loadData(const QString &preserveMime) {
  const QVector<MimeEntry> entries = m_service.buildEntries();
  m_model->setEntries(entries);

  if (!preserveMime.isEmpty()) {
    selectMime(preserveMime);
  } else if (m_proxy->rowCount() > 0) {
    selectFirstEntry();
  } else {
    m_details->setEntry(MimeEntry{});
  }
}

void MainWindow::selectMime(const QString &mime) {
  const QModelIndex sourceIndex = m_model->indexForMime(mime);
  const QModelIndex proxyIndex = m_proxy->mapFromSource(sourceIndex);

  if (proxyIndex.isValid()) {
    if (proxyIndex.parent().isValid()) {
      m_table->expand(proxyIndex.parent());
    }
    m_table->setCurrentIndex(proxyIndex);
    m_table->selectionModel()->select(
        proxyIndex,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    m_table->scrollTo(proxyIndex);
  }
}

void MainWindow::selectFirstEntry() {
  for (int i = 0; i < m_proxy->rowCount(); ++i) {
    const QModelIndex categoryIndex = m_proxy->index(i, 0);
    if (!categoryIndex.isValid()) {
      continue;
    }

    const int childCount = m_proxy->rowCount(categoryIndex);
    if (childCount == 0) {
      continue;
    }

    m_table->expand(categoryIndex);
    const QModelIndex firstChild = m_proxy->index(0, 0, categoryIndex);
    if (firstChild.isValid()) {
      m_table->setCurrentIndex(firstChild);
      m_table->selectionModel()->select(
          firstChild,
          QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
      m_table->scrollTo(firstChild);
      return;
    }
  }

  m_details->setEntry(MimeEntry{});
}

void MainWindow::onSelectionChanged() {
  const QModelIndexList selection = m_table->selectionModel()->selectedRows();

  if (selection.isEmpty()) {
    m_details->setEntry(MimeEntry{});
    return;
  }

  const QModelIndex proxyIndex = selection.first();
  const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
  const MimeEntry entry = m_model->entryForIndex(sourceIndex);
  m_details->setEntry(entry);
}

void MainWindow::onRequestSetDefault(const QString &mime,
                                     const QString &desktopId) {
  m_service.setDefault(mime, desktopId);
  statusBar()->showMessage(QString("Default updated for %1").arg(mime), 3000);
  loadData(mime);
}
