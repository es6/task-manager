#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextStream>
#include <QDateTime>
#include <QDate>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct ProcessDetails {
    QString name;
    QString user;
    QString state;
    QString memory;
    QString vmemory;
    QString rmemory;
    QString smemory;
    QString cpuTime;
    QDateTime startTime;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    double hardDiskCheck(const QString &disk);
    void updateSystemInfo();
    void updateFileSystemInfo();
    void printAll(QString info, QTextStream &in);
    QString bytesToMebibytesString(unsigned long bytes);

    void updateProcesses(bool showOnlyUserProcess);
    void processActions(const QPoint &pos);
    QString kbToMiB(const QString &memLine);
    QString getProcessUid(const QString &statusPath);
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateCPUResourceInfo(); // Declaration for the CPU info update function
    void onProcessFilterChanged(int index);
    void pushButton_clicked();
    void stopProcess(int pid);
    void continueProcess(int pid);
    void killProcess(int pid);
    void listMapsProcess(int pid);
    void showMemoryMapsDialog(const QStringList &lines);
    void listFilesProcess(int pid);
    void propertiesProcess(int pid);
    ProcessDetails getProcessDetails(int pid);


private:
    Ui::MainWindow *ui;
    QTimer *cpuInfoTimer; // QTimer object to trigger updates
};
#endif // MAINWINDOW_H
