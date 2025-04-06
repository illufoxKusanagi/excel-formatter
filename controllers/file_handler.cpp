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
  m_xlsx = new QXlsx::Document(filePath);
  m_numFormat.setNumberFormat("#,##0.00");
  bool success = m_xlsx->load();
  m_rawFileSize = QFileInfo(filePath).size();
  m_fileSize = getHumanReadableSize(m_rawFileSize);
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
  m_cellString = value.toString();
  bool potentiallyNumeric = true;
  bool isNegative = !m_cellString.isEmpty() && m_cellString[0] == '-';
  for (int i = 0; i < m_cellString.length(); i++) {
    QChar c = m_cellString[i];
    if ((c == '-' && i == 0) || c == "0") {
      continue;
    }
    if (!c.isDigit() && c != '.' && c != ',') {
      potentiallyNumeric = false;
      break;
    }
  }

  if (potentiallyNumeric) {
    m_normalizedCell = m_cellString;
    m_normalizedCell.replace(',', '.');
    bool conversionOk = false;
    double numValue = m_normalizedCell.toDouble(&conversionOk);

    if (conversionOk) {
      m_xlsx->write(row, col, numValue, m_numFormat);
    }
  }
}

void FileHandler::handleSaveFile(const QString savePath) {
  if (!savePath.isEmpty()) {
    if (m_xlsx) {
      emit saveProgressUpdate(0);
      QTimer *timer = new QTimer(this);
      int progress = 0;
      connect(timer, &QTimer::timeout, [this, timer, progress]() mutable {
        if (progress < 90) {
          progress += 1;
          emit saveProgressUpdate(progress);
        }
      });
      QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>(this);
      connect(watcher, &QFutureWatcher<bool>::finished, [=]() {
        timer->stop();
        timer->deleteLater();
        bool success = watcher->result();
        emit saveProgressUpdate(99);
        QThread::sleep(1);
        emit saveCompleted(success, savePath);
        watcher->deleteLater();
        cleanupDocument();
      });

      const qint64 SIZE_THRESHOLD = 20 * 1024 * 1024; // 20 MB
      if (m_rawFileSize > SIZE_THRESHOLD) {
        timer->start(500);
      } else {
        timer->start(100);
      }
      QFuture<bool> future = QtConcurrent::run(
          [this, savePath]() { return m_xlsx->saveAs(savePath); });
      watcher->setFuture(future);
    } else {
      emit saveCompleted(false, "");
    }
  }
}

QString FileHandler::getHumanReadableSize(qint64 bytes) {
  constexpr qint64 KB = 1024;
  constexpr qint64 MB = 1024 * KB;
  constexpr qint64 GB = 1024 * MB;

  if (bytes < KB) {
    return QString("%1 bytes").arg(bytes);
  } else if (bytes < MB) {
    return QString("%1 KB").arg(bytes / double(KB), 0, 'f', 2);
  } else if (bytes < GB) {
    return QString("%1 MB").arg(bytes / double(MB), 0, 'f', 2);
  } else {
    return QString("%1 GB").arg(bytes / double(GB), 0, 'f', 2);
  }
}

void FileHandler::clearMemoryCache() {
  QCoreApplication::processEvents();

#ifdef Q_OS_WIN
  HANDLE process = GetCurrentProcess();
  SetProcessWorkingSetSize(process, (SIZE_T)-1, (SIZE_T)-1);
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  void *tempMem = VirtualAlloc(NULL, sysInfo.dwPageSize * 4096,
                               MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (tempMem) {
    memset(tempMem, 0, sysInfo.dwPageSize * 4096);
    VirtualFree(tempMem, 0, MEM_RELEASE);
  }
#endif
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QThread::msleep(5);
}

void FileHandler::pauseProcessing() {
  QMutexLocker locker(&g_mutex);
  g_paused = true;
  // g_pauseCondition.wait(&g_mutex, 1000);
}
void FileHandler::resumeProcessing() {
  QMutexLocker locker(&g_mutex);
  g_paused = false;
  g_pauseCondition.wakeAll();
}

bool FileHandler::g_paused = false;

QMutex FileHandler::g_mutex;

QWaitCondition FileHandler::g_pauseCondition;