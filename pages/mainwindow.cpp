#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Excel converter");
  resize(600, 400);

  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  QWidget *dropFileWidget = new QWidget(this);
  dropFileWidget->setAcceptDrops(true);
  QWidget *buttonWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(dropFileWidget);
  QVBoxLayout *buttonLayout = new QVBoxLayout(buttonWidget);
  m_label = new QLabel("Drag and drop your excel file (.xlsx) here \n or press "
                       "the button to browse file",
                       this);
  m_label->setAlignment(Qt::AlignCenter);
  m_browseFileButton = new QPushButton("Browse file", this);

  m_fileHandler = new FileHandler();
  m_fileHandler->moveToThread(&m_thread);
  m_processButton = new QPushButton("Process file", this);
  m_checkBox = new QCheckBox("Sort file", this);
  m_checkBox->setChecked(false);
  connect(m_checkBox, &QCheckBox::checkStateChanged, this,
          &MainWindow::switchSortingOption);

  layout->addWidget(m_label);
  layout->addWidget(m_browseFileButton);
  layout->addWidget(m_processButton);
  layout->addWidget(m_checkBox);

  mainLayout->addWidget(dropFileWidget);

  m_processButton->setVisible(false);
  m_checkBox->setVisible(false);
  connectSignalsAndSlots();
  m_thread.start();
}

MainWindow::~MainWindow() {
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

  if (m_saveProgress) {
    m_saveProgress->close();
    delete m_saveProgress;
    m_saveProgress = nullptr;
  }

  delete m_fileHandler;
  delete m_selectedFilePath;
  delete m_label;
  delete m_browseFileButton;
  delete m_processButton;
  delete m_checkBox;
}

void MainWindow::switchSortingOption() {
  m_isSortingEnabled = m_checkBox->isChecked();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void MainWindow::getFile() {
  QString filePath = QFileDialog::getOpenFileName(nullptr, "Pilih file", "",
                                                  "Excel Files (*.xlsx)");
  if (filePath.isEmpty()) {
    QMessageBox::warning(nullptr, "Error", "No file selected!");
    return;
  }
  m_selectedFilePath = new QString(filePath);
  m_label->setText("Selected file at: \n" + *m_selectedFilePath);
  m_processButton->setVisible(true);
  m_checkBox->setVisible(true);
}

void MainWindow::dropEvent(QDropEvent *event) {
  QList<QUrl> urls = event->mimeData()->urls();
  if (urls.isEmpty())
    return;
  QString filePath = urls.first().toLocalFile();
  m_selectedFilePath = new QString(filePath);
  m_label->setText("Selected file at: \n" + *m_selectedFilePath);
  m_processButton->setVisible(true);
  m_checkBox->setVisible(true);
}

void MainWindow::startExcelProcessing(const QString &filePath) {
  m_progress =
      new QProgressDialog("Loading Excel file...", "Cancel", 0, 100, this);
  m_progress->setWindowModality(Qt::WindowModal);
  m_progress->setValue(0);
  m_progress->setMinimumDuration(0);
  m_progress->show();
  m_progress->setCancelButton(nullptr);
  connect(m_progress, &QProgressDialog::canceled, this,
          &MainWindow::cancelProcessing);
  QMetaObject::invokeMethod(m_fileHandler, "procesFile", Qt::QueuedConnection,
                            Q_ARG(QString, filePath),
                            Q_ARG(bool, m_isSortingEnabled));
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
  m_saveProgress->setCancelButton(nullptr);
  connect(m_saveProgress, &QProgressDialog::canceled, this,
          &MainWindow::cancelProcessing);
  QMetaObject::invokeMethod(m_fileHandler, "handleSaveFile",
                            Qt::QueuedConnection, Q_ARG(QString, savePath));
}

void MainWindow::onProcessButtonClicked() {
  if (m_selectedFilePath) {
    startExcelProcessing(*m_selectedFilePath);
  } else {
    QMessageBox::warning(this, "Error", "No file selected!");
  }
}

void MainWindow::connectSignalsAndSlots() {

  connect(m_processButton, &QPushButton::clicked, this,
          &MainWindow::onProcessButtonClicked);
  connect(m_browseFileButton, &QPushButton::clicked, this,
          &MainWindow::getFile);

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
}

void MainWindow::cancelProcessing() {
  QMetaObject::invokeMethod(m_fileHandler, "pauseProcessing",
                            Qt::QueuedConnection);
  QProgressDialog *activeProgress = qobject_cast<QProgressDialog *>(sender());
  bool isSaveProgress = (activeProgress == m_saveProgress);
  int ret = QMessageBox::warning(
      this, tr("Warning!"),
      tr("Your file is processed in background.\n"
         "Do you want to forcibly cancel and terminate the operation?"),
      QMessageBox::Yes | QMessageBox::No);

  if (ret == QMessageBox::Yes) {
    QMetaObject::invokeMethod(m_fileHandler, "cancelProcess",
                              Qt::QueuedConnection);
    if (m_thread.isRunning()) {
      m_thread.requestInterruption();
      m_thread.terminate();
      m_thread.wait(1000);
      QMessageBox::information(this, "Processing Canceled",
                               "File processing has been canceled by user");
    }
  } else if (ret == QMessageBox::No) {
    QMetaObject::invokeMethod(m_fileHandler, "resumeProcessing",
                              Qt::QueuedConnection);
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
