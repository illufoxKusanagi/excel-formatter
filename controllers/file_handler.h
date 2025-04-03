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
  QString cellString;
  QString normalized;
  QXlsx::Format numFormat;
  QList<QFutureWatcher<void> *> m_watchers;
  void processCell(QString sheetName);
  void processExcel(QString sheetName);
  void cleanupDocument();
  void clearMemoryCache();
  void processSheetRange(QString sheetName, int startRow, int endRow,
                         QVector<int> columnsToCheck);
  void convertCell(const int row, const int col, const QVariant value);
};

#endif // FILE_HANDLER_H
