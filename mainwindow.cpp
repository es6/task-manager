#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegExp>
#include <filesystem>
#include <QStorageInfo>
#include <QDir>
#define GB (1024 * 1024 * 1024)



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    updateSystemInfo();
    updateProcesses();
}

MainWindow::~MainWindow()
{
    delete ui;
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
                    qDebug() << "line lol" << line;
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
