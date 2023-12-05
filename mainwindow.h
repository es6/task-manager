#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextStream>
#include <QtCharts>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct ProcessInfo {
    int pid;
    int ppid;
    int uid;
    QString name;
    QString status;
    QString memory;
    // Other fields as needed
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
    void updateProcesses();
    void createCpuBarChart();
    void updateCpuBarChart();
    void createRamSwapBarChart();
    void updateRamSwapBarChart();
    void createNetworkBarChart();
    void updateNetworkBarChart();
    void updateGraphs();
    void updateNetworkResourceInfo();
    void updateRamSwapResourceInfo();
    std::vector<QLineSeries*> cpuLineSeriesVector; // Vector of QLineSeries pointers
    std::vector<QLineSeries*> ramSwapLineSeriesVector; // Vector of QLineSeries pointers
    std::vector<QLineSeries*> networkLineSeriesVector;
    QChartView *cpuChartView;
    QChartView *ramSwapChartView;
    QChartView *networkChartView;
    long recievedLast;
    long uploadLast;
//    void updateCPUResourceInfo();
    void updateProcesses(bool showOnlyUserProcess, bool treeView);
    QString kbToMiB(const QString &memLine);
    QString getProcessUid(const QString &statusPath);
    QMap<int, ProcessInfo> readAllProcesses(bool showOnlyUserProcess);
    ProcessInfo readProcessInfo(const QString &statusPath);
    void buildProcessTree(const QMap<int, ProcessInfo> &processes);
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateCPUResourceInfo(); // Declaration for the CPU info update function
    void onProcessFilterChanged(int index);
    void pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *graphInfoTimer; // QTimer object to trigger updates
};
#endif // MAINWINDOW_H
