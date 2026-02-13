#pragma once

#include "services/MimeAssociationService.h"

#include <QString>
#include <QWidget>

class AppRegistry;
class QLabel;
class QListWidget;
class QPushButton;

class DetailsPane : public QWidget {
  Q_OBJECT

public:
  explicit DetailsPane(AppRegistry *registry, QWidget *parent = nullptr);

  void setEntry(const MimeEntry &entry);

signals:
  void requestSetDefault(const QString &mime, const QString &desktopId);

private slots:
  void updateButtonState();
  void onSetDefaultClicked();

private:
  void updateDefaultDisplay();
  void updateAssociations();

  AppRegistry *m_registry;
  MimeEntry m_entry;

  QLabel *m_title;
  QLabel *m_description;
  QLabel *m_defaultIcon;
  QLabel *m_defaultName;
  QListWidget *m_associations;
  QLabel *m_emptyHint;
  QPushButton *m_setDefault;
};
