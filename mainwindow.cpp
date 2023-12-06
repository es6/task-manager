#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QMenu>
#include <QTextStream>
#include <QDebug>
#include <QDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QFormLayout>
#include <QLabel>
#include <QDir>
#include <QHeaderView>
#include <QRegExp>
#include <QtCharts>
#include <QMainWindow>
#include <filesystem>
#include <QStorageInfo>
#include <QTimer>
#include <QDir>
#include <QCoreApplication>
#include <QStringList>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <sys/statvfs.h>
#include <unistd.h>
#define GB (1024 * 1024 * 1024)

std::vector<QLineSeries*> cpuLineSeriesVector;
std::vector<QLineSeries*> ramSwapLineSeriesVector;
std::vector<QLineSeries*> networkLineSeriesVector;
std::vector<int> cpuLastIdle;
std::vector<int> cpuLastUsed;
long recievedLast;
long uploadLast;
QChartView *cpuChartView;
QChartView *ramSwapChartView;
QChartView *networkChartView;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, &QWidget::customContextMenuRequested, this, &MainWindow::processActions);

    updateSystemInfo();
    updateProcesses(false, false);
    updateFileSystemInfo();
    updateCPUResourceInfo();
    updateRamSwapResourceInfo();
    updateNetworkResourceInfo();

    connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onProcessFilterChanged(int)));
    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(pushButton_clicked()));

    int desiredWidth = 300; // Set this to your desired width in pixels
    ui->treeWidget->setColumnWidth(0, desiredWidth);

    QCoreApplication::setApplicationName("Linux Task Manager");
    setWindowTitle( QCoreApplication::applicationName() );

    ui->treeWidget->setStyleSheet("QTreeView::branch { border-image: url(none); }"); // Optional, for styling
    ui->treeWidget->setRootIsDecorated(true); // Ensure root decoration is enabled
    ui->treeWidget->setUniformRowHeights(false); // For performance
    ui->treeWidget->setItemsExpandable(true); // Ensure items are expandable
    ui->treeWidget->setExpandsOnDoubleClick(true); // Expand/collapse on double-click
    ui->treeWidget->setAnimated(true); // Enable animations
    ui->treeWidget->setAllColumnsShowFocus(true); // Focus on all columns
    ui->treeWidget->setAlternatingRowColors(true); // For better readability
    ui->treeWidget->setSortingEnabled(true); // Enable sorting if needed


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
    updateNetworkResourceInfo();
}
void MainWindow::createNetworkBarChart() {
    QChart *lineChart = new QChart();


    lineChart->setTitle("Network History");

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Seconds");
    axisX->setRange(0, 60);
    axisX->setReverse(true);
    lineChart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%.0f KiB/s");
    lineChart->addAxis(axisY, Qt::AlignLeft);


    lineChart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);

    // Set legend alignment to horizontally spaced out
    lineChart->legend()->setAlignment(Qt::AlignBottom);
    lineChart->legend()->setContentsMargins(0, 0, 0, 0);
    lineChart->legend()->setFont(QFont("Arial", 8));

    for (int i = 0; (i < (int)networkLineSeriesVector.size()) ; i++) {
        lineChart->addSeries(networkLineSeriesVector[i]);
        networkLineSeriesVector[i]->attachAxis(axisY);
    }


    networkChartView = new QChartView(lineChart);

    networkChartView->setRenderHint(QPainter::Antialiasing);
    ui->tab_3->layout()->addWidget(networkChartView);
}
void MainWindow::updateNetworkBarChart() {
    QChart *lineChart = new QChart();

    lineChart->setTitle("Network History");

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Seconds");
    axisX->setRange(0, 60);
    axisX->setReverse(true);
    lineChart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%.0f KiB/s");
    lineChart->addAxis(axisY, Qt::AlignLeft);


    lineChart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);

    // Set legend alignment to horizontally spaced out
    lineChart->legend()->setAlignment(Qt::AlignBottom);
    lineChart->legend()->setContentsMargins(0, 0, 0, 0);
    lineChart->legend()->setFont(QFont("Arial", 8));

    for (int i = 0; (i < (int)networkLineSeriesVector.size()) ; i++) {
        lineChart->addSeries(networkLineSeriesVector[i]);
        networkLineSeriesVector[i]->attachAxis(axisY);
    }
    networkChartView->setChart(lineChart);
}

void MainWindow::updateNetworkResourceInfo() {
    QTextStream in;
    QString info;
    QString line;
    long recieveTotal = 0;
    long uploadTotal = 0;
    QFile file("/proc/net/dev");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open /proc/meminfo";
        return;
    }

    in.setDevice(&file);
    line = in.readLine();
    //Does Recieve then transmit
    while(line != nullptr) {
        if (!line.startsWith("Inter-|") && !line.startsWith(" face |")) {
//            qDebug() << line.simplified().split(" ");
            QStringList data = line.simplified().split(" ");
            recieveTotal += data[1].toLong();
            uploadTotal += data[9].toLong();
        } else {
//            qDebug() << "HEaders";
        }
        line = in.readLine();
    }

    if(networkLineSeriesVector.size() == 0) {
        //Making the network line
        //Initially since we need the change to plot a point it is 0
        QLineSeries *recievingLineSeries = new QLineSeries();
        for(int i = 60; i >= 0 ; i--) {
            recievingLineSeries->append(i,0);
        }
        recievingLineSeries->setName("Recieving: 0 KiB/s | Total Recieved: " + bytesToMebibytesString(recieveTotal));

        networkLineSeriesVector.push_back(recievingLineSeries);

        QLineSeries *sendingLineSeries = new QLineSeries();
        for(int i = 60; i >= 0 ; i--) {
            sendingLineSeries->append(i,0);
        }
        sendingLineSeries->setName("Recieving: 0 KiB/s | Total Recieved: " + bytesToMebibytesString(uploadTotal));
        networkLineSeriesVector.push_back(sendingLineSeries);
        recievedLast = recieveTotal;
        uploadLast = uploadTotal;
        createNetworkBarChart();
    } else {
        long recieveIndex  = 0;
        long sendIndex  = 1;
        //recieve index is at 0 and send is at 1
        long recieveKib = ((recieveTotal - recievedLast) / 1024);
        long sendKib = ((uploadTotal - uploadLast) / 1024);
          //For recieve
        for(int i = 60; i > 0; i--){
            QPointF point = networkLineSeriesVector[recieveIndex]->at(i);
            point.setY(networkLineSeriesVector[recieveIndex]->at(i - 1).y());
            networkLineSeriesVector[recieveIndex]->replace(i, point);
        }

        QPointF point = networkLineSeriesVector[recieveIndex]->at(0);
        point.setY(recieveKib);
        networkLineSeriesVector[recieveIndex]->setName("Recieving: "+ bytesToMebibytesString(recieveKib*1024)+"/s | Total Recieved: " + bytesToMebibytesString(recieveTotal));
        networkLineSeriesVector[recieveIndex]->replace(0, point);

        //For send

        for(int i = 60; i > 0; i--){
            QPointF point = networkLineSeriesVector[sendIndex]->at(i);
            point.setY(networkLineSeriesVector[sendIndex]->at(i - 1).y());
            networkLineSeriesVector[sendIndex]->replace(i, point);
        }
        point = networkLineSeriesVector[sendIndex]->at(0);
        point.setY(sendKib);
        networkLineSeriesVector[sendIndex]->setName("Sending: "+ bytesToMebibytesString(sendKib * 1024)+"/s | Total Sent: " + bytesToMebibytesString(recieveTotal));

        networkLineSeriesVector[sendIndex]->replace(0, point);
        recievedLast = recieveTotal;
        uploadLast = uploadTotal;
        updateNetworkBarChart();
    }

//    qDebug() << "Recieved:"
//             << bytesToMebibytesString(recieveTotal);
//    qDebug() << "Trans:"
//             << bytesToMebibytesString(uploadTotal);
//    recievedLast = recieveTotal;
//    uploadLast = uploadTotal;
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
    lineChart->legend()->setFont(QFont("Arial", 8));

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
    lineChart->legend()->setFont(QFont("Arial", 8));
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
            }
        } else if (line.startsWith("MemAvailable:")) {
            QStringList list = line.simplified().split(" ");

            if (list.size() >= 2) {
                freeMemory = list[1].toDouble();
            }
        } else if (line.startsWith("SwapTotal:")) {
            QStringList list = line.simplified().split(" ");

            if (list.size() >= 2) {
                swapTotal = list[1].toDouble();
            }
        } else if (line.startsWith("SwapFree:")) {
            QStringList list = line.simplified().split(" ");

            if (list.size() >= 2) {
                swapFree = list[1].toDouble();
            }
        }
        line = in.readLine();
    }
    double memUsage = ((totalMemory - freeMemory) / totalMemory) * 100;
    double swapUsage = ((swapTotal - swapFree) / swapTotal) * 100;

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
    lineChart->legend()->setFont(QFont("Arial", 2));

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
    lineChart->legend()->setFont(QFont("Arial", 2));

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
                    QString dataSet = data[0] +" (" +result + ")";
                    lineSeries->setName(dataSet);




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
    if(bytes < 256) {
        QString result = QString::number(bytes, 'f', 1);
        result.append(" bytes");
        return result;
    }
    double kibibytes = bytes / 1024;
    double mebibytes = bytes / mebibyte;

    if(kibibytes < 1000) {
        QString result = QString::number(kibibytes, 'f', 1);
        result.append(" KiB");
        return result;
    }

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

void MainWindow::onProcessFilterChanged(int index) {
    bool showOnlyUserProcesses = (index == 1 || index == 3); // Indices 1 and 3 for user processes
    bool treeView = (index == 2 || index == 3); // Indices 2 and 3 for tree view

    updateProcesses(showOnlyUserProcesses, treeView);
}

QMap<int, ProcessInfo> MainWindow::readAllProcesses(bool showOnlyUserProcess) {
    QMap<int, ProcessInfo> processes;
    QDir procDir("/proc");
    QStringList pidDirs = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    uid_t currentUid = getuid();

    foreach (const QString &pidDir, pidDirs) {
        bool ok;
        int pid = pidDir.toInt(&ok);
        if (!ok) continue;

        QString statusPath = "/proc/" + pidDir + "/status";
        ProcessInfo procInfo = readProcessInfo(statusPath);

        if (showOnlyUserProcess && procInfo.uid != currentUid) continue;

        processes[pid] = procInfo;
    }

    return processes;
}

ProcessInfo MainWindow::readProcessInfo(const QString &statusPath) {
    ProcessInfo info;
    QFile file(statusPath);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        QString line = in.readLine();
        while (line != nullptr) {
            if (line.startsWith("Name:")) {
                info.name = line.split("\t").last();
            } else if (line.startsWith("State:")) {
                QString stateLine = line.split("\t").last();
                if (stateLine.startsWith("S")) {
                    info.status = "Sleeping";
                } else if (stateLine.startsWith("R")) {
                    info.status = "Running";
                } else {
                    info.status = "";
                }
            } else if (line.startsWith("VmRSS:")) {
                info.memory = kbToMiB(line.split("\t").last());
            } else if (line.startsWith("Pid:")) {
                info.pid = line.split("\t").last().toInt();
            } else if (line.startsWith("PPid:")) {
                info.ppid = line.split("\t").last().toInt();
            } else if (line.startsWith("Uid:")) {
                info.uid = line.split("\t").last().split(" ")[0].toInt(); // Uid is the first field in a space-separated list
            }
            line = in.readLine();
            // Read other fields as needed
        }
    }
    return info;
}


void MainWindow::buildProcessTree(const QMap<int, ProcessInfo> &processes) {
    QMap<int, QTreeWidgetItem*> processItems;
    ui->treeWidget->clear();

    // Create tree widget items for each process
    for (auto it = processes.cbegin(); it != processes.cend(); ++it) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        const ProcessInfo &info = it.value();
        if (info.memory != "") {
            item->setText(0, info.name);
            item->setText(1, info.status);
            item->setText(2, QString::number(0)); // Placeholder for CPU usage
            item->setText(3, QString::number(info.pid));
            item->setText(4, info.memory);
            processItems[info.pid] = item;
        }
    }

    // Build the tree structure
    for (auto it = processes.cbegin(); it != processes.cend(); ++it) {
        const ProcessInfo &info = it.value();
        QTreeWidgetItem *childItem = processItems.value(info.pid);
        if (info.ppid == 0 || !processItems.contains(info.ppid)) {
            // This is a top-level process or orphan
            ui->treeWidget->addTopLevelItem(childItem);
        } else {
            // This is a child process
            QTreeWidgetItem *parentItem = processItems.value(info.ppid);
            parentItem->addChild(childItem);
        }
    }
    ui->treeWidget->expandAll();
}


void MainWindow::updateProcesses(bool showOnlyUserProcess, bool treeView) {
    if (treeView) {
        QMap<int, ProcessInfo> processes = readAllProcesses(showOnlyUserProcess);
        buildProcessTree(processes);
        ui->treeWidget->expandAll();
    }
    else {
        QDir procDir("/proc");
        QStringList pidList = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        ui->treeWidget->clear();

        QFile statusFile;
        uid_t currentUid = getuid();

        foreach (const QString &pidDir, pidList) {
            bool ok;
            int pid = pidDir.toInt(&ok);
            if (ok) {
                // Read process information from /proc/[pid] files
                QString statusPath = "/proc/" + pidDir + "/status";
                statusFile.setFileName(statusPath);
                QString processUid = getProcessUid(statusPath);
                if (showOnlyUserProcess && (processUid.toInt() != currentUid)) {
                    continue;
                }
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

                    if (processMemory != "") {
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
    }
}

QString MainWindow::getProcessUid(const QString &statusPath) {
    QFile file(statusPath);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        QString line = in.readLine();
        while (line != nullptr) {
            if (line.startsWith("Uid:")) {
                return line.split("\t").at(1); // Uid is the second field
            }
            line = in.readLine();
        }
        file.close();
    }
    return QString();
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

void MainWindow::pushButton_clicked() {
    int index = ui->comboBox->currentIndex();
    bool showOnlyUserProcesses = (index == 1 || index == 3); // Indices 1 and 3 for user processes
    bool treeView = (index == 2 || index == 3); // Indices 2 and 3 for tree view

    updateProcesses(showOnlyUserProcesses, treeView);
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
//        qDebug() << diskSpaceInfo;
    }
}

void MainWindow::printAll(QString info, QTextStream &in) {
    QString line = in.readLine();
    while (line != nullptr) {
        ui->listWidget->addItem(info + line);
        line = in.readLine();
    }
}

double MainWindow::hardDiskCheck(const QString &disk) {
    QStorageInfo storage(disk);
    if(storage.isValid() && storage.isReady())
    {
        return storage.bytesAvailable() * 1.0 / GB;
    }
    return -1;
}

/*
 * 1.2.1 Process Actions and 1.2.2 Detailed View (Properties)
 */

void MainWindow::processActions(const QPoint &pos) {
    QTreeWidgetItem* selectedItem = ui->treeWidget->itemAt(pos);
    if (!selectedItem) return;

    int pid = selectedItem->text(3).toInt();

    QMenu contextMenu;
    QAction* stopAction = contextMenu.addAction(tr("Stop Process"));
    QAction* continueAction = contextMenu.addAction(tr("Continue Process"));
    QAction* killAction = contextMenu.addAction(tr("Kill Process"));
    QAction* listMapsAction = contextMenu.addAction(tr("List Memory Maps"));
    QAction* listFilesAction = contextMenu.addAction(tr("List Open Files"));
    QAction* propertiesAction = contextMenu.addAction(tr("Properties"));

    connect(stopAction, &QAction::triggered, this, [this, pid]() { stopProcess(pid); });
    connect(continueAction, &QAction::triggered, this, [this, pid]() { continueProcess(pid); });
    connect(killAction, &QAction::triggered, this, [this, pid]() { killProcess(pid); });
    connect(listMapsAction, &QAction::triggered, this, [this, pid]() { listMapsProcess(pid); });
    connect(listFilesAction, &QAction::triggered, this, [this, pid]() { listFilesProcess(pid); });
    connect(propertiesAction, &QAction::triggered, this, [this, pid]() { propertiesProcess(pid); });


    contextMenu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
}


void MainWindow::stopProcess(int pid) {
    if (pid > 0) {
        int result = kill(pid, SIGSTOP);
        if (result == 0) {
            qDebug() << "Process with PID" << pid << "stopped successfully.";
        }
    }
}

void MainWindow::continueProcess(int pid) {
    if (pid > 0) {
        int result = kill(pid, SIGCONT);
        if (result == 0) {
            qDebug() << "Process with PID" << pid << "continued successfully.";
        }
    }
}

void MainWindow::killProcess(int pid) {
    if (pid > 0) {
        int result = kill(pid, SIGKILL);
        if (result == 0) {
            qDebug() << "Process with PID" << pid << "killed successfully.";
        }
    }
}

void MainWindow::listMapsProcess(int pid) {
    QString mapsFilePath = QString("/proc/%1/smaps").arg(pid);
    QFile mapsFile(mapsFilePath);

    if (!mapsFile.open(QIODevice::ReadOnly | QIODevice::Text)) { //debug statement
        qDebug() << "Failed to open" << mapsFilePath;
        return;
    }
    QString mapsFileContent = mapsFile.readAll();
    QStringList lines = mapsFileContent.split('\n');

    showMemoryMapsDialog(lines);
}

void MainWindow::showMemoryMapsDialog(const QStringList &lines) {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Memory Maps"));

    QTableWidget *table = new QTableWidget(0, 10, dialog); // Start with 0 rows and add as needed
    table->setHorizontalHeaderLabels({"Filename", "VM Start", "VM End", "VM Size", "Flags", "VM Offset", "Private Clean", "Private Dirty", "Shared Clean", "Shared Dirty"});
    table->setMinimumSize(800, 600);

    int row = 0;
    QString fileName, VMStart, VMEnd, VMSize, flags, VMOffset, privateClean, privateDirty, sharedClean, sharedDirty;

    foreach (const QString &line, lines) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        QStringList columns = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        if (row == 0 && columns.count() >= 6) {
            VMStart = columns[0].split("-")[0];
            VMEnd = columns[0].split("-")[1];
            flags = columns[1];
            VMOffset = columns[2];
            fileName = columns[5].trimmed();
        }
        else if (line.startsWith("Size:")) {
            VMSize = columns[1] + " kB";
        }
        else if (line.startsWith("Shared_Clean:")) {
            sharedClean = columns[1] + " kB";
        }
        else if (line.startsWith("Shared_Dirty:")) {
            sharedDirty = columns[1] + " kB";
        }
        else if (line.startsWith("Private_Clean:")) {
            privateClean = columns[1] + " kB";
        }
        else if (line.startsWith("Private_Dirty:")) {
            privateDirty = columns[1] + " kB";
        }

        if (row == 23) {
            // Add a row only if fileName is not empty
            if (!fileName.isEmpty()) {
                table->insertRow(table->rowCount()); // Insert a new row
                table->setItem(table->rowCount() - 1, 0, new QTableWidgetItem(fileName));
                table->setItem(table->rowCount() - 1, 1, new QTableWidgetItem(VMStart));
                table->setItem(table->rowCount() - 1, 2, new QTableWidgetItem(VMEnd));
                table->setItem(table->rowCount() - 1, 3, new QTableWidgetItem(VMSize));
                table->setItem(table->rowCount() - 1, 4, new QTableWidgetItem(flags));
                table->setItem(table->rowCount() - 1, 5, new QTableWidgetItem(VMOffset));
                table->setItem(table->rowCount() - 1, 6, new QTableWidgetItem(privateClean));
                table->setItem(table->rowCount() - 1, 7, new QTableWidgetItem(privateDirty));
                table->setItem(table->rowCount() - 1, 8, new QTableWidgetItem(sharedClean));
                table->setItem(table->rowCount() - 1, 9, new QTableWidgetItem(sharedDirty));
            }

            // Reset all the QString variables for the next map entry
            fileName = VMStart = VMEnd = VMSize = flags = VMOffset = privateClean = privateDirty = sharedClean = sharedDirty = "";
            row = -1;
        }
        row++;
    }

    table->resizeColumnsToContents();
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(table);
    dialog->setLayout(layout);
    dialog->exec();
}


void MainWindow::listFilesProcess(int pid) {
    //Random
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Open Files"));

    QTableWidget *table = new QTableWidget(0, 2, dialog);
    table->setMinimumSize(800, 600);
    table->setHorizontalHeaderLabels({"File Descriptors", "Target Path"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QString fdPath = QString("/proc/%1/fd").arg(pid);
    QDir fdDir(fdPath);

    if (!fdDir.exists()) {
        qDebug() << "The /proc directory for PID" << pid << "does not exist";
        return;
    }

    QStringList files = fdDir.entryList(QDir::NoDotAndDotDot | QDir::System);
    for (const QString &file : files) {
        QString linkPath = fdDir.absoluteFilePath(file);
        QFileInfo fileInfo(linkPath);

        int currentRow = table->rowCount();
        table->insertRow(currentRow);

        table->setItem(currentRow, 0, new QTableWidgetItem(file));

        if (fileInfo.isSymLink()) {
            table->setItem(currentRow, 1, new QTableWidgetItem(fileInfo.symLinkTarget()));
        } else {
            table->setItem(currentRow, 1, new QTableWidgetItem(linkPath));
        }
    }

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(table);
    dialog->setLayout(layout);

    dialog->exec();
}

ProcessDetails MainWindow::getProcessDetails(int pid) {
    ProcessDetails details;
    //user
    QFile statusFile(QString("/proc/%1/status").arg(pid));
    QString uid = "";
    if (statusFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&statusFile);
        QString line;
        while (!in.atEnd()) {
            line = in.readLine();
            if (line.startsWith("Uid:")) {
                QStringList parts = line.split('\t');
                if (parts.size() > 1) {
                    uid = parts[1];
                    break;
                }
            }
        }
    }
    uid_t uidNum = uid.toInt();
    struct passwd *pw = getpwuid(uidNum);
    if (pw != nullptr) {
        details.user = QString::fromLocal8Bit(pw->pw_name);
    }
    statusFile.seek(0);
    statusFile.close();
    long smemory = 0;
    //state
    if (statusFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&statusFile);
        QString line = in.readLine();
        while (line != nullptr) {
            if (line.startsWith("Name:")) {
                details.name = line.split("\t").last();
            } else if (line.startsWith("State:")) {
                QString stateLine = line.split("\t").last();
                if (stateLine.startsWith("S")) {
                    details.state = "Sleeping";
                } else if (stateLine.startsWith("R")) {
                    details.state = "Running";
                } else {
                    details.state = "";
                }
            } else if (line.startsWith("VmRSS:")) {
                long bytes = line.simplified().split(" ")[1].toLong() * 1024;
                details.memory = bytesToMebibytesString(bytes);
                details.rmemory = details.memory;
            } else if (line.startsWith("VmSize:")) {
                long bytes = line.simplified().split(" ")[1].toLong()  * 1024;
                details.vmemory = bytesToMebibytesString(bytes);
            } else if (line.startsWith("RssFile:")) {
                long bytes = line.simplified().split(" ")[1].toLong()  * 1024;
                smemory += bytes;
            } else if (line.startsWith("RssShmem:")) {
                long bytes = line.simplified().split(" ")[1].toLong()  * 1024;
                smemory += bytes;
            } else if (line.startsWith("VmLib:")) {
                long bytes = line.simplified().split(" ")[1].toLong()  * 1024;
                smemory += bytes;
            }
            line = in.readLine();
        }
    }
    //cpu time and start time
    QFile statFile(QString("/proc/%1/stat").arg(pid));
    if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString line = statFile.readLine();
        QStringList parts = line.split(" ");
        if (parts.count() > 13) {
            long utime = parts[13].toLong();
            long stime = parts[14].toLong();
            long totalTime = (utime + stime) / sysconf(_SC_CLK_TCK);
            details.cpuTime = QString::number(totalTime);
        }
        long startTimeTicks = parts[21].toLong();
        QFile file("/proc/uptime");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Error Opening Uptime File";
        }

        QString data = file.readLine();
        long uptimeSeconds = data.split(" ")[0].toLong();
        long startTimeSeconds = uptimeSeconds - (startTimeTicks / sysconf(_SC_CLK_TCK));
        details.startTime = QDateTime::fromSecsSinceEpoch(startTimeSeconds + QDateTime::currentDateTime().toSecsSinceEpoch() - uptimeSeconds);
    }
    details.smemory = bytesToMebibytesString(smemory);
    return details;
}


void MainWindow::propertiesProcess(int pid) {
    ProcessDetails details = getProcessDetails(pid);

    QDialog *dialog = new QDialog(this);
    QFormLayout *layout = new QFormLayout(dialog);

    layout->addRow("Process Name:" , new QLabel(details.name));
    layout->addRow("User:", new QLabel(details.user));
    layout->addRow("State:", new QLabel(details.state));
    layout->addRow("Memory:", new QLabel(details.memory));
    layout->addRow("Virtual Memory:", new QLabel(details.vmemory));
    layout->addRow("Resident Memory:", new QLabel(details.rmemory));
    layout->addRow("Shared Memory:", new QLabel(details.smemory));
    layout->addRow("CPU Time:", new QLabel(details.cpuTime));
    layout->addRow("Start Time:", new QLabel(details.startTime.toString()));

    dialog->setLayout(layout);
    dialog->exec();
}














