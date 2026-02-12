#include "utils/XdgPaths.h"

#include <QDir>
#include <QFileInfo>

QString XdgPaths::configHome() {
  QString value = qEnvironmentVariable("XDG_CONFIG_HOME");

  if (value.isEmpty()) {
    return QDir::homePath() + "/.config";
  }

  return expandHome(value);
}

QStringList XdgPaths::configDirs() {
  QString value = qEnvironmentVariable("XDG_CONFIG_DIRS");

  if (value.isEmpty()) {
    value = "/etc/xdg";
  }

  QStringList dirs = splitPaths(value);
  QStringList result;

  for (const QString &dir : dirs) {
    QString path = expandHome(dir);

    if (!path.isEmpty() && QDir(path).exists()) {
      result.append(path);
    }
  }
  return result;
}

QString XdgPaths::dataHome() {
  QString value = qEnvironmentVariable("XDG_DATA_HOME");

  if (value.isEmpty()) {
    return QDir::homePath() + "/.local/share";
  }

  return expandHome(value);
}

QStringList XdgPaths::dataDirs() {
  QString value = qEnvironmentVariable("XDG_DATA_DIRS");

  if (value.isEmpty()) {
    value = "/usr/local/share:/usr/share";
  }

  QStringList dirs = splitPaths(value);
  QStringList result;

  for (const QString &dir : dirs) {
    QString path = expandHome(dir);

    if (!path.isEmpty() && QDir(path).exists()) {
      result.append(path);
    }
  }
  return result;
}

QStringList XdgPaths::appDirs() {
  QStringList result;
  QString userApps = dataHome() + "/applications";

  if (QDir(userApps).exists()) {
    result.append(userApps);
  }

  const QStringList data = dataDirs();
  for (const QString &dir : data) {
    QString path = dir + "/applications";

    if (QDir(path).exists()) {
      result.append(path);
    }
  }

  result.removeDuplicates();
  return result;
}

QString XdgPaths::expandHome(const QString &path) {
  if (path.startsWith("~")) {
    return QDir::homePath() + path.mid(1);
  }
  return path;
}

QStringList XdgPaths::splitPaths(const QString &value) {
  QStringList parts = value.split(":", Qt::SkipEmptyParts);
  QStringList result;

  for (const QString &part : parts) {
    QString trimmed = part.trimmed();

    if (!trimmed.isEmpty()) {
      result.append(trimmed);
    }
  }
  return result;
}
