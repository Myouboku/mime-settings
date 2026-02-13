#pragma once

#include "services/MimeAssociationService.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QPair>
#include <QString>
#include <QVariant>
#include <QVector>

class AppRegistry;

class MimeTypeModel : public QAbstractItemModel {
  Q_OBJECT

public:
  enum Column { MimeColumn = 0, DefaultAppColumn = 1, DescriptionColumn = 2, ColumnCount = 3 };

  explicit MimeTypeModel(AppRegistry *registry, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  void setEntries(const QVector<MimeEntry> &entries);
  MimeEntry entryForIndex(const QModelIndex &index) const;
  QModelIndex indexForMime(const QString &mime) const;

private:
  struct CategoryNode {
    QString name;
    QVector<MimeEntry> entries;
  };

  bool isCategoryIndex(const QModelIndex &index) const;

  AppRegistry *m_registry;
  QVector<CategoryNode> m_categories;
  QHash<QString, QPair<int, int>> m_lookup;
};
