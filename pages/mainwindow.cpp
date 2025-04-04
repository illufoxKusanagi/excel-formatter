#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Excel converter");
  resize(600, 400);

  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  QVBoxLayout *layout = new QVBoxLayout(centralWidget);
  label = new QLabel("Drag and drop your excel file (.xlsx) here \n or press "
                     "the button to browse file",
                     this);
  label->setAlignment(Qt::AlignCenter);

  button = new QPushButton("Browse file", this);

  layout->addWidget(label);
  layout->addWidget(button);
  connect(button, &QPushButton::clicked, this, &MainWindow::getFile);
  setAcceptDrops(true);

  fileHandler = new FileHandler();
  fileHandler->moveToThread(&thread);

  connect(fileHandler, &FileHandler::resultReady, this,
          &MainWindow::handleExcelResult);
  connect(fileHandler, &FileHandler::progressUpdate, this,
          [this](int percentage) {
            if (progress && progress->isVisible()) {
              progress->setValue(percentage);
            }
          });
  connect(fileHandler, &FileHandler::processingCanceled, this, [this]() {
    QMessageBox::information(this, "Processing Canceled",
                             "File processing has been canceled by user");
  });
  connect(fileHandler, &FileHandler::processingFinished, [this]() {
    if (progress)
      disconnect(progress, &QProgressDialog::canceled, this,
                 &MainWindow::cancelProcessing);
    progress->close();
  });

  connect(fileHandler, &FileHandler::saveProgressUpdate, this,
          [this](int percentage) {
            if (saveProgress && saveProgress->isVisible()) {
              saveProgress->setValue(percentage);
            }
          });

  connect(fileHandler, &FileHandler::saveCompleted, this,
          [this](bool success, const QString &path) {
            if (saveProgress) {
              disconnect(saveProgress, &QProgressDialog::canceled, this,
                         &MainWindow::cancelProcessing);
              saveProgress->close();
              delete saveProgress;
              saveProgress = nullptr;
            }

            // Show appropriate message
            if (success) {
              QMessageBox::information(this, "Success",
                                       "File saved successfully to: " + path);
            } else {
              QMessageBox::critical(
                  this, "Error",
                  "Failed to save the file. Please try again later.");
            }
          });

  thread.start();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

MainWindow::~MainWindow() {
  // Properly shut down the thread
  if (thread.isRunning()) {
    thread.quit();
    thread.wait(1000);
    if (thread.isRunning()) {
      thread.terminate();
    }
  }
  if (progress) {
    progress->close();
    delete progress;
    progress = nullptr;
  }
}

void MainWindow::getFile() {
  QString filePath = QFileDialog::getOpenFileName(nullptr, "Pilih file", "",
                                                  "Excel Files (*.xlsx)");
  if (filePath.isEmpty()) {
    QMessageBox::warning(nullptr, "Error", "No file selected!");
    return;
  }
  startExcelProcessing(filePath);
}

void MainWindow::startExcelProcessing(const QString &filePath) {
  progress =
      new QProgressDialog("Loading Excel file...", "Cancel", 0, 100, this);
  progress->setWindowModality(Qt::WindowModal);
  progress->setValue(0);
  progress->setMinimumDuration(0);
  progress->show();
  connect(progress, &QProgressDialog::canceled, this,
          &MainWindow::cancelProcessing);
  QMetaObject::invokeMethod(fileHandler, "procesFile", Qt::QueuedConnection,
                            Q_ARG(QString, filePath));
}

void MainWindow::handleExcelResult(const QStringList &sheetNames) {
  QMessageBox::information(
      this, "Success",
      "Data processed successfully! Now please save your file!");
  QString savePath = QFileDialog::getSaveFileName(
      this, "Save File", QDir::homePath(), "Excel Files (*.xlsx)");
  saveProgress =
      new QProgressDialog("Saving Excel file...", "Cancel", 0, 100, this);
  saveProgress->setWindowModality(Qt::WindowModal);
  saveProgress->setValue(0);
  saveProgress->setMinimumDuration(0);
  saveProgress->show();
  connect(saveProgress, &QProgressDialog::canceled, this,
          &MainWindow::cancelProcessing);
  QMetaObject::invokeMethod(fileHandler, "handleSaveFile", Qt::QueuedConnection,
                            Q_ARG(QString, savePath));
}

void MainWindow::dropEvent(QDropEvent *event) {
  QList<QUrl> urls = event->mimeData()->urls();
  if (urls.isEmpty())
    return;
  QString filePath = urls.first().toLocalFile();
  startExcelProcessing(filePath);
}

void MainWindow::cancelProcessing() {
  int ret = QMessageBox::warning(
      this, tr("Warning!"),
      tr("Your file is processed in background.\n"
         "Do you want to forcibly cancel and terminate the operation?"),
      QMessageBox::Yes | QMessageBox::No);

  if (ret == QMessageBox::Yes) {
    QMetaObject::invokeMethod(fileHandler, "cancelProcess",
                              Qt::QueuedConnection);
    if (thread.isRunning()) {
      thread.requestInterruption();
      thread.terminate();
      thread.wait(1000);
      QMessageBox::information(this, "Processing Canceled",
                               "File processing has been canceled by user");
    }
  }
}