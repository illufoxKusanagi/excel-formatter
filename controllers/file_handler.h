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
  void procesFile(QString filePath);
  void cancelProcess();

signals:
  void resultReady(const QStringList &sheetNames);
  void processingFinished();
  void progressUpdate(int percentage);
  void processingCanceled();

public:
  FileHandler();
  ~FileHandler();
  void handleSaveFile();

private:
  bool m_isCanceled = false;
  QXlsx::Document *m_xlsx = nullptr;
  QString m_currentFilePath;
  QList<QFutureWatcher<void> *> m_watchers;
  void convertCell(QString sheetName);
  void processExcel(QString sheetName);
  void cleanupDocument();
  void clearMemoryCache();
  void processSheetRange(QString sheetName, int startRow, int endRow,
                         QVector<int> columnsToCheck);
};

#endif // FILE_HANDLER_H
