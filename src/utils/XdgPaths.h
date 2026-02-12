#pragma once

#include <QString>
#include <QStringList>

class XdgPaths {
public:
  static QString configHome();
  static QStringList configDirs();
  static QString dataHome();
  static QStringList dataDirs();
  static QStringList appDirs();

private:
  static QString expandHome(const QString &path);
  static QStringList splitPaths(const QString &value);
};
