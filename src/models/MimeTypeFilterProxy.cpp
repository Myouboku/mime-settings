#include "models/MimeTypeFilterProxy.h"

#include "models/MimeTypeModel.h"

MimeTypeFilterProxy::MimeTypeFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent) {
  setFilterCaseSensitivity(Qt::CaseInsensitive);
  setSortCaseSensitivity(Qt::CaseInsensitive);
}

void MimeTypeFilterProxy::setFilterText(const QString &text) {
  m_filter = text.trimmed();
  invalidateFilter();
}

bool MimeTypeFilterProxy::filterAcceptsRow(
    int sourceRow, const QModelIndex &sourceParent) const {
  if (m_filter.isEmpty()) {
    return true;
  }

  const QModelIndex mimeIndex =
      sourceModel()->index(sourceRow, MimeTypeModel::MimeColumn, sourceParent);
  const QModelIndex defaultIndex = sourceModel()->index(
      sourceRow, MimeTypeModel::DefaultAppColumn, sourceParent);
  const QModelIndex descIndex = sourceModel()->index(
      sourceRow, MimeTypeModel::DescriptionColumn, sourceParent);

  const QString mimeText = sourceModel()->data(mimeIndex).toString();
  const QString defaultText = sourceModel()->data(defaultIndex).toString();
  const QString descText = sourceModel()->data(descIndex).toString();

  const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
  return mimeText.contains(m_filter, cs) ||
         defaultText.contains(m_filter, cs) || descText.contains(m_filter, cs);
}
