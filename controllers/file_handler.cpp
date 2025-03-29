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

  // Check if canceled early
  if (m_isCanceled) {
    emit processingCanceled();
    emit processingFinished();
    return;
  }

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
      qDebug() << sheetNames[i];
      emit progressUpdate(progress);
      if (m_isCanceled) {
        emit processingCanceled();
        emit processingFinished();
        return;
      }
    }
  }
  if (!m_isCanceled) {
    QString filePath = QFileDialog::getSaveFileName(
        nullptr, "Save File", "output.xlsx", "Excel Files (*.xlsx)");
    bool saveSuccess = m_xlsx->saveAs(filePath);
    if (!saveSuccess) {
      qDebug() << "Failed to save output file";
    }
    cleanupDocument();
  }
  emit resultReady(sheetNames);
  emit processingFinished();
  emit progressUpdate(100);
}

void FileHandler::cancelProcess() {
  qDebug() << "Canceling processing...";
  m_isCanceled = true;
}

void FileHandler::convertCell(QString sheetName) {
  if (!m_xlsx || !m_xlsx->selectSheet(sheetName)) {
    qDebug() << "Error: Could not select sheet" << sheetName;
    return;
  }

  QXlsx::CellRange range = m_xlsx->dimension();
  if (!range.isValid()) {
    qDebug() << "Empty sheet or invalid dimension" << sheetName;
    return;
  }

  int maxRow = range.lastRow();
  int maxCol = range.lastColumn();

  // Process in larger chunks
  const int CHUNK_SIZE = 5000;
  for (int startRow = 5; startRow <= maxRow; startRow += CHUNK_SIZE) {
    int endRow = qMin(startRow + CHUNK_SIZE - 1, maxRow);

    for (int col = 0; col < maxCol; col++) {

      for (int row = startRow; row <= endRow; row++) {
        if (row % 100 == 0 && m_isCanceled)
          return;

        QVariant value = m_xlsx->read(row, col);
        if (value.isNull() || value.toString().isEmpty())
          continue;

        QString cellString = value.toString();

        bool potentiallyNumeric = true;
        for (const QChar &c : cellString) {
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
            m_xlsx->write(row, col, numValue);
          }
        }
      }
      QCoreApplication::processEvents();
    }
  }
}

void FileHandler::clearMemoryCache() { QCoreApplication::processEvents(); }