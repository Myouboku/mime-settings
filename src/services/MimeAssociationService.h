#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct MimeEntry {
  QString mimeType;
  QString description;
  QString defaultAppId;
  QStringList associatedAppIds;
};

class AppRegistry;
class MimeDefaultsStore;

class MimeAssociationService {
public:
  MimeAssociationService(AppRegistry *registry, MimeDefaultsStore *store);

  QVector<MimeEntry> buildEntries() const;
  MimeEntry entryFor(const QString &mime) const;
  void setDefault(const QString &mime, const QString &desktopId);

private:
  AppRegistry *m_registry;
  MimeDefaultsStore *m_store;
};
