#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../controllers/file_handler.h"
#include "xlsxdocument.h"
#include <QCheckBox>
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
  void onProcessButtonClicked();

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

private:
  QLabel *m_label;
  QPushButton *m_browseFileButton;
  QSlider *m_qualitySlider;
  QSpinBox *m_qualityValue;
  QProgressDialog *m_progress;
  QProgressDialog *m_saveProgress;
  QThread m_thread;
  FileHandler *m_fileHandler;
  QString *m_selectedFilePath;
  QCheckBox *m_checkBox;
  QPushButton *m_processButton;
  void processExcel();
  void handleExcelResult(const QStringList &sheetNames);
  void startExcelProcessing(const QString &filePath);
  void setupConnections();
  void connectSignalsAndSlots();
};
#endif // MAINWINDOW_H
