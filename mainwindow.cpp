#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegExp>
#include <QtCharts>
#include <QMainWindow>
#include <filesystem>
#include <QStorageInfo>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QStringList>
#include <sys/statvfs.h>
#define GB (1024 * 1024 * 1024)
std::vector<QLineSeries*> lineSeriesVector;
QChartView *chartView;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    updateSystemInfo();
    updateProcesses();
    updateFileSystemInfo();
    updateCPUResourceInfo();
    // Create a QTimer object
    cpuInfoTimer = new QTimer(this);

    // Set the interval to 1000 milliseconds (1 second)
    cpuInfoTimer->setInterval(1000);

    // Connect the timer's timeout signal to the updateCPUResourceInfo slot
    connect(cpuInfoTimer, &QTimer::timeout, this, &MainWindow::updateCPUResourceInfo);

    // Start the timer
    cpuInfoTimer->start();
}

MainWindow::~MainWindow()
{
    for (QLineSeries* series : lineSeriesVector) {
            delete series; // Release memory for each QLineSeries
    }
    delete ui;
}


void MainWindow::createBarChart() {
    QChart *lineChart = new QChart();


    lineChart->setTitle("CPU stats");

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Seconds");
    axisX->setRange(0, 60);
    axisX->setReverse(true);
    lineChart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%.0f%%");
    lineChart->addAxis(axisY, Qt::AlignLeft);


    lineChart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);

    // Set legend alignment to horizontally spaced out
    lineChart->legend()->setAlignment(Qt::AlignBottom);
    lineChart->legend()->setContentsMargins(0, 0, 0, 0);
    lineChart->legend()->setFont(QFont("Arial", 4));

    for (int i = 0; (i < (int)lineSeriesVector.size()) ; i++) {
        lineChart->addSeries(lineSeriesVector[i]);
        lineSeriesVector[i]->attachAxis(axisY);
//        lineChart->addSeries(lineSeriesVector[i]);
    }


    chartView = new QChartView(lineChart);

    chartView->setRenderHint(QPainter::Antialiasing);

//    // Create a frame to contain the chart
//    QFrame *frame = new QFrame(ui->tab_3);
//    QVBoxLayout *frameLayout = new QVBoxLayout(frame);
//    frameLayout->addWidget(chartView);



//    // Set the frame as the central widget of tab_3
//    ui->tab_3->setLayout(frameLayout);
    ui->tab_3->layout()->addWidget(chartView);

}

void MainWindow::updateBarChart() {
    QChart *lineChart = new QChart();

    lineChart->setTitle("CPU stats");

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Seconds");
    axisX->setRange(0, 60);
    axisX->setReverse(true);
    lineChart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%.0f%%");
    lineChart->addAxis(axisY, Qt::AlignLeft);


    lineChart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);

    // Set legend alignment to horizontally spaced out
    lineChart->legend()->setAlignment(Qt::AlignBottom);
    lineChart->legend()->setContentsMargins(0, 0, 0, 0);
    lineChart->legend()->setFont(QFont("Arial", 4));

    for (int i = 0; (i < (int)lineSeriesVector.size()) ; i++) {
        lineChart->addSeries(lineSeriesVector[i]);
        lineSeriesVector[i]->attachAxis(axisY);
    }
        chartView->setChart(lineChart);
}


void MainWindow::updateCPUResourceInfo() {
    QFile file("/proc/stat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open /proc/stat";
        return;
    }

    QTextStream in;
    in.setDevice(&file);
    bool endCPU = false;
    QString line = in.readLine();

    //Case where vector has not been initially populated
    if(lineSeriesVector.size() == 0) {
        // Create a frame to contain the chart
        QFrame *frame = new QFrame(ui->tab_3);
        QVBoxLayout *frameLayout = new QVBoxLayout(frame);
    //    frameLayout->addWidget(chartView);



        // Set the frame as the central widget of tab_3
        ui->tab_3->setLayout(frameLayout);
        //Creates the new line series and populates it with zero values
        while (line != nullptr && !endCPU) {

            if(!line.startsWith("cpu")) {
                endCPU = true;
            } else {

                QLineSeries *lineSeries = new QLineSeries();

                for(int i = 60; i > 0 ; i--) {
                    lineSeries->append(i,0);
                }
                QStringList data = line.trimmed().split(" ");
                if(data.size() > 8) {
                    int totalNonIdle = data[1].toInt() + data[2].toInt() + data[3].toInt() +
                            data[6].toInt() + data[7].toInt() + data[8].toInt() + data[9].toInt() +
                            data[10].toInt();
                    int totalIdle = data[4].toInt() + data[5].toInt();
                    int totalCPU = totalIdle + totalNonIdle;
                    double cpuUsage = ((double)totalNonIdle)/ ((double)totalCPU) * 100;
                    QString result = QString::number(cpuUsage, 'f', 1);
                    result.append("%");

                    QPointF point = lineSeries->at(0);
                    point.setY(cpuUsage);
                    lineSeries->replace(0, point);

                    //THIS CODE IS INTENDED TO MAKE THE LEGEND DIFFERENT LINES
                    //CURRENTLY DOES NOT WORK
                    if(lineSeriesVector.size() != 0 && lineSeriesVector.size() % 4 == 0){
                        QString dataSet = data[0] + "\n";
                        lineSeries->setName(dataSet);
                    } else {
                        lineSeries->setName(data[0]);
                    }



                    lineSeriesVector.push_back(lineSeries);
                    qDebug() << "ADDED LINE SERIES";
//                    qDebug() << data[0] + ": "
//                             << result;
                }

                line = in.readLine();
            }
        }
        createBarChart();
        //WORKING ON ELSE
    } else {
        int index = 0;
        while (line != nullptr && !endCPU) {
            if(!line.startsWith("cpu")) {
                endCPU = true;
            } else {
//                QPointF point = lineSeriesVector[index]->at(40);
//                point.setY(40);
//                lineSeriesVector[index]->replace(40, point);
                for(int i = 60; i > 0; i--){
                    QPointF point = lineSeriesVector[index]->at(i);
                    point.setY(lineSeriesVector[index]->at(i - 1).y());
                    lineSeriesVector[index]->replace(i, point);
                }

                QStringList data = line.trimmed().split(" ");
                if(data.size() > 8) {
                    int totalNonIdle = data[1].toInt() + data[2].toInt() + data[3].toInt() +
                            data[6].toInt() + data[7].toInt() + data[8].toInt() + data[9].toInt() +
                            data[10].toInt();
                    int totalIdle = data[4].toInt() + data[5].toInt();
                    int totalCPU = totalIdle + totalNonIdle;
                    double cpuUsage = ((double)totalNonIdle)/ ((double)totalCPU) * 100;
                    QString result = QString::number(cpuUsage, 'f', 1);
                    result.append("%");
                    qDebug() << data[0] + ": "
                             << result;
                    index++;
                }
                line = in.readLine();
            }
        }
        updateBarChart();
    }
}

QString MainWindow::bytesToMebibytesString(unsigned long bytes) {
    const double mebibyte = 1024 * 1024; // 1 Mebibyte = 1024 * 1024 bytes
    double mebibytes = bytes / mebibyte;

    if(mebibytes > 1000) {
        double gibibytes = mebibytes / 1024;
        QString result = QString::number(gibibytes, 'f', 1);
        result.append(" GiB");
        return result;
    } else {
        QString result = QString::number(mebibytes, 'f', 1);
        result.append(" MiB");
        return result;
    }
}

void MainWindow::updateFileSystemInfo() {
//    qDebug() << "IN FILE";
    QFile file("/proc/mounts");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open /proc/mounts";
        return;
    }

//    qDebug() << "IN FILE";
    QTextStream in;
    in.setDevice(&file);
    QString line = in.readLine();
    while (line != nullptr) {
        QStringList parts = line.split(' ');

        QString mountPoint = parts[1];
        QString device = parts[0];
        QString type = parts[2];

        // Get filesystem statistics using statvfs
        struct statvfs buffer;
        if (statvfs(mountPoint.toStdString().c_str(), &buffer) == 0 &&
                (device.startsWith("/dev"))) {
            unsigned long totalSpace = buffer.f_blocks * buffer.f_frsize;
            unsigned long freeSpace = buffer.f_bfree * buffer.f_frsize;
            unsigned long availableSpace = buffer.f_bavail * buffer.f_frsize;
            unsigned long usedSpace = totalSpace - freeSpace;

            // Convert sizes to Mebibytes (MiB) or Gibibyte (Gib) and represent as QStrings
            QString totalSpaceMiB = bytesToMebibytesString(totalSpace);
            QString freeSpaceMiB = bytesToMebibytesString(freeSpace);
            QString availableSpaceMiB = bytesToMebibytesString(availableSpace);
            QString usedSpaceMiB = bytesToMebibytesString(usedSpace);

            QTreeWidgetItem *topItem = new QTreeWidgetItem();
            topItem->setText(0, device);
            topItem->setText(1, mountPoint);
            topItem->setText(2, type);
            topItem->setText(3, totalSpaceMiB);
            topItem->setText(4, freeSpaceMiB);
            topItem->setText(5, availableSpaceMiB);
            topItem->setText(6, usedSpaceMiB);
            ui->treeWidgetFileSystem->addTopLevelItem(topItem);
//            qDebug() << "Mount Point:" << mountPoint
//                     << "Device:" << device
//                     << "Type:" << type
//                     << "Total:" << totalSpace
//                     << "Free:" << freeSpace
//                     << "Available:" << availableSpace
//                     << "Used:" << usedSpace;
        }
        line = in.readLine();
    }
    file.close();
}




void MainWindow::updateProcesses() {
    QDir procDir("/proc");
    QStringList pidList = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    ui->treeWidget->clear();

    QFile statusFile;

    foreach (const QString &pidDir, pidList) {
        bool ok;
        int pid = pidDir.toInt(&ok);
        if (ok) {
            // Read process information from /proc/[pid] files
            QString statusPath = "/proc/" + pidDir + "/status";
            statusFile.setFileName(statusPath);
            if (statusFile.open(QIODevice::ReadOnly)) {
                QTextStream in(&statusFile);
                QString line = in.readLine();
                QString processName;
                QString processStatus;
                QString processMemory;
                QString qPid;
                while (line != nullptr) {
//                    qDebug() << "line lol" << line;
                    if (line.startsWith("Name:")) {
                        processName = line.split("\t").last();
                    } else if (line.startsWith("State:")) {
                        QString stateLine = line.split("\t").last();
                        if (stateLine.startsWith("S")) {
                            processStatus = "Sleeping";
                        } else if (stateLine.startsWith("R")) {
                            processStatus = "Running";
                        } else {
                            processStatus = "";
                        }
                    } else if (line.startsWith("VmRSS:")) {
                        processMemory = kbToMiB(line.split("\t").last());
                    }
                    line = in.readLine();
                }
                statusFile.close();

                // Add process information to tree widget
                QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
                item->setText(0, processName);
                item->setText(1, processStatus);
                item->setText(2, QString::number(0)); // Placeholder for CPU usage
                item->setText(3, pidDir);
                item->setText(4, processMemory);
            }
        }
    }
}

QString MainWindow::kbToMiB(const QString &memLine) {
    QRegExp rx("(\\d+)");
    int pos = rx.indexIn(memLine);
    if (pos > -1) {
        QString memKbString = rx.cap(1);

        // Convert to float and then to MiB
        float memKb = memKbString.toFloat();
        float memMiB = memKb / 1024; // 1024 KB in a MiB

        // Format to one decimal place
        return QString("%1 MiB").arg(memMiB, 0, 'f', 1);
    }
    return QString();
}

/*
 * 1.1 Basic System Information
 */

void MainWindow::updateSystemInfo() {
    QFile file;
    QTextStream in;
    QString info;
    QString line;

    file.setFileName("/proc/sys/kernel/osrelease");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        in.setDevice(&file);
        info = "OS Release Version: ";
        printAll(info, in);
        file.close();
    }

    file.setFileName("/proc/sys/kernel/version");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        in.setDevice(&file);
        info = "Kernel Version: ";
        printAll(info, in);
        file.close();
    }

    file.setFileName("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        in.setDevice(&file);
        line = in.readLine();
        QRegExp rx("(\\d+)");
        int pos = rx.indexIn(line);
        QString memKbString = rx.cap(1);

        // Convert to float and then to GB
        float memKb = memKbString.toFloat();
        float memGb = memKb / 1048576;

        // Format to one decimal place
        QString memInfo = QString("Memory: %1 GB").arg(memGb, 0, 'f', 1);

        ui->listWidget->addItem(memInfo);
//        qDebug() << memInfo;
        file.close();
    }

    file.setFileName("/proc/cpuinfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        in.setDevice(&file);
        QString cpuInfoLabel = "model name\t: ";
        QString cpuInfo;
        line = in.readLine();
        while (line != nullptr) {
            if (line.startsWith(cpuInfoLabel)) {
                cpuInfo = line.replace(cpuInfoLabel, "");
                break;
            }
            line = in.readLine();
        }

        if (!cpuInfo.isEmpty()) {
            ui->listWidget->addItem("Processor: " + cpuInfo);
        }
        file.close();
    }

    // Display disk space here with hardDiskCheck
    double diskSpace = hardDiskCheck("/homes/lin1413");
    if (diskSpace != -1) {
        QString diskSpaceInfo = QString("Available disk space: %1 GB").arg(diskSpace, 0, 'f', 1);
        ui->listWidget->addItem(diskSpaceInfo);
        qDebug() << diskSpaceInfo;
    }
}

void MainWindow::printAll(QString info, QTextStream &in) {
    QString line = in.readLine();
    while (line != nullptr) {
        ui->listWidget->addItem(info + line);
        line = in.readLine();
    }
}

double MainWindow::hardDiskCheck(const QString &disk)
{
    QStorageInfo storage(disk);
    if(storage.isValid() && storage.isReady())
    {
        return storage.bytesAvailable() * 1.0 / GB;
    }
    return -1;
}
