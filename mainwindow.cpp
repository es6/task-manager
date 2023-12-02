#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegExp>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    updateSystemInfo();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateSystemInfo() {
    qDebug() << "hi";
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
        qDebug() << memInfo;
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

    file.setFileName("/proc/diskstats");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //        in.setDevice(&file);
        //        info = "Disk Stats: ";
        //        printAll(info, in);
        file.close();
    }
}

void MainWindow::printAll(QString info, QTextStream &in) {
    QString line = in.readLine();
    while (line != nullptr) {
        ui->listWidget->addItem(info + line);
        line = in.readLine();
    }
}
