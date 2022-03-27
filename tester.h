#ifndef TESTER_H
#define TESTER_H

#include <QUdpSocket>
#include <QTimer>

#include <map>
#include <mutex>
#include <chrono>
#include <string_view>
#include <random>


using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;
using HashResult = size_t;

struct TesterInfo
{
    size_t allPackets{0};
    size_t confirmedPackets{0};
    size_t missingPackets{0};
    size_t corruptedPackets{0};
    double totalLatency{0};
    double maxLatency{0};
    Timestamp firstTimestamp;
    Timestamp lastTimestamp;
};

class Tester : public QObject
{
public:
    Tester(QObject *parent, const QString& serverAddress, quint16 serverPort, quint16 clientPort, quint16 payloadBytes, double blockRateSeconds);

    TesterInfo getInfo() const;

private:
    void timerTicked();
    void socketRxAvailable();

private:
    struct PacketInfo
    {
        Timestamp timestamp;
        HashResult hash;
    };

    const bool isServer_;
    const QHostAddress serverAddress_;
    const quint16 serverPort_;
    const quint16 clientPort_;
    HashResult doHash(std::string_view data);

    QUdpSocket *socketRx{nullptr};
    QUdpSocket *socketTx{nullptr};
    QTimer *timer{nullptr};

    TesterInfo info_;
    mutable std::mutex mutexInfo_;

    std::map<Timestamp, PacketInfo> timestampLookup_;
    std::map<HashResult, PacketInfo> hashLookup_;
    std::string txBuffer_;
    std::string rxBuffer_;

    std::random_device randomDev_;
    std::mt19937 randomGen_;
    std::uniform_int_distribution<> randomDist_;
};


#endif // TESTER_H
