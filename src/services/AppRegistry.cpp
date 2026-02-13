#include "services/AppRegistry.h"

#include "utils/XdgPaths.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>

namespace {
QStringList parseMimeTypesFromDesktopFile(const QString &filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return {};
  }

  QTextStream in(&file);
  bool inDesktopEntry = false;
  while (!in.atEnd()) {
    const QString line = in.readLine();
    const QString trimmed = line.trimmed();

    if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')) {
      continue;
    }

    if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
      const QString section = trimmed.mid(1, trimmed.size() - 2).trimmed();
      inDesktopEntry = (section.compare("Desktop Entry", Qt::CaseInsensitive) == 0);
      continue;
    }

    if (!inDesktopEntry) {
      continue;
    }

    const int eq = trimmed.indexOf('=');
    if (eq <= 0) {
      continue;
    }

    const QString key = trimmed.left(eq).trimmed();
    if (key.compare("MimeType", Qt::CaseInsensitive) != 0) {
      continue;
    }

    const QString value = trimmed.mid(eq + 1).trimmed();
    const QStringList items = value.split(';', Qt::SkipEmptyParts);
    QStringList cleaned;

    for (const QString &item : items) {
      const QString part = item.trimmed();
      if (!part.isEmpty()) {
        cleaned.append(part);
      }
    }

    return cleaned;
  }

  return {};
}
} // namespace

void AppRegistry::load() {
  m_apps.clear();
  m_mimeToApps.clear();

  const QStringList appDirs = XdgPaths::appDirs();
  for (const QString &dir : appDirs) {
    QDirIterator it(dir, QStringList() << "*.desktop", QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
      const QString filePath = it.next();
      const QString desktopId = desktopIdForFile(filePath, dir);

      if (desktopId.isEmpty() || m_apps.contains(desktopId)) {
        continue;
      }

      indexDesktopFile(filePath, dir);
    }
  }
}

const AppInfo *AppRegistry::findById(const QString &id) const {
  auto it = m_apps.find(id);

  if (it == m_apps.end()) {
    return nullptr;
  }

  return &it.value();
}

QString AppRegistry::appDisplayName(const QString &id) const {
  const AppInfo *app = findById(id);

  if (!app) {
    return id.isEmpty() ? QString() : id;
  }

  return app->name.isEmpty() ? app->desktopId : app->name;
}

QStringList AppRegistry::appsForMime(const QString &mime) const {
  return m_mimeToApps.value(mime);
}

QList<AppInfo> AppRegistry::allApps() const {
  return m_apps.values();
}

void AppRegistry::indexDesktopFile(const QString &filePath, const QString &baseDir) {
  const QString desktopId = desktopIdForFile(filePath, baseDir);

  if (desktopId.isEmpty() || m_apps.contains(desktopId)) {
    return;
  }

  QSettings settings(filePath, QSettings::IniFormat);
  settings.beginGroup("Desktop Entry");

  const QString type = settings.value("Type").toString().trimmed();
  if (!type.isEmpty() && type.compare("Application", Qt::CaseInsensitive) != 0) {
    return;
  }

  const bool noDisplay = settings.value("NoDisplay", false).toBool();
  const bool hidden = settings.value("Hidden", false).toBool();
  if (noDisplay || hidden) {
    return;
  }

  AppInfo app;
  app.desktopId = desktopId;
  app.desktopPath = filePath;
  app.name = settings.value("Name").toString().trimmed();
  app.exec = settings.value("Exec").toString().trimmed();
  app.iconName = settings.value("Icon").toString().trimmed();

  app.mimeTypes = parseMimeTypesFromDesktopFile(filePath);
  if (app.mimeTypes.isEmpty()) {
    const QString mimeValue = settings.value("MimeType").toString();
    const QStringList mimeParts = mimeValue.split(';', Qt::SkipEmptyParts);
    for (const QString &part : mimeParts) {
      const QString trimmed = part.trimmed();

      if (!trimmed.isEmpty()) {
        app.mimeTypes.append(trimmed);
      }
    }
  }

  if (app.name.isEmpty()) {
    app.name = desktopId;
  }

  m_apps.insert(desktopId, app);

  for (const QString &mime : app.mimeTypes) {
    QStringList &list = m_mimeToApps[mime];
    if (!list.contains(desktopId)) {
      list.append(desktopId);
    }
  }
}

QString AppRegistry::desktopIdForFile(const QString &filePath, const QString &baseDir) const {
  QDir dir(baseDir);
  QString relative = dir.relativeFilePath(filePath);

  if (relative.isEmpty()) {
    return QString();
  }

  relative.replace('\\', '/');
  relative.replace('/', '-');
  return relative;
}
