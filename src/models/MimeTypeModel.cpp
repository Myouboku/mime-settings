#include "models/MimeTypeModel.h"

#include "services/AppRegistry.h"

MimeTypeModel::MimeTypeModel(AppRegistry *registry, QObject *parent)
    : QAbstractTableModel(parent), m_registry(registry) {}

int MimeTypeModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return m_entries.size();
}

int MimeTypeModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return ColumnCount;
}

QVariant MimeTypeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
    return QVariant();
  }

  const MimeEntry &entry = m_entries.at(index.row());
  if (role == Qt::DisplayRole) {
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

  return QVariant();
}

QVariant MimeTypeModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
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
  m_entries = entries;
  endResetModel();
}

MimeEntry MimeTypeModel::entryAt(int row) const {
  if (row < 0 || row >= m_entries.size()) {
    return MimeEntry{};
  }

  return m_entries.at(row);
}

int MimeTypeModel::rowForMime(const QString &mime) const {
  for (int i = 0; i < m_entries.size(); ++i) {
    if (m_entries[i].mimeType == mime) {
      return i;
    }
  }

  return -1;
}
