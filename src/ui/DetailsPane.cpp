#include "ui/DetailsPane.h"

#include "services/AppRegistry.h"

#include <QAbstractItemView>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

DetailsPane::DetailsPane(AppRegistry *registry, QWidget *parent)
    : QWidget(parent), m_registry(registry) {
  m_title = new QLabel(this);
  QFont titleFont = m_title->font();
  titleFont.setPointSize(titleFont.pointSize() + 3);
  titleFont.setBold(true);
  m_title->setFont(titleFont);

  m_description = new QLabel(this);
  m_description->setWordWrap(true);

  m_defaultIcon = new QLabel(this);
  m_defaultIcon->setFixedSize(32, 32);
  m_defaultIcon->setScaledContents(true);

  m_defaultName = new QLabel(this);
  m_defaultName->setWordWrap(true);

  m_associations = new QListWidget(this);
  m_associations->setSelectionMode(QAbstractItemView::SingleSelection);

  m_emptyHint = new QLabel("No associated applications", this);
  QFont hintFont = m_emptyHint->font();
  hintFont.setItalic(true);
  m_emptyHint->setFont(hintFont);
  m_emptyHint->setVisible(false);

  m_setDefault = new QPushButton("Set as default", this);
  m_setDefault->setEnabled(false);

  auto *defaultRow = new QHBoxLayout();
  defaultRow->addWidget(m_defaultIcon);
  defaultRow->addWidget(m_defaultName, 1);

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(m_title);
  layout->addWidget(m_description);
  layout->addSpacing(8);
  layout->addWidget(new QLabel("Default application", this));
  layout->addLayout(defaultRow);
  layout->addSpacing(12);
  layout->addWidget(new QLabel("Associated applications", this));
  layout->addWidget(m_associations, 1);
  layout->addWidget(m_emptyHint);
  layout->addSpacing(8);
  layout->addWidget(m_setDefault, 0, Qt::AlignRight);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(8);

  connect(m_associations, &QListWidget::itemSelectionChanged, this,
          &DetailsPane::updateButtonState);
  connect(m_setDefault, &QPushButton::clicked, this,
          &DetailsPane::onSetDefaultClicked);
}

void DetailsPane::setEntry(const MimeEntry &entry) {
  m_entry = entry;

  if (entry.mimeType.isEmpty()) {
    m_title->setText("Select a MIME type");
    m_description->setText("Pick a row on the left to view details.");
  } else {
    m_title->setText(entry.mimeType);
    m_description->setText(entry.description.isEmpty() ? "No description"
                                                       : entry.description);
  }

  updateDefaultDisplay();
  updateAssociations();
  updateButtonState();
}

void DetailsPane::updateDefaultDisplay() {
  if (m_entry.defaultAppId.isEmpty()) {
    m_defaultName->setText("No default application");
    m_defaultIcon->setPixmap(QPixmap());
    return;
  }

  const AppInfo *app = m_registry->findById(m_entry.defaultAppId);
  const QString name = app ? app->name : m_entry.defaultAppId;
  m_defaultName->setText(name);

  const QString iconName = app ? app->iconName : QString();
  QIcon icon = QIcon::fromTheme(iconName.isEmpty() ? "application-x-executable"
                                                   : iconName);
  m_defaultIcon->setPixmap(icon.pixmap(32, 32));
}

void DetailsPane::updateAssociations() {
  m_associations->clear();

  for (const QString &appId : m_entry.associatedAppIds) {
    const AppInfo *app = m_registry->findById(appId);
    const QString name = app ? app->name : QString("%1 (missing)").arg(appId);

    auto *item = new QListWidgetItem(name, m_associations);
    item->setData(Qt::UserRole, appId);

    const QString iconName = app ? app->iconName : QString();
    QIcon icon = QIcon::fromTheme(

        iconName.isEmpty() ? "application-x-executable" : iconName);
    item->setIcon(icon);

    if (appId == m_entry.defaultAppId) {
      QFont font = item->font();
      font.setBold(true);
      item->setFont(font);
    }
  }

  if (m_entry.associatedAppIds.isEmpty()) {
    m_emptyHint->setVisible(true);
  } else {
    m_emptyHint->setVisible(false);
  }

  if (!m_entry.defaultAppId.isEmpty()) {
    for (int i = 0; i < m_associations->count(); ++i) {
      QListWidgetItem *item = m_associations->item(i);

      if (item->data(Qt::UserRole).toString() == m_entry.defaultAppId) {
        item->setSelected(true);
        break;
      }
    }
  }
}

void DetailsPane::updateButtonState() {
  const QListWidgetItem *item = m_associations->currentItem();

  if (!item) {
    m_setDefault->setEnabled(false);
    return;
  }

  const QString selectedId = item->data(Qt::UserRole).toString();
  const bool canSet =
      !selectedId.isEmpty() && selectedId != m_entry.defaultAppId;
  m_setDefault->setEnabled(canSet);
}

void DetailsPane::onSetDefaultClicked() {
  const QListWidgetItem *item = m_associations->currentItem();

  if (!item || m_entry.mimeType.isEmpty()) {
    return;
  }

  const QString selectedId = item->data(Qt::UserRole).toString();
  if (selectedId.isEmpty()) {
    return;
  }

  emit requestSetDefault(m_entry.mimeType, selectedId);
}
