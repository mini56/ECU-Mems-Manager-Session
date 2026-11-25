#ifndef MEMS3PROTOCOL_H
#define MEMS3PROTOCOL_H

// Independent MEMS 3 9600 8E1 diagnostic session.
// Reference: archived RoverMEMS i386 ecu-3.go (James Portman, GPLv3).
// This module never sends the legacy ROSCO or MEMS 2J frame format.
#include <QByteArray>
#include <QElapsedTimer>
#include <QSerialPort>
#include "ecucommunicationtraceio.h"
#include <QStringList>
#include <QVariantMap>
#include <QtGlobal>
#include <functional>
#include "mems2jprotocol.h" // Proven seed/key function only.

namespace Mems3Protocol {
inline quint8 byte(const QByteArray &data, int index)
{ return quint8(static_cast<unsigned char>(data.at(index))); }
inline quint16 word(const QByteArray &data, int index)
{ return quint16((quint16(byte(data,index)) << 8) | byte(data,index+1)); }

// The header identifies requests; ECU responses may have a different header.
inline QByteArray frame(const QByteArray &payload)
{
    if (payload.isEmpty() || payload.size()>96) return QByteArray();
    QByteArray result=QByteArray::fromHex("b813f7");
    result.append(char(payload.size()));
    result.append(payload);
    quint8 checksum=0;
    for (char value:result) checksum ^= quint8(static_cast<unsigned char>(value));
    result.append(char(checksum));
    return result;
}
inline bool unpack(const QByteArray &packet, QByteArray *payload)
{
    if (packet.size()<6) return false;
    const int count=byte(packet,3);
    if (count<1 || count>96 || packet.size()!=count+5) return false;
    quint8 checksum=0;
    for (char value:packet) checksum ^= quint8(static_cast<unsigned char>(value));
    if (checksum!=0) return false;
    if (payload) *payload=packet.mid(4,count);
    return true;
}
inline bool decodeMeasurement(quint8 id, const QByteArray &p, QVariantMap *values)
{
    if (!values || p.size()<2 || byte(p,0)!=0x61 || byte(p,1)!=id) return false;
    auto set=[values](const char *name,double value,unsigned received) {
        const QString key=QString::fromLatin1(name);
        values->insert(key,value);
        QVariantMap raw=values->value(QStringLiteral("_received")).toMap();
        raw.insert(key,received);
        values->insert(QStringLiteral("_received"),raw);
    };
    switch(id)
    {
    case 0x00:
        if (p.size()<12) return false;
        set("coolant_c",(double(word(p,2))-2730.0)/10.0,word(p,2));
        set("oil_c",(double(word(p,6))-2730.0)/10.0,word(p,6));
        set("intake_c",(double(word(p,10))-2730.0)/10.0,word(p,10));
        break;
    case 0x06:
        if (p.size()<12) return false;
        set("map_kpa",double(word(p,2))/100.0,word(p,2));
        set("throttle_mv",word(p,8),word(p,8)); // Millivolts per archived source.
        set("rpm",word(p,10),word(p,10));
        break;
    case 0x0A:
        if (p.size()<6) return false;
        set("lambda_feedback_percent",double(word(p,2))/100.0,word(p,2));
        set("lambda_mv",word(p,4),word(p,4));
        break;
    case 0x0B:
        if (p.size()<6) return false;
        set("coil1_us",word(p,2),word(p,2)); set("coil2_us",word(p,4),word(p,4));
        break;
    case 0x21:
        if (p.size()<4) return false;
        set("rpm_deviation_raw",word(p,2),word(p,2)); // Sign/units not established.
        break;
    default: return false;
    }
    return true;
}
inline QString faultDescription(quint16 id)
{
    switch(id) {
    case 0x1232: return QStringLiteral("Relais pompe carburant, circuit ouvert");
    case 0x0650: return QStringLiteral("Circuit temoin moteur");
    case 0x0481: return QStringLiteral("Ventilateur condenseur climatisation");
    case 0x1508: return QStringLiteral("Commande moteur pas a pas, circuit ouvert");
    case 0x1186: case 0x1185: return QStringLiteral("Chauffage sonde lambda amont");
    case 0x1192: return QStringLiteral("Chauffage sonde lambda aval");
    case 0x0445: return QStringLiteral("Commande purge canister");
    case 0x0480: return QStringLiteral("Ventilateur refroidissement");
    case 0x1610: return QStringLiteral("Relais principal, circuit ouvert");
    case 0x0113: return QStringLiteral("Sonde air admission, court-circuit");
    case 0x0118: return QStringLiteral("Sonde liquide, court-circuit");
    case 0x0122: return QStringLiteral("Capteur papillon, court-circuit");
    case 0x0562: return QStringLiteral("Tension alimentation");
    case 0x0197: return QStringLiteral("Sonde huile, court-circuit");
    case 0x0462: return QStringLiteral("Niveau carburant, court-circuit masse");
    case 0x0340: return QStringLiteral("Capteur position arbre a cames");
    case 0x0106: return QStringLiteral("Pression admission incorrecte");
    case 0x1316: return QStringLiteral("Rates combustion / emissions");
    case 0x0170: return QStringLiteral("Systeme carburant");
    case 0x0655: return QStringLiteral("Voyant temperature compartiment moteur, circuit ouvert");
    default: return QStringLiteral("Defaut non catalogue");
    }
}
inline QString faultState(quint8 state)
{
    switch(state) {
    case 0x20: return QStringLiteral("historique");
    case 0x74: case 0x58: case 0x71: return QStringLiteral("present, test incomplet");
    case 0x30: return QStringLiteral("historique, test incomplet");
    case 0x61: case 0x62: case 0x64: return QStringLiteral("present");
    default: return QStringLiteral("etat non catalogue");
    }
}
inline QStringList decodeFaults(const QByteArray &p)
{
    QStringList faults;
    if (p.size()<2 || byte(p,0)!=0x58) return faults;
    // RoverMEMS skips the two first bytes (58 + subfunction/status).
    // Retain the raw response separately when unexpected trailing data occur.
    for(int i=2;i+2<p.size();i+=3) {
        const quint16 id=word(p,i);
        faults.append(QStringLiteral("%1 : %2 (%3)")
                      .arg(QStringLiteral("%1").arg(id,4,16,QChar('0')).toUpper(),
                           faultDescription(id),faultState(byte(p,i+2))));
    }
    return faults;
}

class Session {
public:
    bool connectTo(const QString &portName,const std::function<bool()> &cancelled)
    {
        close();
        m_serial.setPortName(portName);
        if (!m_serial.setBaudRate(9600) ||
            !m_serial.setDataBits(QSerialPort::Data8) ||
            !m_serial.setParity(QSerialPort::EvenParity) ||
            !m_serial.setStopBits(QSerialPort::OneStop) ||
            !m_serial.setFlowControl(QSerialPort::NoFlowControl) ||
            !m_serial.open(QIODevice::ReadWrite)) { close(); return false; }
        m_serial.clear(QSerialPort::AllDirections);
        QByteArray reply;
        if (cancelled() ||
            exchange(QByteArray::fromHex("1a9a"),QByteArray::fromHex("5a9a"),
                     1100,cancelled,&reply)!=Positive ||
            exchange(QByteArray::fromHex("10a0"),QByteArray::fromHex("50"),
                     900,cancelled,&reply)!=Positive ||
            exchange(QByteArray::fromHex("2701"),QByteArray::fromHex("6701"),
                     900,cancelled,&reply)!=Positive ||
            reply.size()<4) { close(); return false; }
        const quint16 seed=word(reply,2);
        if(seed!=0) {
            const quint16 key=Mems2JProtocol::generateKey(seed);
            QByteArray response=QByteArray::fromHex("2702");
            response.append(char(key>>8));response.append(char(key&0xff));
            if (exchange(response,QByteArray::fromHex("6702"),900,cancelled,&reply)!=Positive)
            { close();return false; }
        }
        // Wake-up, diagnostic session and seed exchange are real ECU replies.
        // Optional ping or individual sensor refusal cannot undo this proof.
        exchange(QByteArray::fromHex("3e"),QByteArray::fromHex("7e"),
                 350,cancelled,&reply);
        if(cancelled()) { close();return false; }
        m_ready=true;
        return true;
    }

    bool readCycle(const std::function<bool()> &cancelled)
    {
        if (!isOpen() || cancelled()) return false;
        const quint8 ids[]={0xff,0x00,0x06,0x0a,0x0b,0x21};
        int good=0;
        for(quint8 id:ids) {
            if(cancelled())return false;
            const QByteArray request=id==0xff
                ? QByteArray::fromHex("18000000")
                : QByteArray::fromHex("21")+QByteArray(1,char(id));
            const QByteArray positive=id==0xff
                ? QByteArray::fromHex("58")
                : QByteArray::fromHex("61")+QByteArray(1,char(id));
            QByteArray reply;
            if(exchange(request,positive,450,cancelled,&reply)!=Positive)
                continue;
            if(id==0xff) {
                m_values.insert(QStringLiteral("faults_raw"),
                    QString::fromLatin1(reply.toHex(' ').toUpper()));
                m_faults=decodeFaults(reply);++good;
            } else if(decodeMeasurement(id,reply,&m_values)) ++good;
        }
        return good>0;
    }

    // Only invoked by an explicit user clear-fault action, never by connect.
    bool clearFaults(const std::function<bool()> &cancelled)
    {
        if (!isOpen() || cancelled()) return false;
        QByteArray reply;
        return exchange(QByteArray::fromHex("140000"),QByteArray::fromHex("540000"),
                        1000,cancelled,&reply)==Positive;
    }

    bool isOpen() const {return m_ready && m_serial.isOpen();}
    QVariantMap values() const {return m_values;}
    QStringList faults() const {return m_faults;}
    void close()
    {
        m_ready=false;
        if(m_serial.isOpen())m_serial.close();
        m_receive.clear();m_lastSent.clear();m_values.clear();m_faults.clear();
    }

private:
    qint64 traceWrite(const QByteArray &bytes)
    {
        const qint64 n=m_serial.write(bytes);
        if(n>0)ecu_trace_bytes("TX",(const uint8_t*)bytes.constData(),(size_t)n);
        return n;
    }
    QByteArray traceReadAll()
    {
        const QByteArray result=m_serial.readAll();
        if(!result.isEmpty())ecu_trace_bytes("RX",(const uint8_t*)result.constData(),(size_t)result.size());
        return result;
    }
    enum Status { Timeout,Positive,Negative };
    Status exchange(const QByteArray &command,const QByteArray &expected,
                    int timeout,const std::function<bool()> &cancelled,QByteArray *reply)
    {
        m_lastSent=frame(command);
        if(m_lastSent.isEmpty() ||
           traceWrite(m_lastSent)!=m_lastSent.size() ||
           !m_serial.waitForBytesWritten(400))return Timeout;
        QElapsedTimer timer;timer.start();
        while(timer.elapsed()<timeout && !cancelled()) {
            while(!m_receive.isEmpty()) {
                // Some K-Line adapters echo only the raw 1A 9A wake-up bytes.
                if(command==QByteArray::fromHex("1a9a") &&
                   m_receive.startsWith(QByteArray::fromHex("1a9a"))) {
                    m_receive.remove(0,2);continue;
                }
                if(command==QByteArray::fromHex("1a9a") &&
                   m_receive==QByteArray(1,char(0x1a))) break;
                if(!m_lastSent.isEmpty() && m_receive.startsWith(m_lastSent)) {
                    m_receive.remove(0,m_lastSent.size());continue;
                }
                if(!m_lastSent.isEmpty() && m_lastSent.startsWith(m_receive))
                    break; // Fragmented local echo.
                if(m_receive.size()<5)break;
                const int count=byte(m_receive,3);
                if(count<1 || count>96){m_receive.remove(0,1);continue;}
                if(m_receive.size()<count+5)break;
                const QByteArray packet=m_receive.left(count+5);
                QByteArray payload;
                if(!unpack(packet,&payload)){m_receive.remove(0,1);continue;}
                m_receive.remove(0,count+5);
                if(packet==m_lastSent)continue;
                if(payload.startsWith(expected)) {
                    if(reply)*reply=payload;
                    return Positive;
                }
                if(payload.size()>=3 && byte(payload,0)==0x7f &&
                   byte(payload,1)==byte(command,0)) {
                    if(byte(payload,2)==0x78)continue; // Response pending.
                    if(reply)*reply=payload;
                    return Negative;
                }
            }
            const int remaining=timeout-int(timer.elapsed());
            if(remaining<=0)break;
            m_serial.waitForReadyRead(qMin(60,remaining));
            m_receive.append(traceReadAll());
            if(!m_serial.isOpen() ||
               m_serial.error()==QSerialPort::ResourceError ||
               m_serial.error()==QSerialPort::DeviceNotFoundError)break;
        }
        return Timeout;
    }
    QSerialPort m_serial;
    QByteArray m_receive,m_lastSent;
    QVariantMap m_values;
    QStringList m_faults;
    bool m_ready=false;
};
} // namespace Mems3Protocol
#endif // MEMS3PROTOCOL_H
