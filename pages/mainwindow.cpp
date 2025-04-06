#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Excel converter");
  resize(600, 400);

  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  QVBoxLayout *layout = new QVBoxLayout(centralWidget);
  m_label = new QLabel("Drag and drop your excel file (.xlsx) here \n or press "
                       "the button to browse file",
                       this);
  m_label->setAlignment(Qt::AlignCenter);

  m_browseFileButton = new QPushButton("Browse file", this);

  layout->addWidget(m_label);
  layout->addWidget(m_browseFileButton);
  connect(m_browseFileButton, &QPushButton::clicked, this,
          &MainWindow::getFile);
  setAcceptDrops(true);

  m_fileHandler = new FileHandler();
  m_fileHandler->moveToThread(&m_thread);

  connect(m_fileHandler, &FileHandler::resultReady, this,
          &MainWindow::handleExcelResult);
  connect(m_fileHandler, &FileHandler::progressUpdate, this,
          [this](int percentage) {
            if (m_progress && m_progress->isVisible()) {
              m_progress->setValue(percentage);
            }
          });
  connect(m_fileHandler, &FileHandler::processingCanceled, this, [this]() {
    QMessageBox::information(this, "Processing Canceled",
                             "File processing has been canceled by user");
  });
  connect(m_fileHandler, &FileHandler::processingFinished, [this]() {
    if (m_progress)
      disconnect(m_progress, &QProgressDialog::canceled, this,
                 &MainWindow::cancelProcessing);
    m_progress->close();
  });

  connect(m_fileHandler, &FileHandler::saveProgressUpdate, this,
          [this](int percentage) {
            if (m_saveProgress && m_saveProgress->isVisible()) {
              m_saveProgress->setValue(percentage);
            }
          });

  connect(m_fileHandler, &FileHandler::saveCompleted, this,
          [this](bool success, const QString &path) {
            if (m_saveProgress) {
              disconnect(m_saveProgress, &QProgressDialog::canceled, this,
                         &MainWindow::cancelProcessing);
              m_saveProgress->close();
              delete m_saveProgress;
              m_saveProgress = nullptr;
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

  m_thread.start();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

MainWindow::~MainWindow() {
  // Properly shut down the thread
  if (m_thread.isRunning()) {
    m_thread.quit();
    m_thread.wait(1000);
    if (m_thread.isRunning()) {
      m_thread.terminate();
    }
  }
  if (m_progress) {
    m_progress->close();
    delete m_progress;
    m_progress = nullptr;
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
  m_progress =
      new QProgressDialog("Loading Excel file...", "Cancel", 0, 100, this);
  m_progress->setWindowModality(Qt::WindowModal);
  m_progress->setValue(0);
  m_progress->setMinimumDuration(0);
  m_progress->show();
  connect(m_progress, &QProgressDialog::canceled, this,
          &MainWindow::cancelProcessing);
  QMetaObject::invokeMethod(m_fileHandler, "procesFile", Qt::QueuedConnection,
                            Q_ARG(QString, filePath));
}

void MainWindow::handleExcelResult(const QStringList &sheetNames) {
  QMessageBox::information(
      this, "Success",
      "Data processed successfully! Now please save your file!");
  QString savePath = QFileDialog::getSaveFileName(
      this, "Save File", QDir::homePath(), "Excel Files (*.xlsx)");
  m_saveProgress =
      new QProgressDialog("Saving Excel file...", "Cancel", 0, 100, this);
  m_saveProgress->setWindowModality(Qt::WindowModal);
  m_saveProgress->setValue(0);
  m_saveProgress->setMinimumDuration(0);
  m_saveProgress->show();
  connect(m_saveProgress, &QProgressDialog::canceled, this,
          &MainWindow::cancelProcessing);
  QMetaObject::invokeMethod(m_fileHandler, "handleSaveFile",
                            Qt::QueuedConnection, Q_ARG(QString, savePath));
}

void MainWindow::dropEvent(QDropEvent *event) {
  QList<QUrl> urls = event->mimeData()->urls();
  if (urls.isEmpty())
    return;
  QString filePath = urls.first().toLocalFile();
  startExcelProcessing(filePath);
}

void MainWindow::cancelProcessing() {
  // 1. Immediately pause the worker
  QMetaObject::invokeMethod(m_fileHandler, "pauseProcessing",
                            Qt::QueuedConnection);

  // Store which progress dialog triggered the cancellation
  QProgressDialog *activeProgress = qobject_cast<QProgressDialog *>(sender());
  bool isSaveProgress = (activeProgress == m_saveProgress);

  // 2. Ask user whether to continue or forcibly cancel
  int ret = QMessageBox::warning(
      this, tr("Warning!"),
      tr("Your file is processed in background.\n"
         "Do you want to forcibly cancel and terminate the operation?"),
      QMessageBox::Yes | QMessageBox::No);

  if (ret == QMessageBox::Yes) {
    // User confirmed the forced cancellation
    QMetaObject::invokeMethod(m_fileHandler, "cancelProcess",
                              Qt::QueuedConnection);
    if (m_thread.isRunning()) {
      // Terminate the thread
      m_thread.requestInterruption();
      m_thread.terminate();
      m_thread.wait(1000);
      QMessageBox::information(this, "Processing Canceled",
                               "File processing has been canceled by user");
    }
  } else if (ret == QMessageBox::No) {
    // User wants to continue
    // 3. Tell the worker to resume processing
    QMetaObject::invokeMethod(m_fileHandler, "resumeProcessing",
                              Qt::QueuedConnection);

    // 4. Restore the progress dialog
    if (isSaveProgress && m_saveProgress) {
      connect(m_saveProgress, &QProgressDialog::canceled, this,
              &MainWindow::cancelProcessing);
      int currentValue = m_saveProgress->value();
      m_saveProgress->show();
      m_saveProgress->setValue(currentValue);
    } else if (!isSaveProgress && m_progress) {
      connect(m_progress, &QProgressDialog::canceled, this,
              &MainWindow::cancelProcessing);
      int currentValue = m_progress->value();
      m_progress->show();
      m_progress->setValue(currentValue);
    }
  }
}