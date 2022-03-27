#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "tester.h"

#include <QMainWindow>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void onModeChanged(int);
    void onButtonOpenClicked();
    void statusTimerTicked();

private:
    bool isServer() const;
    bool isOpened() const;

    QGroupBox *groupConfig;
    QGroupBox *groupTest;
    QPushButton *buttonOpen;
    QPushButton *buttonRunTest;

    QComboBox *comboMode;
    QLineEdit *editServerAddress;
    QLineEdit *editServerPort;
    QLineEdit *editClientPort;
    QLineEdit *editPayloadBytes;
    QLineEdit *editBlockRate;

    QLineEdit *editPackets;
    QLineEdit *editConfirmedPackets;
    QLineEdit *editMissingPackets;
    QLineEdit *editCorruptedPackets;
    QLineEdit *editMaxLatency;
    QLineEdit *editAvgLatency;
    QLineEdit *editAvgBlockRate;

    Tester *tester{nullptr};
};
#endif // MAINWINDOW_H
