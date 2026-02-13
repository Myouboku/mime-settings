#include "models/MimeTypeModel.h"

#include "services/AppRegistry.h"

#include <QFont>
#include <QStringList>

#include <algorithm>

MimeTypeModel::MimeTypeModel(AppRegistry *registry, QObject *parent)
    : QAbstractItemModel(parent), m_registry(registry) {
}

int MimeTypeModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid()) {
    return m_categories.size();
  }

  if (parent.column() != 0 || parent.internalId() != 0) {
    return 0;
  }

  const int categoryIndex = parent.row();
  if (categoryIndex < 0 || categoryIndex >= m_categories.size()) {
    return 0;
  }

  return m_categories[categoryIndex].entries.size();
}

int MimeTypeModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent);
  return ColumnCount;
}

QModelIndex MimeTypeModel::index(int row, int column, const QModelIndex &parent) const {
  if (row < 0 || column < 0 || column >= ColumnCount) {
    return QModelIndex();
  }

  if (!parent.isValid()) {
    if (row >= m_categories.size()) {
      return QModelIndex();
    }

    return createIndex(row, column, static_cast<quintptr>(0));
  }

  if (parent.column() != 0 || parent.internalId() != 0) {
    return QModelIndex();
  }

  const int categoryIndex = parent.row();
  if (categoryIndex < 0 || categoryIndex >= m_categories.size()) {
    return QModelIndex();
  }

  const auto &entries = m_categories[categoryIndex].entries;
  if (row >= entries.size()) {
    return QModelIndex();
  }

  return createIndex(row, column, static_cast<quintptr>(categoryIndex + 1));
}

QModelIndex MimeTypeModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) {
    return QModelIndex();
  }

  const quintptr internalId = child.internalId();
  if (internalId == 0) {
    return QModelIndex();
  }

  const int categoryIndex = static_cast<int>(internalId - 1);
  if (categoryIndex < 0 || categoryIndex >= m_categories.size()) {
    return QModelIndex();
  }

  return createIndex(categoryIndex, 0, static_cast<quintptr>(0));
}

QVariant MimeTypeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return QVariant();
  }

  const bool isCategory = isCategoryIndex(index);
  if (role == Qt::DisplayRole) {
    if (isCategory) {
      if (index.column() == MimeColumn) {
        const int categoryIndex = index.row();
        if (categoryIndex >= 0 && categoryIndex < m_categories.size()) {
          return m_categories[categoryIndex].name;
        }
      }
      return QVariant();
    }

    const int categoryIndex = static_cast<int>(index.internalId() - 1);
    if (categoryIndex < 0 || categoryIndex >= m_categories.size()) {
      return QVariant();
    }
    const auto &entries = m_categories[categoryIndex].entries;
    if (index.row() < 0 || index.row() >= entries.size()) {
      return QVariant();
    }
    const MimeEntry &entry = entries[index.row()];
    switch (index.column()) {
    case MimeColumn:
      return entry.mimeType;
    case DefaultAppColumn: {
      if (entry.defaultAppId.isEmpty()) {
        return QString("-");
      }

      const QString name = m_registry->appDisplayName(entry.defaultAppId);
      return name.isEmpty() ? entry.defaultAppId : name;
    }
    case DescriptionColumn:
      return entry.description.isEmpty() ? QString("-") : entry.description;
    default:
      break;
    }
  }

  if (role == Qt::FontRole && isCategory && index.column() == MimeColumn) {
    QFont font;
    font.setBold(true);
    return font;
  }

  return QVariant();
}

QVariant MimeTypeModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return QVariant();
  }
  switch (section) {
  case MimeColumn:
    return QString("MIME");
  case DefaultAppColumn:
    return QString("Default Application");
  case DescriptionColumn:
    return QString("Description");
  default:
    return QVariant();
  }
}

void MimeTypeModel::setEntries(const QVector<MimeEntry> &entries) {
  beginResetModel();
  m_categories.clear();
  m_lookup.clear();

  QHash<QString, QVector<MimeEntry>> grouped;

  for (const MimeEntry &entry : entries) {
    QString category = entry.mimeType.section('/', 0, 0);
    if (category.isEmpty()) {
      category = QString("other");
    }
    grouped[category].append(entry);
  }

  QStringList categories = grouped.keys();
  std::sort(categories.begin(), categories.end(), [](const QString &a, const QString &b) {
    return QString::localeAwareCompare(a, b) < 0;
  });

  for (const QString &category : categories) {
    CategoryNode node;
    node.name = category;
    node.entries = grouped.value(category);
    m_categories.append(node);
  }

  for (int i = 0; i < m_categories.size(); ++i) {
    const auto &entriesInCategory = m_categories[i].entries;
    for (int j = 0; j < entriesInCategory.size(); ++j) {
      m_lookup.insert(entriesInCategory[j].mimeType, QPair<int, int>(i, j));
    }
  }
  endResetModel();
}

MimeEntry MimeTypeModel::entryForIndex(const QModelIndex &index) const {
  if (!index.isValid() || isCategoryIndex(index)) {
    return MimeEntry{};
  }

  const int categoryIndex = static_cast<int>(index.internalId() - 1);
  if (categoryIndex < 0 || categoryIndex >= m_categories.size()) {
    return MimeEntry{};
  }

  const auto &entries = m_categories[categoryIndex].entries;
  if (index.row() < 0 || index.row() >= entries.size()) {
    return MimeEntry{};
  }

  return entries[index.row()];
}

QModelIndex MimeTypeModel::indexForMime(const QString &mime) const {
  const auto it = m_lookup.constFind(mime);
  if (it == m_lookup.constEnd()) {
    return QModelIndex();
  }

  const QPair<int, int> loc = it.value();
  if (loc.first < 0 || loc.first >= m_categories.size()) {
    return QModelIndex();
  }

  const auto &entries = m_categories[loc.first].entries;
  if (loc.second < 0 || loc.second >= entries.size()) {
    return QModelIndex();
  }

  return createIndex(loc.second, 0, static_cast<quintptr>(loc.first + 1));
}

bool MimeTypeModel::isCategoryIndex(const QModelIndex &index) const {
  return index.isValid() && index.internalId() == 0;
}
