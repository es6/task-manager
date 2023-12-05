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
    // Create a QTimer object
    cpuInfoTimer = new QTimer(this);

    // Set the interval to 1000 milliseconds (1 second)
    cpuInfoTimer->setInterval(1000);

    // Connect the timer's timeout signal to the updateCPUResourceInfo slot
    connect(cpuInfoTimer, &QTimer::timeout, this, &MainWindow::updateCPUResourceInfo);

    // Start the timer
    cpuInfoTimer->start();
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
}

MainWindow::~MainWindow()
{
    delete ui;
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
    while (line != nullptr && !endCPU) {
        if(!line.startsWith("cpu")) {
            endCPU = true;
        } else {
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
//                qDebug() << data[0] + ": "
//                         << result;
            }
            line = in.readLine();
        }
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

    QTableWidget *table = new QTableWidget(lines.size(), 6, dialog);
    table->setHorizontalHeaderLabels({"Filename", "VM Start", "VM End", "VM Size", "Flags", "VM Offset", "Private Clean", "Private Dirty", "Shared Clean," "Shared Dirty"});

    int row = 0;
    foreach (const QString &line, lines) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        qDebug() << "foreach: " << line;
        QStringList columns = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
        for (int col = 0; col < columns.size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(columns[col]);
            table->setItem(row, col, item);
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














