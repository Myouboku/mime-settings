#include "ui/MainWindow.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QFont font("Noto Sans");
  if (!font.family().isEmpty()) {
    app.setFont(font);
  }

  MainWindow window;
  window.show();

  return app.exec();
}
