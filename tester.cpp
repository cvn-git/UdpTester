#include "tester.h"

#include <QDebug>


Tester::Tester(QObject *parent, const QString& serverAddress, quint16 serverPort, quint16 clientPort, quint16 payloadBytes, double blockRateSeconds)
    : QObject(parent)
    , isServer_(serverAddress.isEmpty())
    , serverAddress_(serverAddress)
    , serverPort_(serverPort)
    , clientPort_(clientPort)
    , txBuffer_(payloadBytes, ' ')
    , rxBuffer_(payloadBytes * 2, ' ')
    , randomGen_(randomDev_())
    , randomDist_(std::numeric_limits<char>::min(), std::numeric_limits<char>::max())
{
    qDebug() << "Create socket";
    socket = new QUdpSocket(this);
    quint16 port = (isServer_)? serverPort : clientPort;
    qDebug() << "Bind to port " << port;
    socket->bind(port);
    connect(socket, &QUdpSocket::readyRead, this, &Tester::socketRxAvailable);

    if (!isServer_)
    {
        qDebug() << "Start timer";
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Tester::timerTicked);
        timer->start(static_cast<int>(std::round(1000 / blockRateSeconds)));
    }
}

TesterInfo Tester::getInfo() const
{
    std::lock_guard<std::mutex> lock(mutexInfo_);
    return info_;
}

void Tester::timerTicked()
{
    // Randomize a buffer
    for (size_t k = 0; k < txBuffer_.size(); k++)
    {
        txBuffer_[k] = static_cast<char>(randomDist_(randomGen_));
    }

    PacketInfo packetInfo;
    packetInfo.timestamp = std::chrono::steady_clock::now();
    packetInfo.hash = doHash(txBuffer_);
    //qDebug() << "Tx Hash " << packetInfo.hash;
    timestampLookup_.emplace(packetInfo.timestamp, packetInfo);
    hashLookup_.emplace(packetInfo.hash, packetInfo);

    // Send packet
    socket->writeDatagram(txBuffer_.data(), txBuffer_.size(), serverAddress_, serverPort_);
    {
        std::lock_guard<std::mutex> lock(mutexInfo_);
        if (info_.allPackets == 0)
        {
            info_.firstTimestamp = packetInfo.timestamp;
        }
        info_.lastTimestamp = packetInfo.timestamp;
        info_.allPackets += 1;
    }

    // Check for expired packets
    size_t numExpired = 0;
    auto it = timestampLookup_.begin();
    while (it != timestampLookup_.end())
    {
        auto timeDiff = std::chrono::duration_cast<std::chrono::duration<double>>(packetInfo.timestamp - it->second.timestamp);
        if (timeDiff.count() < 3.0)
        {
            break;
        }
        numExpired++;

        auto it2 = hashLookup_.find(it->second.hash);
        if (it2 != hashLookup_.end())
        {
            hashLookup_.erase(it2);
        }

        it = timestampLookup_.erase(it);
    }

    if (numExpired > 0)
    {
        std::lock_guard<std::mutex> lock(mutexInfo_);
        info_.missingPackets += numExpired;
    }
}

void Tester::socketRxAvailable()
{
    while (socket->hasPendingDatagrams())
    {
        QHostAddress address;
        quint16 port = 0;
        auto numBytes = socket->readDatagram(rxBuffer_.data(), rxBuffer_.size(), &address, &port);
        //qDebug() << "Receive " << numBytes << " bytes from " << address << ":" << port;
        if (numBytes == 0)
        {
            continue;
        }

        auto rxTimestamp = std::chrono::steady_clock::now();
        if (isServer_)
        {
            // Loopback
            socket->writeDatagram(rxBuffer_.data(), numBytes, address, clientPort_);

            std::lock_guard<std::mutex> lock(mutexInfo_);
            if (info_.allPackets == 0)
            {
                info_.firstTimestamp = rxTimestamp;
            }
            info_.lastTimestamp = rxTimestamp;
            info_.allPackets += 1;
        }
        else
        {
            if (numBytes != txBuffer_.size())
            {
                std::lock_guard<std::mutex> lock(mutexInfo_);
                info_.corruptedPackets += 1;
                continue;
            }

            auto hash = doHash(std::string_view(rxBuffer_.data(), numBytes));
            //qDebug() << "Rx Hash " << hash;
            auto it1 = hashLookup_.find(hash);
            if (it1 == hashLookup_.end())
            {
                std::lock_guard<std::mutex> lock(mutexInfo_);
                info_.corruptedPackets += 1;
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(mutexInfo_);
                info_.confirmedPackets += 1;
                auto timeDiff = std::chrono::duration_cast<std::chrono::duration<double>>(rxTimestamp - it1->second.timestamp);
                info_.totalLatency += timeDiff.count();
                info_.maxLatency = std::max(info_.maxLatency, timeDiff.count());
            }

            auto it2 = timestampLookup_.find(it1->second.timestamp);
            if (it2 != timestampLookup_.end())
            {
                timestampLookup_.erase(it2);
            }
            hashLookup_.erase(it1);
        }
    }
}

HashResult Tester::doHash(std::string_view data)
{
    return std::hash<std::string_view>{}(data);
}
