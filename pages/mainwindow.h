#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../controllers/file_handler.h"
#include "xlsxdocument.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

private slots:
  void getFile();
  void cancelProcessing();

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  QLabel *label;
  QPushButton *button;
  QSlider *qualitySlider;
  QSpinBox *qualityValue;
  QProgressDialog *progress;
  QThread thread;
  FileHandler *fileHandler;
  void processExcel();
  void handleExcelResult(const QStringList &sheetNames);
  void startExcelProcessing(const QString &filePath);
};
#endif // MAINWINDOW_H
