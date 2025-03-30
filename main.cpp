#include "pages/mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QIcon appIcon;
  appIcon.addFile(":/icons/excelConvert32.ico", QSize(32, 32));
  appIcon.addFile(":/icons/excelConvert64.ico", QSize(64, 64));
  appIcon.addFile(":/icons/excelConvert128.ico", QSize(128, 128));
  app.setWindowIcon(appIcon);
  MainWindow w;
  w.show();
  return app.exec();
}
