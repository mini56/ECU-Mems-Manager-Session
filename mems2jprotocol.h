#ifndef MEMS2JPROTOCOL_H
#define MEMS2JPROTOCOL_H

// MEMS 2J read-only K-Line transport. Based on the already archived RoverMEMS
// sources (James Portman, GPLv3), research/mems/lecture-ecu-19-2j-3/.
// Independent of librosco: do not feed MEMS 2J replies to ROSCO 0x80/0x7D.
#include <QByteArray>
#include <QElapsedTimer>
#include <QSerialPort>
#include <QStringList>
#include <QThread>
#include <QVariantMap>
#include <QtGlobal>
#include <functional>

namespace Mems2JProtocol {

inline quint8 byte(const QByteArray &s, int i)
{
    return quint8(static_cast<unsigned char>(s.at(i)));
}
inline quint16 u16(const QByteArray &s, int i)
{
    return quint16((quint16(byte(s, i)) << 8) | byte(s, i + 1));
}
inline QByteArray frame(const QByteArray &payload)
{
    if (payload.isEmpty() || payload.size() > 96)
        return QByteArray();
    QByteArray result;
    result.append(char(payload.size()));
    result.append(payload);
    unsigned int checksum = 0;
    for (char ch : result)
        checksum += quint8(static_cast<unsigned char>(ch));
    result.append(char(checksum & 0xFFu));
    return result;
}
inline bool unpack(const QByteArray &packet, QByteArray *payload)
{
    if (packet.size() < 3)
        return false;
    const int size = byte(packet, 0);
    if (size < 1 || size > 96 || packet.size() != size + 2)
        return false;
    unsigned int checksum = 0;
    for (int i = 0; i <= size; ++i)
        checksum += byte(packet, i);
    if ((checksum & 0xFFu) != byte(packet, size + 1))
        return false;
    if (payload)
        *payload = packet.mid(1, size);
    return true;
}
inline quint16 generateKey(quint16 seed)
{
    // Portage exact de rovermems-reference/ecu-auth.go.
    const auto bit = [](quint16 v, int n) { return (v >> n) & 1u; };
    int loops = 1 + (bit(seed,15) ? 8 : 0) + (bit(seed,7) ? 4 : 0)
                  + (bit(seed,4) ? 2 : 0) + (bit(seed,0) ? 1 : 0);
    quint16 key = 0;
    while (loops-- > 0)
    {
        key = quint16(seed >> 1);
        if (bit(seed,13) && bit(seed,3))
            key = quint16(key & 0xFFFEu);
        else
            key = quint16(key | 0x0001u);
        if (bit(seed,9) ^ bit(seed,8) ^ bit(seed,2) ^ bit(seed,1))
            key = quint16(key | 0x8000u);
        seed = key;
    }
    return key;
}

// 21 19 bit names are documented for Mini MPi ONLY, not proven universal
// on VVC/other MEMS 2J. Keep this qualification in the UI.
inline QStringList decodeFaults(const QByteArray &p)
{
    QStringList result;
    if (p.size() < 2 || byte(p,0) != 0x61 || byte(p,1) != 0x19)
        return result;
    struct BitLabel { int index; quint8 mask; const char *text; };
    static const BitLabel bits[] = {
        {4,0x40,"Température extérieure : tension basse"},
        {4,0x20,"Alimentation : tension basse"},
        {4,0x10,"Température huile : tension basse"},
        {4,0x04,"Température liquide : tension basse"},
        {4,0x01,"Système : tension basse"},
        {5,0x80,"Batterie : tension basse"},
        {5,0x10,"Lambda banc 1 : tension basse"},
        {5,0x04,"Papillon : tension basse"},
        {5,0x02,"Air admission : tension basse"},
        {5,0x01,"MAP : tension basse"},
        {8,0x40,"Température extérieure : tension haute"},
        {8,0x20,"Alimentation : tension haute"},
        {8,0x10,"Température huile : tension haute"},
        {8,0x04,"Température liquide : tension haute"},
        {8,0x01,"Système : tension haute"},
        {9,0x80,"Batterie : tension haute"},
        {9,0x10,"Lambda banc 1 : tension haute"},
        {9,0x04,"Papillon : tension haute"},
        {9,0x02,"Air admission : tension haute"},
        {9,0x01,"MAP : tension haute"},
        {12,0x40,"Température extérieure : présent"},
        {12,0x20,"Alimentation : présent"},
        {12,0x10,"Température huile : présent"},
        // bit2 index 12 is ambiguous in the original Go source.
        {12,0x04,"Bit 12.2 : liquide / tension système (ambigu, non attribué)"},
        {13,0x80,"Batterie : présent"},
        {13,0x10,"Lambda banc 1 : présent"},
        {13,0x04,"Papillon : présent"},
        {13,0x02,"Air admission : présent"},
        {13,0x01,"MAP : présent"},
        {23,0x08,"MAP : présent (second groupe)"},
        {23,0x04,"Température huile : présent (second groupe)"},
        {23,0x02,"Air admission : présent (second groupe)"},
        {23,0x01,"Température liquide : présent (second groupe)"},
        {25,0x08,"MAP : historique"},
        {25,0x04,"Température huile : historique"},
        {25,0x02,"Air admission : historique"},
        {25,0x01,"Température liquide : historique"},
        {26,0x01,"Vitesse route : présent"},
        {26,0x02,"Communication boîte auto : présent"},
        {26,0x10,"Correction carburant banc 1 : présent"},
        {26,0x20,"Correction carburant banc 2 : présent"},
        {28,0x01,"Vitesse route : historique"},
        {28,0x02,"Communication boîte auto : historique"},
        {28,0x10,"Correction carburant : historique"}
    };
    for (const BitLabel &b : bits)
        if (p.size() > b.index && (byte(p,b.index) & b.mask))
            result.append(QString::fromUtf8(b.text));
    return result;
}

inline bool decodeMeasurement(quint8 id, const QByteArray &p, QVariantMap *out)
{
    if (!out || p.size() < 2 || byte(p,0)!=0x61 || byte(p,1)!=id)
        return false;
    const auto set = [out](const char *name, double n) {
        out->insert(QString::fromLatin1(name), n);
    };
    switch (id)
    {
    case 0x00: case 0x06:
        // RoverMEMS knows these packets but does not assign proven units.
        // Retain exact bytes instead of inventing sensor conversions.
        out->insert(QStringLiteral("data_%1_raw").arg(id,2,16,QChar('0')).toUpper(),
                    QString::fromLatin1(p.toHex(' ').toUpper()));
        return true;
    case 0x01: case 0x02: case 0x03:
        if (p.size()<4) return false;
        set(id==0x01?"coolant_c":id==0x02?"oil_c":"intake_c",
            (double(u16(p,2))-2732.0)/10.0);
        return true;
    case 0x05: if (p.size()<4) return false; set("fuel_raw",u16(p,2)); return true;
    case 0x07: if (p.size()<4) return false; set("map_kpa",u16(p,2)/100.0); return true;
    case 0x08: if (p.size()<4) return false; set("throttle_deg",u16(p,2)/100.0); return true;
    case 0x09: if (p.size()<4) return false; set("rpm",u16(p,2)); return true;
    case 0x0A:
        if (p.size()<6) return false;
        set("lambda_feedback_percent",u16(p,2)/100.0);
        set("lambda_o2_raw",u16(p,4)); // source Go: unit not independently established
        return true;
    case 0x0B:
        if (p.size()<4) return false;
        set("coil1_raw",byte(p,2)); set("coil2_raw",byte(p,3));
        return true;
    case 0x0C:
        if (p.size()<6) return false;
        for(int i=0;i<4;++i)
            out->insert(QStringLiteral("injector%1_raw").arg(i+1),int(byte(p,i+2)));
        return true;
    case 0x0D: if (p.size()<3) return false; set("speed_raw",byte(p,2)); return true;
    case 0x0F:
        if (p.size()<3) return false;
        set("throttle_closed",(byte(p,2)&1u)?1:0);
        set("ignition_on",(byte(p,2)&2u)?1:0);
        set("ac_requested",(byte(p,2)&8u)?1:0);
        return true;
    case 0x10: if (p.size()<6) return false; set("battery_v",u16(p,4)/1000.0); return true;
    case 0x11:
        if (p.size()<3) return false;
        set("primary_sync",(byte(p,2)&1u)?0:1);
        set("secondary_sync",(byte(p,2)&2u)?0:1);
        return true;
    case 0x12: if (p.size()<4) return false; set("idle_steps",u16(p,2)/2.0); return true;
    case 0x13: if (p.size()<3) return false; set("closed_loop",(byte(p,2)&1u)?1:0); return true;
    case 0x21:
        if (p.size()<4) return false;
        set("rpm_error",int(qint16(u16(p,2)))); // signed 2's-complement; fixes Go boundary bug
        return true;
    case 0x25: if (p.size()<4) return false; set("cam_raw",u16(p,2)); return true;
    case 0x3A:
        if (p.size()<6) return false;
        set("idle_offset_raw10",u16(p,2)/10.0);
        set("idle_target_rpm",u16(p,4));
        return true;
    default:
        return false;
    }
}

class Session
{
public:
    bool connectTo(const QString &portName, const std::function<bool()> &cancelled)
    {
        close();
        m_serial.setPortName(portName);
        if (!m_serial.setBaudRate(10400) ||
            !m_serial.setDataBits(QSerialPort::Data8) ||
            !m_serial.setParity(QSerialPort::NoParity) ||
            !m_serial.setStopBits(QSerialPort::OneStop) ||
            !m_serial.setFlowControl(QSerialPort::NoFlowControl) ||
            !m_serial.open(QIODevice::ReadWrite))
            return false;
        m_serial.clear(QSerialPort::AllDirections);
        if (!m_serial.setBreakEnabled(false)) { close(); return false; }
        QThread::msleep(200);
        if (cancelled()) { close(); return false; }
        if (!m_serial.setBreakEnabled(true)) { close(); return false; }
        QThread::msleep(25);
        if (!m_serial.setBreakEnabled(false)) { close(); return false; }
        QThread::msleep(75); // 25 ms high + 50 ms extra from archived RoverMEMS
        if (cancelled()) { close(); return false; }
        m_lastSent = QByteArray::fromHex("8113f7810c");
        if (m_serial.write(m_lastSent) != m_lastSent.size() ||
            !m_serial.waitForBytesWritten(400))
        { close(); return false; }
        QByteArray reply;
        if (receive(QByteArray::fromHex("c1d58f"),0x81,1200,cancelled,&reply) != Positive ||
            exchange(QByteArray::fromHex("10a0"),QByteArray::fromHex("50"),900,cancelled,&reply) != Positive ||
            exchange(QByteArray::fromHex("2701"),QByteArray::fromHex("6701"),900,cancelled,&reply) != Positive ||
            reply.size()<4)
        { close(); return false; }
        const quint16 seed = u16(reply,2);
        const quint16 key = seed ? generateKey(seed) : 0;
        QByteArray sendKey = QByteArray::fromHex("2702");
        sendKey.append(char(key>>8)); sendKey.append(char(key&0xFFu));
        if (exchange(sendKey,QByteArray::fromHex("6702"),900,cancelled,&reply) != Positive)
        { close(); return false; }

        // The ECU has already confirmed wake-up, session, seed and key.
        // Some 2J variants can reject an individual sensor such as 21 09;
        // it must not invalidate a successfully established ECU session.
        exchange(QByteArray::fromHex("3e"),QByteArray::fromHex("7e"),350,cancelled,&reply);
        if (cancelled()) { close(); return false; }
        if (exchange(QByteArray::fromHex("2109"),QByteArray::fromHex("6109"),
                     900,cancelled,&reply) == Positive)
            decodeMeasurement(0x09,reply,&m_values);
        if (cancelled()) { close(); return false; }
        m_ready = true;
        return true;
    }

    bool readCycle(const std::function<bool()> &cancelled)
    {
        if (!isOpen() || cancelled()) return false;
        static const quint8 primary[] = {0x01,0x03,0x07,0x08,0x0A,0x0F,0x10,0x12,0x13};
        static const quint8 extra[] = {0x00,0x02,0x05,0x06,0x0B,0x0C,0x0D,0x11,0x21,0x25,0x3A};
        QList<quint8> ids = {0x09,0x19};
        for (int i=0;i<4;++i)
            ids.append(primary[(m_primaryPosition++) % (sizeof(primary)/sizeof(primary[0]))]);
        ids.append(extra[(m_extraPosition++) % (sizeof(extra)/sizeof(extra[0]))]);
        int good = 0;
        for (quint8 id : ids)
        {
            if (cancelled()) return false;
            QByteArray payload;
            payload.append(char(0x21)); payload.append(char(id));
            QByteArray reply;
            const Status status = exchange(payload,payloadPrefix(id),400,cancelled,&reply);
            if (status == Negative)
            {
                // Do not blacklist a field for the entire ECU session.
                // A temporary conditions-not-correct or security response
                // may clear later; preserve every other working measurement.
                continue;
            }
            if (status != Positive) continue;
            if (id==0x19)
            {
                m_values.insert(QStringLiteral("faults_raw"),
                                QString::fromLatin1(reply.toHex(' ').toUpper()));
                m_faults=decodeFaults(reply);
                ++good;
            }
            else if (decodeMeasurement(id,reply,&m_values))
                ++good;
        }
        return good>0;
    }

    // RoverMEMS 2J routine 31 CB + 15 zero bytes; acknowledgement 71 CB.
    // Never used during connection, only on explicit clear-fault request.
    bool clearFaults(const std::function<bool()> &cancelled)
    {
        if (!isOpen() || cancelled()) return false;
        QByteArray command=QByteArray::fromHex("31cb");
        command.append(QByteArray(15,char(0)));
        QByteArray reply;
        return exchange(command,QByteArray::fromHex("71cb"),1000,cancelled,&reply)==Positive;
    }

    QVariantMap values() const { return m_values; }
    QStringList faults() const { return m_faults; }
    bool isOpen() const { return m_ready && m_serial.isOpen(); }
    void close()
    {
        m_ready = false;
        if (m_serial.isOpen()) m_serial.close();
        m_receive.clear(); m_lastSent.clear();
        m_values.clear(); m_faults.clear();
        m_primaryPosition=0; m_extraPosition=0;
    }

private:
    enum Status { Timeout, Positive, Negative };
    static QByteArray payloadPrefix(quint8 id)
    {
        QByteArray p;
        p.append(char(0x61)); p.append(char(id)); return p;
    }
    Status exchange(const QByteArray &command, const QByteArray &positive,
                    int timeout, const std::function<bool()> &cancelled,
                    QByteArray *reply)
    {
        m_lastSent=frame(command);
        if (m_lastSent.isEmpty() ||
            m_serial.write(m_lastSent)!=m_lastSent.size() ||
            !m_serial.waitForBytesWritten(400))
            return Timeout;
        return receive(positive,byte(command,0),timeout,cancelled,reply);
    }
    Status receive(const QByteArray &positive, quint8 service, int timeout,
                   const std::function<bool()> &cancelled, QByteArray *reply)
    {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed()<timeout && !cancelled())
        {
            while (!m_receive.isEmpty())
            {
                // Echo can arrive in several USB chunks. The init echo is
                // unframed; regular KWP payloads are length-framed.
                if (!m_lastSent.isEmpty() && m_receive.startsWith(m_lastSent))
                { m_receive.remove(0,m_lastSent.size()); continue; }
                if (!m_lastSent.isEmpty() && m_lastSent.startsWith(m_receive))
                    break;
                const int n=byte(m_receive,0);
                if (n<1 || n>96) { m_receive.remove(0,1); continue; }
                if (m_receive.size()<n+2) break;
                const QByteArray candidate=m_receive.left(n+2);
                QByteArray payload;
                if (!unpack(candidate,&payload))
                { m_receive.remove(0,1); continue; }
                m_receive.remove(0,n+2);
                if (candidate==m_lastSent) continue;
                if (payload.startsWith(positive))
                { if(reply)*reply=payload; return Positive; }
                if (payload.size()>=3 && byte(payload,0)==0x7F &&
                    byte(payload,1)==service)
                {
                    if (byte(payload,2)==0x78) continue; // response pending
                    if(reply)*reply=payload;
                    return Negative;
                }
                // Ignore a frame from a previous request; never mislabel it.
            }
            const int remaining=timeout-int(timer.elapsed());
            if (remaining<=0) break;
            m_serial.waitForReadyRead(qMin(60,remaining));
            m_receive.append(m_serial.readAll());
            if (!m_serial.isOpen() || m_serial.error()==QSerialPort::ResourceError ||
                m_serial.error()==QSerialPort::DeviceNotFoundError)
                break;
        }
        return Timeout;
    }

    QSerialPort m_serial;
    QByteArray m_receive, m_lastSent;
    QVariantMap m_values;
    QStringList m_faults;
    int m_primaryPosition=0, m_extraPosition=0;
    bool m_ready=false;
};
}
#endif // MEMS2JPROTOCOL_H
