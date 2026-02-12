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
#include <QTableView>
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

  m_table = new QTableView(leftPane);
  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_table->setAlternatingRowColors(true);
  m_table->setSortingEnabled(true);
  m_table->verticalHeader()->setVisible(false);
  m_table->horizontalHeader()->setStretchLastSection(false);
  m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

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

  m_table->horizontalHeader()->setSectionResizeMode(
      MimeTypeModel::MimeColumn, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(
      MimeTypeModel::DefaultAppColumn, QHeaderView::ResizeToContents);
  m_table->horizontalHeader()->setSectionResizeMode(
      MimeTypeModel::DescriptionColumn, QHeaderView::Stretch);

  connect(m_search, &QLineEdit::textChanged, m_proxy,
          &MimeTypeFilterProxy::setFilterText);
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
      "QTableView { background: #ffffff; border: 1px solid #d6dbe0; "
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
    m_table->selectRow(0);
  } else {
    m_details->setEntry(MimeEntry{});
  }
}

void MainWindow::selectMime(const QString &mime) {
  const int sourceRow = m_model->rowForMime(mime);

  if (sourceRow < 0) {
    return;
  }

  const QModelIndex sourceIndex = m_model->index(sourceRow, 0);
  const QModelIndex proxyIndex = m_proxy->mapFromSource(sourceIndex);

  if (proxyIndex.isValid()) {
    m_table->selectRow(proxyIndex.row());
  }
}

void MainWindow::onSelectionChanged() {
  const QModelIndexList selection = m_table->selectionModel()->selectedRows();

  if (selection.isEmpty()) {
    m_details->setEntry(MimeEntry{});
    return;
  }

  const QModelIndex proxyIndex = selection.first();
  const QModelIndex sourceIndex = m_proxy->mapToSource(proxyIndex);
  const MimeEntry entry = m_model->entryAt(sourceIndex.row());
  m_details->setEntry(entry);
}

void MainWindow::onRequestSetDefault(const QString &mime,
                                     const QString &desktopId) {
  m_service.setDefault(mime, desktopId);
  statusBar()->showMessage(QString("Default updated for %1").arg(mime), 3000);
  loadData(mime);
}
