#include "services/MimeAssociationService.h"

#include "services/AppRegistry.h"
#include "services/MimeDefaultsStore.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QSet>
#include <algorithm>

namespace {
QString firstInstalledId(const QStringList &candidates,
                         const AppRegistry *registry) {
  for (const QString &id : candidates) {
    if (!id.isEmpty() && registry->findById(id)) {
      return id;
    }
  }

  return QString();
}

void addInstalled(QSet<QString> &target, const QStringList &candidates,
                  const AppRegistry *registry) {
  for (const QString &id : candidates) {
    if (!id.isEmpty() && registry->findById(id)) {
      target.insert(id);
    }
  }
}
} // namespace

MimeAssociationService::MimeAssociationService(AppRegistry *registry,
                                               MimeDefaultsStore *store)
    : m_registry(registry), m_store(store) {}

QVector<MimeEntry> MimeAssociationService::buildEntries() const {
  QMimeDatabase db;
  QList<QMimeType> types = db.allMimeTypes();
  std::sort(types.begin(), types.end(),
            [](const QMimeType &a, const QMimeType &b) {
              return QString::localeAwareCompare(a.name(), b.name()) < 0;
            });

  const QHash<QString, QStringList> userDefaults = m_store->userDefaults();
  const QHash<QString, QStringList> systemDefaults = m_store->systemDefaults();
  const QHash<QString, QStringList> userAssoc = m_store->userAssociations();
  const QHash<QString, QStringList> systemAssoc = m_store->systemAssociations();

  QVector<MimeEntry> entries;
  entries.reserve(types.size());

  for (const QMimeType &type : types) {
    MimeEntry entry;
    entry.mimeType = type.name();
    entry.description = type.comment();

    QString defaultId;
    const QStringList userList = userDefaults.value(entry.mimeType);
    if (!userList.isEmpty()) {
      defaultId = firstInstalledId(userList, m_registry);
    } else {
      const QStringList sysList = systemDefaults.value(entry.mimeType);
      if (!sysList.isEmpty()) {
        defaultId = firstInstalledId(sysList, m_registry);
      }
    }
    entry.defaultAppId = defaultId;

    QSet<QString> assoc;
    const QStringList registryApps = m_registry->appsForMime(entry.mimeType);

    for (const QString &id : registryApps) {
      assoc.insert(id);
    }

    const QStringList userExtra = userAssoc.value(entry.mimeType);
    addInstalled(assoc, userExtra, m_registry);

    const QStringList sysExtra = systemAssoc.value(entry.mimeType);
    addInstalled(assoc, sysExtra, m_registry);

    if (!defaultId.isEmpty()) {
      assoc.insert(defaultId);
    }

    QStringList assocList = assoc.values();
    std::sort(assocList.begin(), assocList.end(),
              [this](const QString &a, const QString &b) {
                const QString nameA = m_registry->appDisplayName(a);
                const QString nameB = m_registry->appDisplayName(b);
                const int cmp = QString::localeAwareCompare(nameA, nameB);
                return cmp == 0 ? a < b : cmp < 0;
              });
    entry.associatedAppIds = assocList;

    entries.append(entry);
  }

  return entries;
}

MimeEntry MimeAssociationService::entryFor(const QString &mime) const {
  const QVector<MimeEntry> entries = buildEntries();

  for (const MimeEntry &entry : entries) {
    if (entry.mimeType == mime) {
      return entry;
    }
  }

  return MimeEntry{};
}

void MimeAssociationService::setDefault(const QString &mime,
                                        const QString &desktopId) {
  m_store->setUserDefault(mime, desktopId);
}
