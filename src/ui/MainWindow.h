#pragma once

#include <QMainWindow>
#include <QString>

#include "services/AppRegistry.h"
#include "services/MimeAssociationService.h"
#include "services/MimeDefaultsStore.h"

class DetailsPane;
class MimeTypeModel;
class MimeTypeFilterProxy;
class QLineEdit;
class QTreeView;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

private slots:
  void onSelectionChanged();
  void onRequestSetDefault(const QString &mime, const QString &desktopId);

private:
  void buildUi();
  void loadData(const QString &preserveMime = QString());
  void selectMime(const QString &mime);
  void selectFirstEntry();

  AppRegistry m_registry;
  MimeDefaultsStore m_store;
  MimeAssociationService m_service;

  MimeTypeModel *m_model;
  MimeTypeFilterProxy *m_proxy;
  QLineEdit *m_search;
  QTreeView *m_table;
  DetailsPane *m_details;
};
