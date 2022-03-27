#include "mainwindow.h"

#include <QGridLayout>
#include <QLabel>
#include <QDebug>


namespace
{

void addListItem(QGridLayout* layout, const QString& label, QWidget* widget)
{
    auto labelWidget = new QLabel(label);
    auto row = layout->rowCount();
    layout->addWidget(labelWidget, row, 0, Qt::AlignRight);

    layout->addWidget(widget, row, 1);
}

}   // anonymous namespace



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto mainWidget = new QWidget(this);
    auto mainLayout = new QGridLayout(mainWidget);
    setCentralWidget(mainWidget);

    // Config group
    groupConfig = new QGroupBox(tr("Config"));
    auto layoutConfig = new QGridLayout(groupConfig);
    mainLayout->addWidget(groupConfig, 0, 0);

    comboMode = new QComboBox();
    comboMode->addItems(QStringList({tr("Client"), tr("Server")}));
    connect(comboMode, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onModeChanged);
    addListItem(layoutConfig, tr("Mode:"), comboMode);

    editServerAddress = new QLineEdit(tr("127.0.0.1"));
    addListItem(layoutConfig, tr("Server address:"), editServerAddress);

    editServerPort = new QLineEdit(tr("42178"));
    addListItem(layoutConfig, tr("Server port:"), editServerPort);

    editClientPort = new QLineEdit(tr("42179"));
    addListItem(layoutConfig, tr("Client port:"), editClientPort);

    editPayloadBytes = new QLineEdit(tr("64"));
    addListItem(layoutConfig, tr("Paylod bytes:"), editPayloadBytes);

    editBlockRate = new QLineEdit(tr("500"));
    addListItem(layoutConfig, tr("Block rate:"), editBlockRate);

    buttonOpen = new QPushButton(tr("Close"));
    connect(buttonOpen, &QPushButton::clicked, this, &MainWindow::onButtonOpenClicked);
    mainLayout->addWidget(buttonOpen, 0, 1);

    // Test group
    groupTest = new QGroupBox(tr("Test"));
    auto layoutTest = new QGridLayout(groupTest);
    mainLayout->addWidget(groupTest, 1, 0, 1, 2);

    editPackets = new QLineEdit(tr("0"));
    editPackets->setReadOnly(true);
    addListItem(layoutTest, tr("Packets:"), editPackets);

    editConfirmedPackets = new QLineEdit(tr("0"));
    editConfirmedPackets->setReadOnly(true);
    addListItem(layoutTest, tr("Confirmed packets:"), editConfirmedPackets);

    editMissingPackets = new QLineEdit(tr("0"));
    editMissingPackets->setReadOnly(true);
    addListItem(layoutTest, tr("Missing packets:"), editMissingPackets);

    editCorruptedPackets = new QLineEdit(tr("0"));
    editCorruptedPackets->setReadOnly(true);
    addListItem(layoutTest, tr("Corrupted packets:"), editCorruptedPackets);

    editMaxLatency = new QLineEdit(tr("0"));
    editMaxLatency->setReadOnly(true);
    addListItem(layoutTest, tr("Max latency:"), editMaxLatency);

    editAvgLatency = new QLineEdit(tr("0"));
    editAvgLatency->setReadOnly(true);
    addListItem(layoutTest, tr("Average latency:"), editAvgLatency);

    editAvgBlockRate = new QLineEdit(tr("0"));
    editAvgBlockRate->setReadOnly(true);
    addListItem(layoutTest, tr("Average block rate:"), editAvgBlockRate);

    auto statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::statusTimerTicked);
    statusTimer->start(1000);

    // Done
    onModeChanged(0);
    onButtonOpenClicked();
}

MainWindow::~MainWindow()
{
}

void MainWindow::onModeChanged(int)
{
    const bool server = isServer();
    editServerAddress->setEnabled(!server);
    editPayloadBytes->setEnabled(!server);
    editBlockRate->setEnabled(!server);
}

void MainWindow::onButtonOpenClicked()
{
    const bool opened = isOpened();
    groupConfig->setEnabled(opened);
    groupTest->setEnabled(!opened);

    if (tester != nullptr)
    {
        tester->deleteLater();
        tester = nullptr;
    }

    if (!opened)
    {
        QString serverAddress = (isServer())? QString() : editServerAddress->text();
        auto serverPort = static_cast<quint16>(editServerPort->text().toInt());
        auto clientPort = static_cast<quint16>(editClientPort->text().toInt());
        auto payloadBytes = static_cast<quint16>(editPayloadBytes->text().toInt());
        auto blockRate = editBlockRate->text().toFloat();
        tester = new Tester(this, serverAddress, serverPort, clientPort, payloadBytes, blockRate);
    }

    buttonOpen->setText((opened)? tr("Open") : tr("Close"));
}

void MainWindow::statusTimerTicked()
{
    if (tester != nullptr)
    {
        auto info = tester->getInfo();
        editPackets->setText(QString::number(info.allPackets));
        editConfirmedPackets->setText(QString::number(info.confirmedPackets));
        editMissingPackets->setText(QString::number(info.missingPackets));
        editCorruptedPackets->setText(QString::number(info.corruptedPackets));
        editMaxLatency->setText(QString::number(info.maxLatency));
        editAvgLatency->setText(QString::number(info.totalLatency / info.confirmedPackets));
        auto timeDiff = std::chrono::duration_cast<std::chrono::duration<double>>(info.lastTimestamp - info.firstTimestamp);
        editAvgBlockRate->setText(QString::number((info.allPackets - 1) / timeDiff.count()));
    }
}

bool MainWindow::isServer() const
{
    return comboMode->currentText().compare(tr("Server")) == 0;
}

bool MainWindow::isOpened() const
{
    return buttonOpen->text().compare(tr("Open")) != 0;
}
