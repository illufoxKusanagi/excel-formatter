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
  numFormat.setNumberFormat("#,##0.00");
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
      processCell(sheetNames[i]);
      emit progressUpdate(progress);
    }
  }
  emit resultReady(sheetNames);
  emit progressUpdate(99);
  QThread::sleep(3);
  emit processingFinished();
}

void FileHandler::cancelProcess() {
  QMessageBox::information(nullptr, "Processing Canceled",
                           "File processing has been canceled by user");
  m_isCanceled = true;
}

void FileHandler::processCell(QString sheetName) {
  m_xlsx->selectSheet(sheetName);
  QCoreApplication::processEvents();
  QXlsx::CellRange range = m_xlsx->dimension();
  if (!range.isValid()) {
    QMessageBox::warning(nullptr, "Error",
                         "Invalid range in the Excel file. Please check the "
                         "file and try again.");
    return;
  }

  int maxRow = range.lastRow();
  int maxCol = range.lastColumn();

  const int CHUNK_SIZE = 1000;

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
        convertCell(row, col, value);
      }
    }
  }
  clearMemoryCache();
}

void FileHandler::convertCell(const int row, const int col,
                              const QVariant value) {
  cellString = value.toString();
  bool potentiallyNumeric = true;
  bool isNegative = !cellString.isEmpty() && cellString[0] == '-';
  for (int i = 0; i < cellString.length(); i++) {
    QChar c = cellString[i];
    if ((c == '-' && i == 0) || c == "0") {
      continue;
    }
    if (!c.isDigit() && c != '.' && c != ',') {
      potentiallyNumeric = false;
      break;
    }
  }

  if (potentiallyNumeric) {
    normalized = cellString;
    normalized.replace(',', '.');
    bool conversionOk = false;
    double numValue = normalized.toDouble(&conversionOk);

    if (conversionOk) {
      m_xlsx->write(row, col, numValue, numFormat);
    }
  }
}

void FileHandler::handleSaveFile(const QString savePath) {
  if (!savePath.isEmpty()) {
    if (m_xlsx) {
      emit saveProgressUpdate(0);

      // Use a timer to simulate progress updates during saving
      QTimer *timer = new QTimer(this);
      int progress = 0;

      // Connect timer to update progress
      connect(timer, &QTimer::timeout, [this, timer, progress]() mutable {
        if (progress < 90) {
          progress += 2;
          emit saveProgressUpdate(progress);
        }
      });

      // Use QtConcurrent to perform save in a background thread
      QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);

      // When save completes in background thread, handle results
      connect(watcher, &QFutureWatcher<bool>::finished, [=]() {
        // Stop the timer
        timer->stop();
        timer->deleteLater();

        // Get the save result
        bool success = watcher->result();

        // Show 100% progress
        emit saveProgressUpdate(99);

        // Notify completion
        QThread::sleep(1);
        emit saveCompleted(success, savePath);
        // Clean up
        watcher->deleteLater();
        cleanupDocument();
      });

      // Start the timer
      timer->start(200);

      // Start the save operation in background thread
      QFuture<bool> future = QtConcurrent::run(
          [this, savePath]() { return m_xlsx->saveAs(savePath); });

      // Set the future to watch
      watcher->setFuture(future);
    } else {
      emit saveCompleted(false, "");
    }
  }
}

void FileHandler::clearMemoryCache() {
  QCoreApplication::processEvents();

#ifdef Q_OS_WIN
  // Windows-specific memory release
  HANDLE process = GetCurrentProcess();

  // Simpler approach using SetProcessWorkingSetSize
  // This tells Windows to trim the working set to minimum
  SetProcessWorkingSetSize(process, (SIZE_T)-1, (SIZE_T)-1);

  // Alternative approach if the above doesn't work
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);

  // Allocate and free a large block of memory to flush caches
  void *tempMem = VirtualAlloc(NULL, sysInfo.dwPageSize * 4096,
                               MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (tempMem) {
    // Write to the memory to ensure it's actually allocated
    memset(tempMem, 0, sysInfo.dwPageSize * 4096);
    // Free it immediately
    VirtualFree(tempMem, 0, MEM_RELEASE);
  }
#endif

  // Force garbage collection in Qt
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

  // Give system time to reclaim memory
  QThread::msleep(5);
}