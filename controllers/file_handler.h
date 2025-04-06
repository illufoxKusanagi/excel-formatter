#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "xlsxdocument.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QObject>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QString>
#include <QVector>
#include <QtConcurrent/QtConcurrent>

class FileHandler : public QObject {
  Q_OBJECT

public slots:
  void cancelProcess();
  void procesFile(const QString filePath);
  void handleSaveFile(const QString savePath);

signals:
  void resultReady(const QStringList &sheetNames);
  void processingFinished();
  void progressUpdate(int percentage);
  void processingCanceled();
  void saveProgressUpdate(int percentage);
  void saveCompleted(bool success, const QString &path);

public:
  FileHandler();
  ~FileHandler();
  static bool g_paused;
  static QMutex g_mutex;
  static QWaitCondition g_pauseCondition;

private:
  bool m_isCanceled = false;
  QXlsx::Document *m_xlsx = nullptr;
  QString m_cellString;
  QString m_normalizedCell;
  QXlsx::Format m_numFormat;
  QList<QFutureWatcher<void> *> m_watchers;
  QStringList m_sheetNames;
  QString m_fileSize;
  qint64 m_rawFileSize;

  void processCell(QString sheetName);
  void processExcel(QString sheetName);
  void cleanupDocument();
  void clearMemoryCache();
  void processSheetRange(QString sheetName, int startRow, int endRow,
                         QVector<int> columnsToCheck);
  void convertCell(const int row, const int col, const QVariant value);
  QString getHumanReadableSize(qint64 bytes);
  void pauseProcessing();
  void resumeProcessing();
};

#endif // FILE_HANDLER_H

#ifdef Q_OS_WIN
#include <windows.h>
#pragma comment(lib, "kernel32.lib")
#endif
