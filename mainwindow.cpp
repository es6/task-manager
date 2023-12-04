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

std::vector<QLineSeries*> cpuLineSeriesVector;
std::vector<QLineSeries*> ramSwapLineSeriesVector;
std::vector<int> cpuLastIdle;
std::vector<int> cpuLastUsed;
QChartView *cpuChartView;
QChartView *ramSwapChartView;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    updateSystemInfo();
    updateProcesses();
    updateFileSystemInfo();
    updateCPUResourceInfo();
    updateRamSwapResourceInfo();
    // Create a QTimer object
    graphInfoTimer = new QTimer(this);

    // Set the interval to 1000 milliseconds (1 second)
    graphInfoTimer->setInterval(1000);

    // Connect the timer's timeout signal to the updateCPUResourceInfo slot
    connect(graphInfoTimer, &QTimer::timeout, this, &MainWindow::updateGraphs);

    // Start the timer
    graphInfoTimer->start();
}

MainWindow::~MainWindow() {
    for (QLineSeries* series : cpuLineSeriesVector) {
            delete series; // Release memory for each QLineSeries
    }
    for (QLineSeries* series : ramSwapLineSeriesVector) {
            delete series; // Release memory for each QLineSeries
    }
    delete ui;
}

void MainWindow::updateGraphs(){
    updateCPUResourceInfo();
    updateRamSwapResourceInfo();
}

void MainWindow::updateRamSwapBarChart() {
    QChart *lineChart = new QChart();

    lineChart->setTitle("Memory and Swap History");

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
    lineChart->legend()->setFont(QFont("Arial", 6));

    for (int i = 0; (i < (int)ramSwapLineSeriesVector.size()) ; i++) {
        lineChart->addSeries(ramSwapLineSeriesVector[i]);
        ramSwapLineSeriesVector[i]->attachAxis(axisY);
    }
    ramSwapChartView->setChart(lineChart);
}


void MainWindow::createRamSwapBarChart() {
    QChart *lineChart = new QChart();


    lineChart->setTitle("Memory and Swap History");

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
    lineChart->legend()->setFont(QFont("Arial", 6));

    for (int i = 0; (i < (int)ramSwapLineSeriesVector.size()) ; i++) {
        lineChart->addSeries(ramSwapLineSeriesVector[i]);
        ramSwapLineSeriesVector[i]->attachAxis(axisY);
    }


    ramSwapChartView = new QChartView(lineChart);

    ramSwapChartView->setRenderHint(QPainter::Antialiasing);
    ui->tab_3->layout()->addWidget(ramSwapChartView);
}


void MainWindow::updateRamSwapResourceInfo() {
    QTextStream in;
    QString info;
    QString line;

    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open /proc/meminfo";
        return;
    }

    in.setDevice(&file);
    line = in.readLine();
    double totalMemory;
    double freeMemory;
    double swapTotal;
    double swapFree;
    while (line != nullptr) {
//        qDebug() << line;

        if (line.startsWith("MemTotal:")) {
            QStringList list = line.simplified().split(" ");

            if (list.size() >= 2) {
                totalMemory = list[1].toDouble();
                qDebug() << "Total RAM:" << totalMemory / 1048576 << "GB"; // Convert to GB
            }
        } else if (line.startsWith("MemAvailable:")) {
            QStringList list = line.simplified().split(" ");

            if (list.size() >= 2) {
                freeMemory = list[1].toDouble();
                qDebug() << "Avail:" << freeMemory / 1048576 << "GB"; // Convert to GB
            }
        } else if (line.startsWith("SwapTotal:")) {
            QStringList list = line.simplified().split(" ");

            if (list.size() >= 2) {
                swapTotal = list[1].toDouble();
                qDebug() << "Swap Total:" << swapTotal / 1048576 << "GB"; // Convert to GB
            }
        } else if (line.startsWith("SwapFree:")) {
            QStringList list = line.simplified().split(" ");

            if (list.size() >= 2) {
                swapFree = list[1].toDouble();
                qDebug() << "SwapFree:" << swapFree / 1048576 << "GB"; // Convert to GB
            }
        }
        line = in.readLine();
    }
    double memUsage = ((totalMemory - freeMemory) / totalMemory) * 100;
    double swapUsage = ((swapTotal - swapFree) / swapTotal) * 100;

    qDebug() << memUsage;
    //Initialize the vector
    if(ramSwapLineSeriesVector.size() == 0) {
        //Making the ram line
        QLineSeries *ramLineSeries = new QLineSeries();
        QString sMemUsage = QString::number(memUsage, 'f', 1);
        ramLineSeries->setName("Memory: "+ bytesToMebibytesString((totalMemory - freeMemory) * 1024)
                               + " ("+ sMemUsage + "%) of " + bytesToMebibytesString(totalMemory * 1024));
        for(int i = 60; i > 0 ; i--) {
            ramLineSeries->append(i,0);
        }
        QPointF point = ramLineSeries->at(0);
        point.setY(memUsage);
        ramLineSeries->replace(0, point);
        ramSwapLineSeriesVector.push_back(ramLineSeries);


        QLineSeries *swapLineSeries = new QLineSeries();
        QString sSwapUsage = QString::number(swapUsage, 'f', 1);
        swapLineSeries->setName("Swap: " + bytesToMebibytesString((swapTotal - swapFree) * 1024)
                                + " ("+ sSwapUsage + "%) of " + bytesToMebibytesString(swapTotal * 1024));
        for(int i = 60; i > 0 ; i--) {
            swapLineSeries->append(i,0);
        }
        point = swapLineSeries->at(0);
        point.setY(swapUsage);
        ramSwapLineSeriesVector.push_back(swapLineSeries);

        createRamSwapBarChart();
    } else {
        int ramIndex  = 0;
        int swapIndex  = 1;
        //Ram index is at 0 and Swap is at 1

        for(int i = 60; i > 0; i--){
            QPointF point = ramSwapLineSeriesVector[ramIndex]->at(i);
            point.setY(ramSwapLineSeriesVector[ramIndex]->at(i - 1).y());
            ramSwapLineSeriesVector[ramIndex]->replace(i, point);
        }
        QPointF point = ramSwapLineSeriesVector[ramIndex]->at(0);
        point.setY(memUsage);
        ramSwapLineSeriesVector[ramIndex]->replace(0, point);

        //For swap

        for(int i = 60; i > 0; i--){
            QPointF point = ramSwapLineSeriesVector[swapIndex]->at(i);
            point.setY(ramSwapLineSeriesVector[swapIndex]->at(i - 1).y());
            ramSwapLineSeriesVector[swapIndex]->replace(i, point);
        }
        point = ramSwapLineSeriesVector[swapIndex]->at(0);
        point.setY(swapUsage);
        ramSwapLineSeriesVector[swapIndex]->replace(0, point);
        updateRamSwapBarChart();
    }

    file.close();
}



void MainWindow::createCpuBarChart() {
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

    for (int i = 0; (i < (int)cpuLineSeriesVector.size()) ; i++) {
        lineChart->addSeries(cpuLineSeriesVector[i]);
        cpuLineSeriesVector[i]->attachAxis(axisY);
//        lineChart->addSeries(cpuLineSeriesVector[i]);
    }


    cpuChartView = new QChartView(lineChart);

    cpuChartView->setRenderHint(QPainter::Antialiasing);
    ui->tab_3->layout()->addWidget(cpuChartView);
}

void MainWindow::updateCpuBarChart() {
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

    for (int i = 0; (i < (int)cpuLineSeriesVector.size()) ; i++) {
        lineChart->addSeries(cpuLineSeriesVector[i]);
        cpuLineSeriesVector[i]->attachAxis(axisY);
    }
    cpuChartView->setChart(lineChart);
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
    if(cpuLineSeriesVector.size() == 0) {
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

                    cpuLastIdle.push_back(totalIdle);
                    cpuLastUsed.push_back(totalNonIdle);

                    double cpuUsage = ((double)totalNonIdle)/ ((double)totalCPU) * 100;
                    QString result = QString::number(cpuUsage, 'f', 1);
                    result.append("%");

                    QPointF point = lineSeries->at(0);
                    point.setY(cpuUsage);
                    lineSeries->replace(0, point);


                    //THIS CODE IS INTENDED TO MAKE THE LEGEND DIFFERENT LINES
                    //CURRENTLY DOES NOT WORK
                    if(cpuLineSeriesVector.size() != 0 && cpuLineSeriesVector.size() % 4 == 0){
                        QString dataSet = data[0] + "\n";
                        lineSeries->setName(dataSet);
                    } else {
                        lineSeries->setName(data[0]);
                    }



                    cpuLineSeriesVector.push_back(lineSeries);
//                    qDebug() << "ADDED LINE SERIES";
//                    qDebug() << data[0] + ": "
//                             << result;
                }

                line = in.readLine();
            }
        }
        createCpuBarChart();
        //WORKING ON ELSE
    } else {
        int index = 0;
        while (line != nullptr && !endCPU) {
            if(!line.startsWith("cpu")) {
                endCPU = true;//                    int totalCPU = totalIdle + totalNonIdle;

            } else {
                for(int i = 60; i > 0; i--){
                    QPointF point = cpuLineSeriesVector[index]->at(i);
                    point.setY(cpuLineSeriesVector[index]->at(i - 1).y());
                    cpuLineSeriesVector[index]->replace(i, point);
                }


                QStringList data = line.trimmed().split(" ");
                if(data.size() > 8) {
                    int totalNonIdle = data[1].toInt() + data[2].toInt() + data[3].toInt() +
                            data[6].toInt() + data[7].toInt() + data[8].toInt() + data[9].toInt() +
                            data[10].toInt();
                    int totalIdle = data[4].toInt() + data[5].toInt();
//                    int totalCPU = totalIdle + totalNonIdle;

                    int changeIdle = totalIdle - cpuLastIdle.at(index);
                    int changeUse = totalNonIdle - cpuLastUsed.at(index);
                    int changeTotal = changeIdle + changeUse;
                    double cpuUsage = ((double)changeUse)/ ((double)changeTotal) * 100;

//                    double cpuUsage = ((double)totalNonIdle)/ ((double)totalCPU) * 100;

                    QString result = QString::number(cpuUsage, 'f', 1);
                    result.append("%");
//                    qDebug() << data[0] + ": "
//                             << result;
                    QPointF point = cpuLineSeriesVector[index]->at(0);
//                    point.setY(cpuUsage + (rand() % 100));
                    //Unsure why cpuUsage is not changing from /proc/stat

                    point.setY(cpuUsage);

                    cpuLineSeriesVector[index]->replace(0, point);
                    index++;

                }

                line = in.readLine();
            }
        }
        updateCpuBarChart();
    }
}

QString MainWindow::bytesToMebibytesString(unsigned long bytes) {
    const double mebibyte = 1024 * 1024; // 1 Mebibyte = 1024 * 1024 bytes
    if(bytes < 1000) {
        QString result = QString::number(bytes, 'f', 1);
        result.append(" bytes");
        return result;
    }
    double mebibytes = bytes / mebibyte;

    if(mebibytes > 100) {
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
