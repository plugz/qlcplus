/*
  Q Light Controller Plus
  artnetcontroller.cpp

  Copyright (c) Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "artnetcontroller.h"

#include <QMutexLocker>
#include <QDebug>

#define TRANSMIT_FULL    "Full"
#define TRANSMIT_PARTIAL "Partial"

ArtNetController::ArtNetController(QString ipaddr, QNetworkAddressEntry interface,
                                   QString macAddress, Type type, quint32 line, QObject *parent)
    : QObject(parent)
{
    m_ipAddr = QHostAddress(ipaddr);
    m_MACAddress = macAddress;
    m_line = line;

    if (m_ipAddr == QHostAddress::LocalHost)
        m_broadcastAddr = QHostAddress::LocalHost;
    else
        m_broadcastAddr = interface.broadcast();

    m_netmask = interface.netmask();

    qDebug() << "[ArtNetController] Broadcast address:" << m_broadcastAddr.toString() << "(MAC:" << m_MACAddress << ")";
    qDebug() << "[ArtNetController] type: " << type;
    m_packetizer.reset(new ArtNetPacketizer());
    m_packetSent = 0;
    m_packetReceived = 0;

    m_UdpSocket = new QUdpSocket(this);

    if (m_UdpSocket->bind(ARTNET_DEFAULT_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint) == false)
    {
        qWarning() << "[ArtNetController] failed to bind socket";
        return;
    }

    connect(m_UdpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingPackets()));

    // don't send a Poll if we're an input
    if (type == Output)
    {
        m_outPacket.reserve(512);
        m_packetizer->setupArtNetPoll(m_outPacket);
        qint64 sent = m_UdpSocket->writeDatagram(m_outPacket.data(), m_outPacket.size(),
                                                 m_broadcastAddr, ARTNET_DEFAULT_PORT);
        if (sent < 0)
        {
            qWarning() << "Unable to send initial Poll packet";
            qWarning() << "Errno: " << m_UdpSocket->error();
            qWarning() << "Errmgs: " << m_UdpSocket->errorString();
        }
        else
            m_packetSent++;
    }
}

ArtNetController::~ArtNetController()
{
    disconnect(m_UdpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingPackets()));
    m_UdpSocket->close();
    QMapIterator<int, QByteArray *> it(m_dmxValuesMap);
    while(it.hasNext())
    {
        it.next();
        QByteArray *ba = it.value();
        delete ba;
    }
    m_dmxValuesMap.clear();
}

ArtNetController::Type ArtNetController::type()
{
    int type = Unknown;
    foreach(UniverseInfo info, m_universeMap.values())
    {
        type |= info.type;
    }

    return Type(type);
}

quint32 ArtNetController::line()
{
    return m_line;
}

quint64 ArtNetController::getPacketSentNumber()
{
    return m_packetSent;
}

quint64 ArtNetController::getPacketReceivedNumber()
{
    return m_packetReceived;
}

QString ArtNetController::getNetworkIP()
{
    return m_ipAddr.toString();
}

QString ArtNetController::getNetmask()
{
    return m_netmask.toString();
}

QHash<QHostAddress, ArtNetNodeInfo> ArtNetController::getNodesList()
{
    return m_nodesList;
}

void ArtNetController::addUniverse(quint32 universe, ArtNetController::Type type)
{
    qDebug() << "[ArtNet] addUniverse - universe" << universe << ", type" << type;
    if (m_universeMap.contains(universe))
    {
        m_universeMap[universe].type |= (int)type;
    }
    else
    {
        UniverseInfo info;
        info.outputAddress = m_broadcastAddr;
        info.outputUniverse = universe;
        info.transmissionMode = Full;
        info.type = type;
        m_universeMap[universe] = info;
    }
}

void ArtNetController::removeUniverse(quint32 universe, ArtNetController::Type type)
{
    if (m_universeMap.contains(universe))
    {
        if (m_universeMap[universe].type == type)
            m_universeMap.take(universe);
        else
            m_universeMap[universe].type &= ~type;
    }
}

void ArtNetController::setOutputIPAddress(quint32 universe, QString address)
{
    if (m_universeMap.contains(universe) == false)
        return;

    QMutexLocker locker(&m_dataMutex);
    QStringList iFaceIP = m_ipAddr.toString().split(".");
    QStringList addList = address.split(".");

    for (int i = 0; i < addList.count(); i++)
        iFaceIP.replace(4 - addList.count() + i , addList.at(i));

    QString newIP = iFaceIP.join(".");
    qDebug() << "[setOutputIPAddress] transmit to IP: " << newIP;

    m_universeMap[universe].outputAddress = QHostAddress(newIP);
}

void ArtNetController::setOutputUniverse(quint32 universe, quint32 artnetUni)
{
    if (m_universeMap.contains(universe) == false)
        return;

    QMutexLocker locker(&m_dataMutex);
    m_universeMap[universe].outputUniverse = artnetUni;
}

void ArtNetController::setTransmissionMode(quint32 universe, ArtNetController::TransmissionMode mode)
{
    if (m_universeMap.contains(universe) == false)
        return;

    QMutexLocker locker(&m_dataMutex);
    m_universeMap[universe].transmissionMode = int(mode);
}

QString ArtNetController::transmissionModeToString(ArtNetController::TransmissionMode mode)
{
    switch (mode)
    {
        default:
        case Full:
            return QString(TRANSMIT_FULL);
        break;
        case Partial:
            return QString(TRANSMIT_PARTIAL);
        break;
    }
}

ArtNetController::TransmissionMode ArtNetController::stringToTransmissionMode(const QString &mode)
{
    if (mode == QString(TRANSMIT_PARTIAL))
        return Partial;
    else
        return Full;
}

QList<quint32> ArtNetController::universesList()
{
    QList<quint32> universesList(m_universeMap.keys());
    qSort(universesList);
    return universesList;
}

UniverseInfo *ArtNetController::getUniverseInfo(quint32 universe)
{
    if (m_universeMap.contains(universe))
        return &m_universeMap[universe];

    return NULL;
}

void ArtNetController::sendDmx(const quint32 universe, const QByteArray &data)
{
    QMutexLocker locker(&m_dataMutex);
    QHostAddress outAddress = m_broadcastAddr;
    quint32 outUniverse = universe;
    TransmissionMode transmitMode = Full;

    QHash<quint32, UniverseInfo>::const_iterator uniIt(m_universeMap.constFind(universe));
    if (uniIt != m_universeMap.constEnd())
    {
        UniverseInfo const& info = *uniIt;
        outAddress = info.outputAddress;
        outUniverse = info.outputUniverse;
        transmitMode = TransmissionMode(info.transmissionMode);
    }

    m_packetizer->setupArtNetDmx(m_outPacket, outUniverse, data, transmitMode == Full);

    qint64 sent = m_UdpSocket->writeDatagram(m_outPacket.data(), m_outPacket.size(),
                                             outAddress, ARTNET_DEFAULT_PORT);
    if (sent < 0)
    {
        qDebug() << "sendDmx failed";
        qDebug() << "Errno: " << m_UdpSocket->error();
        qDebug() << "Errmgs: " << m_UdpSocket->errorString();
    }
    else
        m_packetSent++;
}

void ArtNetController::processPendingPackets()
{
    while (m_UdpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        QHostAddress senderAddress;
        datagram.resize(m_UdpSocket->pendingDatagramSize());
        m_UdpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddress);
        //if (senderAddress != m_ipAddr)
        {
            //qDebug() << "Received packet with size: " << datagram.size() << ", host: " << senderAddress.toString();
            int opCode = -1;
            if (m_packetizer->checkPacketAndCode(datagram, opCode) == true)
            {
                m_packetReceived++;
                switch (opCode)
                {
                    case ARTNET_POLLREPLY:
                    {
                        qDebug() << "[ArtNet] ArtPollReply received";
                        ArtNetNodeInfo newNode;
                        if (m_packetizer->fillArtPollReplyInfo(datagram, newNode) == true)
                        {
                            if (m_nodesList.contains(senderAddress) == false)
                                m_nodesList[senderAddress] = newNode;
                        }
                    }
                    break;
                    case ARTNET_POLL:
                    {
                        qDebug() << "[ArtNet] ArtPoll received";
                        QByteArray pollReplyPacket;
                        m_packetizer->setupArtNetPollReply(pollReplyPacket, m_ipAddr, m_MACAddress);
                        m_UdpSocket->writeDatagram(pollReplyPacket.data(), pollReplyPacket.size(),
                                                   senderAddress, ARTNET_DEFAULT_PORT);
                        m_packetSent++;
                    }
                    break;
                    case ARTNET_DMX:
                    {
                        QByteArray dmxData;
                        quint32 universe;
                        if (this->type() == Input)
                        {
                            if (m_packetizer->fillDMXdata(datagram, dmxData, universe) == true)
                            {
                                qDebug() << "[ArtNet] DMX data received. Universe:" << universe << "Data size:" << dmxData.size();

                                QByteArray *dmxValues;
                                if (m_dmxValuesMap.contains(universe) == false)
                                    m_dmxValuesMap[universe] = new QByteArray(512, 0);
                                dmxValues = m_dmxValuesMap[universe];

                                //quint32 emitStartAddr = UINT_MAX;
                                for (int i = 0; i < dmxData.length(); i++)
                                {
                                    if (dmxValues->at(i) != dmxData.at(i))
                                    {
                                        dmxValues->replace(i, 1, (const char *)(dmxData.data() + i), 1);
                                        //if (emitStartAddr == UINT_MAX)
                                        //    emitStartAddr = (quint32)i;
                                        emit valueChanged(universe, m_line, i, (uchar)dmxData.at(i));
                                    }
                                    /*
                                    else
                                    {
                                        if (emitStartAddr != UINT_MAX)
                                        {
                                            // SIGNAL is: void valuesChanged(quint32 input, quint32 startChannel, QByteArray &values);
                                            emit valuesChanged(universe, emitStartAddr, m_dmxValues.mid(emitStartAddr, i - emitStartAddr));
                                        }
                                    }
                                    */
                                }
                            }
                        }
                    }
                    break;
                    default:
                        qDebug() << "[ArtNet] opCode not supported yet (" << opCode << ")";
                    break;
                }
            }
            else
                qDebug() << "[ArtNet] Malformed packet received";
        }
     }
}
