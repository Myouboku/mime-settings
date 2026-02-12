#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

struct AppInfo {
  QString desktopId;
  QString name;
  QString exec;
  QString iconName;
  QStringList mimeTypes;
  QString desktopPath;
};

class AppRegistry {
public:
  void load();

  const AppInfo *findById(const QString &id) const;
  QString appDisplayName(const QString &id) const;
  QStringList appsForMime(const QString &mime) const;
  QList<AppInfo> allApps() const;

private:
  void indexDesktopFile(const QString &filePath, const QString &baseDir);
  QString desktopIdForFile(const QString &filePath,
                           const QString &baseDir) const;

  QHash<QString, AppInfo> m_apps;
  QHash<QString, QStringList> m_mimeToApps;
};
