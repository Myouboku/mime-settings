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
class QTableView;

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

  AppRegistry m_registry;
  MimeDefaultsStore m_store;
  MimeAssociationService m_service;

  MimeTypeModel *m_model;
  MimeTypeFilterProxy *m_proxy;
  QLineEdit *m_search;
  QTableView *m_table;
  DetailsPane *m_details;
};
