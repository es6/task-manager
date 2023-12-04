#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextStream>
#include <QtCharts>


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
    void updateProcesses();
    void createCpuBarChart();
    void updateCpuBarChart();
    void createRamSwapBarChart();
    void updateRamSwapBarChart();
    void updateGraphs();
    void updateRamSwapResourceInfo();
    std::vector<QLineSeries*> cpuLineSeriesVector; // Vector of QLineSeries pointers
    std::vector<QLineSeries*> ramSwapLineSeriesVector; // Vector of QLineSeries pointers
    QChartView *cpuChartView;
    QChartView *ramSwapChartView;
//    void updateCPUResourceInfo();
    QString kbToMiB(const QString &memLine);
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateCPUResourceInfo(); // Declaration for the CPU info update function

private:
    Ui::MainWindow *ui;
    QTimer *graphInfoTimer; // QTimer object to trigger updates
};
#endif // MAINWINDOW_H
