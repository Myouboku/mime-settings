#pragma once

#include <QAbstractTableModel>
#include <QString>
#include <QVariant>
#include <QVector>

#include "services/MimeAssociationService.h"

class AppRegistry;

class MimeTypeModel : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    MimeColumn = 0,
    DefaultAppColumn = 1,
    DescriptionColumn = 2,
    ColumnCount = 3
  };

  explicit MimeTypeModel(AppRegistry *registry, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;

  void setEntries(const QVector<MimeEntry> &entries);
  MimeEntry entryAt(int row) const;
  int rowForMime(const QString &mime) const;

private:
  AppRegistry *m_registry;
  QVector<MimeEntry> m_entries;
};
