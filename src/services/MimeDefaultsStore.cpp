#include "services/MimeDefaultsStore.h"

#include "utils/XdgPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace {
QHash<QString, QStringList> parseSectionEntries(const QString &filePath,
                                                const QString &sectionName) {
  QHash<QString, QStringList> result;
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return result;
  }

  QTextStream in(&file);
  bool inSection = false;
  while (!in.atEnd()) {
    const QString line = in.readLine();
    const QString trimmed = line.trimmed();

    if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')) {
      continue;
    }

    if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
      const QString section = trimmed.mid(1, trimmed.size() - 2).trimmed();
      inSection = (section.compare(sectionName, Qt::CaseInsensitive) == 0);
      continue;
    }

    if (!inSection) {
      continue;
    }

    const int eq = trimmed.indexOf('=');
    if (eq <= 0) {
      continue;
    }

    const QString key = trimmed.left(eq).trimmed();
    QString value = trimmed.mid(eq + 1).trimmed();
    QStringList items = value.split(';', Qt::SkipEmptyParts);
    QStringList cleaned;

    for (const QString &item : items) {
      const QString part = item.trimmed();

      if (!part.isEmpty()) {
        cleaned.append(part);
      }
    }

    if (!key.isEmpty()) {
      result.insert(key, cleaned);
    }
  }

  return result;
}

void mergeAssociations(QHash<QString, QStringList> &target,
                       const QHash<QString, QStringList> &source) {
  for (auto it = source.begin(); it != source.end(); ++it) {
    QStringList &list = target[it.key()];

    for (const QString &value : it.value()) {
      if (!list.contains(value)) {
        list.append(value);
      }
    }
  }
}
} // namespace

void MimeDefaultsStore::reload() {
  m_userDefaults.clear();
  m_systemDefaults.clear();
  m_userAssociations.clear();
  m_systemAssociations.clear();

  const QString userPath = userMimeappsPath();
  m_userDefaults = parseSectionEntries(userPath, "Default Applications");
  m_userAssociations = parseSectionEntries(userPath, "Added Associations");

  const QStringList configDirs = XdgPaths::configDirs();
  for (const QString &dir : configDirs) {
    const QString filePath = dir + "/mimeapps.list";
    const QHash<QString, QStringList> defaults =
        parseSectionEntries(filePath, "Default Applications");

    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
      if (!m_systemDefaults.contains(it.key())) {
        m_systemDefaults.insert(it.key(), it.value());
      }
    }

    const QHash<QString, QStringList> associations =
        parseSectionEntries(filePath, "Added Associations");
    mergeAssociations(m_systemAssociations, associations);
  }

  const QStringList dataDirs = XdgPaths::dataDirs();
  for (const QString &dir : dataDirs) {
    const QString filePath = dir + "/applications/mimeapps.list";
    const QHash<QString, QStringList> defaults =
        parseSectionEntries(filePath, "Default Applications");

    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
      if (!m_systemDefaults.contains(it.key())) {
        m_systemDefaults.insert(it.key(), it.value());
      }
    }

    const QHash<QString, QStringList> associations =
        parseSectionEntries(filePath, "Added Associations");
    mergeAssociations(m_systemAssociations, associations);
  }
}

QHash<QString, QStringList> MimeDefaultsStore::userDefaults() const {
  return m_userDefaults;
}

QHash<QString, QStringList> MimeDefaultsStore::systemDefaults() const {
  return m_systemDefaults;
}

QHash<QString, QStringList> MimeDefaultsStore::userAssociations() const {
  return m_userAssociations;
}

QHash<QString, QStringList> MimeDefaultsStore::systemAssociations() const {
  return m_systemAssociations;
}

void MimeDefaultsStore::setUserDefault(const QString &mime, const QString &desktopId) {
  const QString filePath = userMimeappsPath();
  QFileInfo info(filePath);
  QDir().mkpath(info.absolutePath());

  QStringList lines;
  QFile file(filePath);
  if (file.exists()) {
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream in(&file);

      while (!in.atEnd()) {
        lines.append(in.readLine());
      }

      file.close();
    }
  }

  const QString sectionHeader = "[Default Applications]";
  bool inSection = false;
  bool foundSection = false;
  bool updated = false;
  int insertIndex = -1;

  for (int i = 0; i < lines.size(); ++i) {
    const QString trimmed = lines[i].trimmed();

    if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
      if (inSection && !updated && insertIndex == -1) {
        insertIndex = i;
      }

      inSection = (trimmed.compare(sectionHeader, Qt::CaseInsensitive) == 0);

      if (inSection) {
        foundSection = true;
      }
      continue;
    }

    if (!inSection) {
      continue;
    }

    if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')) {
      continue;
    }

    const int eq = trimmed.indexOf('=');
    if (eq <= 0) {
      continue;
    }

    const QString key = trimmed.left(eq).trimmed();
    if (key != mime) {
      continue;
    }

    QString value = trimmed.mid(eq + 1).trimmed();
    QStringList items = value.split(';', Qt::SkipEmptyParts);
    QStringList cleaned;
    cleaned.append(desktopId);

    for (const QString &item : items) {
      const QString part = item.trimmed();

      if (!part.isEmpty() && part != desktopId) {
        cleaned.append(part);
      }
    }

    QString newValue = cleaned.join(';');
    if (!newValue.endsWith(';')) {
      newValue.append(';');
    }

    lines[i] = key + "=" + newValue;
    updated = true;
    break;
  }

  if (!foundSection) {
    if (!lines.isEmpty()) {
      lines.append("");
    }

    lines.append(sectionHeader);
    lines.append(mime + "=" + desktopId + ";");
  } else if (!updated) {
    if (insertIndex == -1) {
      insertIndex = lines.size();
    }

    lines.insert(insertIndex, mime + "=" + desktopId + ";");
  }

  if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    QTextStream out(&file);

    for (const QString &line : lines) {
      out << line << '\n';
    }

    file.close();
  }

  reload();
}

QString MimeDefaultsStore::userMimeappsPath() const {
  return XdgPaths::configHome() + "/mimeapps.list";
}
