#pragma once

#include "services/AppRegistry.h"
#include "services/MimeAssociationService.h"
#include "services/MimeDefaultsStore.h"

#include <QHash>
#include <QMainWindow>
#include <QString>
#include <QVector>

class DetailsPane;
class MimeTypeModel;
class MimeTypeFilterProxy;
class QComboBox;
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
  void loadPalette();
  void loadAppearanceSettings();
  void saveAppearanceSettings() const;
  void applyTheme();
  void populateThemePicker();
  void populateAccentPicker();
  QString settingsFilePath() const;
  void loadData(const QString &preserveMime = QString());
  void selectMime(const QString &mime);
  void selectFirstEntry();

  struct ThemeColor {
    QString id;
    QString name;
    QString hex;
    bool accent = false;
    int order = 0;
  };

  struct ThemeData {
    QString id;
    QString name;
    bool dark = false;
    int order = 0;
    QHash<QString, ThemeColor> colors;
    QVector<ThemeColor> accents;
  };

  const ThemeData *currentTheme() const;
  const ThemeColor *currentAccent(const ThemeData &theme) const;
  QString colorFor(const ThemeData &theme, const QString &id, const QString &fallback) const;

  AppRegistry m_registry;
  MimeDefaultsStore m_store;
  MimeAssociationService m_service;

  MimeTypeModel *m_model;
  MimeTypeFilterProxy *m_proxy;
  QLineEdit *m_search;
  QTreeView *m_table;
  DetailsPane *m_details;
  QComboBox *m_themePicker;
  QComboBox *m_accentPicker;
  QHash<QString, ThemeData> m_themes;
  QVector<QString> m_themeOrder;
  QString m_currentThemeId;
  QString m_currentAccentId;
  bool m_updatingAppearance = false;
};
