#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class MimeDefaultsStore {
public:
  void reload();

  QHash<QString, QStringList> userDefaults() const;
  QHash<QString, QStringList> systemDefaults() const;
  QHash<QString, QStringList> userAssociations() const;
  QHash<QString, QStringList> systemAssociations() const;

  void setUserDefault(const QString &mime, const QString &desktopId);
  QString userMimeappsPath() const;

private:
  QHash<QString, QStringList> m_userDefaults;
  QHash<QString, QStringList> m_systemDefaults;
  QHash<QString, QStringList> m_userAssociations;
  QHash<QString, QStringList> m_systemAssociations;
};
