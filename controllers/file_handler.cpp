#include "file_handler.h"

FileHandler::FileHandler() {}

FileHandler::~FileHandler() { cleanupDocument(); }

void FileHandler::cleanupDocument() {
  if (m_xlsx) {
    delete m_xlsx;
    m_xlsx = nullptr;
  }
}

void FileHandler::procesFile(QString filePath) {
  m_isCanceled = false;
  emit progressUpdate(0);
  m_currentFilePath = filePath;
  m_xlsx = new QXlsx::Document(filePath);
  bool success = m_xlsx->load();
  emit progressUpdate(10);
  QStringList sheetNames;
  if (success) {
    sheetNames = m_xlsx->sheetNames();
    for (int i = 0; i < sheetNames.size(); i++) {
      QCoreApplication::processEvents();
      if (m_isCanceled) {
        emit processingCanceled();
        emit processingFinished();
        return;
      }
      int progress = 10 + (i + 1) * 80 / sheetNames.size();
      convertCell(sheetNames[i]);
      emit progressUpdate(progress);
    }
  }
  // if (!m_isCanceled) {
  //   bool saveSuccess = m_xlsx->saveAs(savePath);
  //   if (!saveSuccess) {
  //     QMessageBox::warning(nullptr, "Error",
  //                          "Failed to save the file. Please try again.");
  //   }
  //   cleanupDocument();
  // }
  emit resultReady(sheetNames);
  emit processingFinished();
  emit progressUpdate(100);
}

void FileHandler::cancelProcess() {
  QMessageBox::information(nullptr, "Processing Canceled",
                           "File processing has been canceled by user");
  m_isCanceled = true;
}

void FileHandler::convertCell(QString sheetName) {
  m_xlsx->selectSheet(sheetName);
  QXlsx::CellRange range = m_xlsx->dimension();
  if (!range.isValid()) {
    QMessageBox::warning(nullptr, "Error",
                         "Invalid range in the Excel file. Please check the "
                         "file and try again.");
    return;
  }

  int maxRow = range.lastRow();
  int maxCol = range.lastColumn();

  // Process in larger chunks
  const int CHUNK_SIZE = 5000;
  for (int startRow = 0; startRow <= maxRow; startRow += CHUNK_SIZE) {
    int endRow = qMin(startRow + CHUNK_SIZE - 1, maxRow);

    for (int col = 0; col <= maxCol; col++) {

      for (int row = startRow; row <= endRow; row++) {
        if (row % 100 == 0 && m_isCanceled) {
          return;
        }

        QVariant value = m_xlsx->read(row, col);
        if (value.isNull() || value.toString().isEmpty())
          continue;

        QString cellString = value.toString();
        bool potentiallyNumeric = true;
        bool isNegative = !cellString.isEmpty() && cellString[0] == '-';
        for (int i = 0; i < cellString.length(); i++) {
          QChar c = cellString[i];
          // Allow minus sign only at the beginning
          if ((c == '-' && i == 0) || c == "0") {
            continue;
          }
          // Otherwise only allow digits and decimal separators
          if (!c.isDigit() && c != '.' && c != ',') {
            potentiallyNumeric = false;
            break;
          }
        }

        if (potentiallyNumeric) {
          QString normalized = cellString;
          normalized.replace(',', '.');
          bool conversionOk = false;
          double numValue = normalized.toDouble(&conversionOk);

          if (conversionOk) {
            QXlsx::Format numFormat;
            numFormat.setNumberFormat("#,##0.00");
            m_xlsx->write(row, col, numValue, numFormat);
          }
        }
      }
      QCoreApplication::processEvents();
    }
  }
}

void FileHandler::handleSaveFile() {
  QString savePath = QFileDialog::getSaveFileName(
      nullptr, "Save File", QDir::homePath(), "Excel Files (*.xlsx)");
  if (!savePath.isEmpty()) {
    if (m_xlsx) {
      // Create a progress dialog for saving
      // QProgressDialog *saveProgress =
      //     new QProgressDialog("Saving file...", "Cancel", 0, 100, nullptr);
      // saveProgress->setWindowModality(Qt::WindowModal);
      // saveProgress->setValue(0);
      // saveProgress->setMinimumDuration(0);
      // saveProgress->show();

      // Use a timer to simulate progress updates since QXlsx doesn't provide
      // progress info
      //   QTimer *timer = new QTimer();
      //   int progress = 0;

      //   QObject::connect(timer, &QTimer::timeout, [=]() mutable {
      //     // Increment progress to simulate saving progress
      //     if (progress < 90) {
      //       progress += 5;
      //       saveProgress->setValue(progress);
      //     }
      //     QCoreApplication::processEvents();
      //   });

      //   // Start the timer
      //   timer->start(200);

      //   // Perform the actual save
      //   bool saveSuccess = false;
      //   try {
      //     saveSuccess = m_xlsx->saveAs(savePath);
      //     // Set to 100% when complete
      //     saveProgress->setValue(100);
      //   } catch (...) {
      //     saveSuccess = false;
      //   }

      //   // Stop the timer
      //   timer->stop();
      //   timer->deleteLater();

      //   // Close and delete the progress dialog
      //   saveProgress->close();
      //   saveProgress->deleteLater();
      // }
      m_xlsx->saveAs(savePath);
      QMessageBox::information(nullptr, "Success",
                               "File saved successfully to: " + savePath);
    } else {
      QMessageBox::warning(nullptr, "Error", "Saving file canceled by user.");
    }
    cleanupDocument();
  }
}

void FileHandler::clearMemoryCache() { QCoreApplication::processEvents(); }