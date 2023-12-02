#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextStream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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
    QString kbToMiB(const QString &memLine);
    QString getProcessUid(const QString &statusPath);
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateCPUResourceInfo(); // Declaration for the CPU info update function
    void onProcessFilterChanged(int index);
    void pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *cpuInfoTimer; // QTimer object to trigger updates
};
#endif // MAINWINDOW_H
