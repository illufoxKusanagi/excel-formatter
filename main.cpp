#include "pages/mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
  QIcon appIcon;
  QApplication app(argc, argv);
  MainWindow w;
  appIcon.addFile(":/icons/excelConvert32.png", QSize(32, 32));
  appIcon.addFile(":/icons/excelConvert64.png", QSize(64, 64));
  appIcon.addFile(":/icons/excelConvert128.png", QSize(128, 128));
  app.setWindowIcon(appIcon);
  w.show();
  return app.exec();
}
