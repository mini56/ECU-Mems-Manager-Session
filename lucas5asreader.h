#ifndef LUCAS5ASREADER_H
#define LUCAS5ASREADER_H

// SESSION 190 - isolated Lucas 5AS diagnostic transport and output tests.
// The 5AS uses a separate adapted KKL interface (OBD 3 TX / OBD 1 RX).
// No EEPROM, immobiliser programming, PLIP learning or motor/RC5 commands.
#include "lucas5asprotocol.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariantMap>
#include <atomic>
#include <functional>
#include <memory>

namespace Lucas5AsReader {

class Session
{
public:
    using Cancelled=std::function<bool()>;
    ~Session(){close();}
    bool isOpen() const {return m_ready && m_port.isOpen();}
    void close()
    {
        m_ready=false;
        if(m_port.isOpen()){
            m_port.setBreakEnabled(false);
            m_port.close();
        }
        m_received.clear();
    }

    bool connectTo(const QString &port, const Cancelled &cancelled,
                   Lucas5AsProtocol::StatusPacket *initial,
                   QString *trace)
    {
        close();
        if(cancelled())return false;
        m_port.setPortName(port);
        if(!m_port.setBaudRate(Lucas5AsProtocol::kNormalBaud) ||
           !m_port.setDataBits(QSerialPort::Data8) ||
           !m_port.setParity(QSerialPort::NoParity) ||
           !m_port.setStopBits(QSerialPort::OneStop) ||
           !m_port.setFlowControl(QSerialPort::NoFlowControl) ||
           !m_port.open(QIODevice::ReadWrite)){
            close();
            return false;
        }
        m_port.clear(QSerialPort::AllDirections);
        if(!m_port.setBreakEnabled(false) || !pause(300,cancelled)){
            close();return false;
        }

        // Reuse only the *transport principle* from MEMS 1.9: BREAK low
        // for logical zero, each ten-bit frame cell scheduled to one clock.
        // The Lucas address is different: 0x1C, not the engine's 0x16.
        // An actual 5AS response is still mandatory.
        QElapsedTimer bitClock;
        bitClock.start();
        for(int bit=0;bit<Lucas5AsProtocol::kSlowBitCount;++bit){
            if(cancelled() ||
               !m_port.setBreakEnabled(!Lucas5AsProtocol::slowBitHigh(bit))){
                close();return false;
            }
            const qint64 deadline=qint64(bit+1)*
                Lucas5AsProtocol::kSlowBitDurationMs;
            if(!pauseUntil(bitClock,deadline,cancelled)){
                close();return false;
            }
        }
        if(!m_port.setBreakEnabled(false)){close();return false;}
        if(trace)
            *trace=QStringLiteral("TX @5Bd 1C (10 bits, 200 ms/bit)\n");

        // The Lucas 5AS normal wake is 55 83 76. Do not transmit a
        // hypothetical ISO9141 key-byte ACK: the Andrew Revill binary
        // analysis shows no such ACK in its normal-mode SlowInit routine.
        if(!waitForWake(2200,cancelled)){
            close();return false;
        }
        if(trace)trace->append(QStringLiteral("RX @9600 55 83 76\n"));

        Lucas5AsProtocol::StatusPacket packet;
        if(!requestStatus(&packet,cancelled,trace)){
            close();return false;
        }
        m_ready=true; // Actual wake AND a length-framed status from an ECU.
        if(initial)*initial=packet;
        return true;
    }

    bool readStatus(Lucas5AsProtocol::StatusPacket *result,
                    const Cancelled &cancelled,QString *trace)
    {
        if(!isOpen() || cancelled())return false;
        return requestStatus(result,cancelled,trace);
    }

    // One bounded physical output test after an explicit UI confirmation.
    // Always attempt the corresponding STOP, even after cancellation,
    // a failed START transmission or a serial write timeout. Only NORMAL
    // diagnostic-mode tests 8/9/13 are accepted; no EEPROM or PLIP command.
    bool pulseOutput(int output,const Cancelled &cancelled,QString *trace)
    {
        quint8 start=0,stop=0;
        if(!isOpen() || cancelled() ||
           !Lucas5AsProtocol::outputTestCommands(output,&start,&stop))
            return false;
        const auto send=[this,trace](quint8 command){
            if(!m_port.isOpen())return false;
            const char octet=char(command);
            const bool sent=m_port.write(&octet,1)==1 &&
                            m_port.waitForBytesWritten(500);
            if(trace)
                trace->append(QStringLiteral("TX @9600 %1 (%2)\n")
                    .arg(qulonglong(command),2,16,QLatin1Char('0'))
                    .arg(sent?QStringLiteral("sent"):QStringLiteral("error")));
            return sent;
        };
        const bool started=send(start);
        const bool completed=started && pause(900,cancelled);
        const bool stopped=send(stop);
        return started && completed && stopped;
    }

private:
    bool pause(int milliseconds,const Cancelled &cancelled)
    {
        int elapsed=0;
        while(elapsed<milliseconds && !cancelled()){
            const int delay=qMin(25,milliseconds-elapsed);
            QThread::msleep(static_cast<unsigned long>(delay));
            elapsed+=delay;
        }
        return !cancelled();
    }
    bool pauseUntil(const QElapsedTimer &clock,qint64 deadline,
                    const Cancelled &cancelled)
    {
        while(clock.elapsed()<deadline && !cancelled()){
            const qint64 left=deadline-clock.elapsed();
            if(left>0)
                QThread::msleep(static_cast<unsigned long>(qMin(qint64(20),left)));
        }
        return !cancelled();
    }
    bool portUsable() const
    {
        return m_port.isOpen() &&
            m_port.error()!=QSerialPort::ResourceError &&
            m_port.error()!=QSerialPort::DeviceNotFoundError;
    }
    bool waitForWake(int timeout,const Cancelled &cancelled)
    {
        QByteArray wake;
        QElapsedTimer timer;
        timer.start();
        while(timer.elapsed()<timeout && !cancelled()){
            wake+=m_port.readAll();
            if(Lucas5AsProtocol::hasNormalWakeReply(wake)){
                // Preserve any bytes already delivered after the handshake.
                const int offset=wake.indexOf(Lucas5AsProtocol::normalWakeReply());
                m_received=wake.mid(offset+3);
                return true;
            }
            if(wake.size()>48)
                wake.remove(0,wake.size()-48);
            const int remaining=timeout-int(timer.elapsed());
            if(remaining<=0)break;
            m_port.waitForReadyRead(qMin(50,remaining));
            if(!portUsable())return false;
        }
        return false;
    }
    bool requestStatus(Lucas5AsProtocol::StatusPacket *result,
                       const Cancelled &cancelled,QString *trace)
    {
        if(!result || cancelled() || !m_port.isOpen())return false;
        const char command=char(Lucas5AsProtocol::kReadStatusCommand);
        // Discard any leftover handshake bytes before a new normal read.
        m_received.clear();
        m_port.readAll();
        if(m_port.write(&command,1)!=1 ||
           !m_port.waitForBytesWritten(500))
            return false;
        if(trace)trace->append(QStringLiteral("TX @9600 83\n"));

        QElapsedTimer timer;
        timer.start();
        while(timer.elapsed()<2000 && !cancelled()){
            m_received+=m_port.readAll();
            if(m_received.size()>256)return false;
            if(Lucas5AsProtocol::extractStatus(m_received,result)){
                if(trace)
                    trace->append(QStringLiteral("RX @9600 %1\n")
                        .arg(QString::fromLatin1(
                            result->raw.toHex(' ').toUpper())));
                m_received.clear();
                return true;
            }
            const int remaining=2000-int(timer.elapsed());
            if(remaining<=0)break;
            m_port.waitForReadyRead(qMin(50,remaining));
            if(!portUsable())return false;
        }
        return false;
    }
    QSerialPort m_port; // Created and used exclusively on worker thread.
    QByteArray m_received;
    bool m_ready=false;
};

inline QVariantMap packetValues(const Lucas5AsProtocol::StatusPacket &p)
{
    QVariantMap data;
    const auto rawBit=[](const Lucas5AsProtocol::SwitchField &field){
        return field.available
            ? QStringLiteral("0x%1")
                  .arg(qulonglong(field.bit7High?0x80:0x00),2,16,
                       QLatin1Char('0'))
            : QString();
    };
    data.insert(QStringLiteral("bonnet"),rawBit(p.bonnet));
    data.insert(QStringLiteral("ignition"),rawBit(p.ignition));
    data.insert(QStringLiteral("doors"),rawBit(p.doors));
    data.insert(QStringLiteral("possibleBoot"),rawBit(p.possibleBoot));
    data.insert(QStringLiteral("model"),p.unitDescription);
    data.insert(QStringLiteral("serial"),QString::fromLatin1(
        p.serialLittleEndian.toHex(' ').toUpper()));
    for(int n=0;n<4;++n)
        data.insert(QStringLiteral("plip%1").arg(n+1),
            n<p.fobCountersRaw.size()
                ?QStringLiteral("0x%1")
                    .arg(qulonglong(p.fobCountersRaw.at(n)),2,16,
                         QLatin1Char('0'))
                :QString());
    return data;
}

} // namespace Lucas5AsReader

class Lucas5AsWorker final : public QObject
{
    Q_OBJECT
public:
    explicit Lucas5AsWorker(QObject *parent=nullptr):QObject(parent){}
    void allowRequests(){m_cancel.store(false,std::memory_order_relaxed);}
    void requestCancel(){m_cancel.store(true,std::memory_order_relaxed);}

public slots:
    void connectCandidates(const QStringList &ports)
    {
        closeSession();
        const auto cancelled=[this](){
            return m_cancel.load(std::memory_order_relaxed);
        };
        for(const QString &port:ports){
            if(cancelled())break;
            emit probingPort(port);
            std::unique_ptr<Lucas5AsReader::Session> candidate(
                new Lucas5AsReader::Session);
            Lucas5AsProtocol::StatusPacket status;
            QString trace;
            if(!candidate->connectTo(port,cancelled,&status,&trace))continue;
            if(cancelled())break;
            m_session=std::move(candidate);
            emit connectionChanged(true,port);
            emit statusRead(true,Lucas5AsReader::packetValues(status),trace);
            return;
        }
        closeSession();
        emit connectionChanged(false,QString());
    }
    void disconnectModule()
    {
        requestCancel();
        closeSession();
        emit connectionChanged(false,QString());
    }
    void readStatus()
    {
        if(!m_session || !m_session->isOpen()){
            emit statusRead(false,QVariantMap(),QString());
            return;
        }
        const auto cancelled=[this](){
            return m_cancel.load(std::memory_order_relaxed);
        };
        Lucas5AsProtocol::StatusPacket status;
        QString trace;
        if(m_session->readStatus(&status,cancelled,&trace))
            emit statusRead(true,Lucas5AsReader::packetValues(status),trace);
        else emit statusRead(false,QVariantMap(),QString());
    }
    void pulseOutput(int output)
    {
        QString trace;
        const auto cancelled=[this](){
            return m_cancel.load(std::memory_order_relaxed);
        };
        const bool ok=m_session && m_session->isOpen() &&
            m_session->pulseOutput(output,cancelled,&trace);
        emit outputTested(ok,output,trace);
    }

signals:
    void probingPort(const QString &port);
    void connectionChanged(bool connected,const QString &port);
    void statusRead(bool ok,const QVariantMap &fields,const QString &trace);
    void outputTested(bool ok,int output,const QString &trace);

private:
    void closeSession()
    {
        if(m_session){
            m_session->close();
            m_session.reset();
        }
    }
    std::unique_ptr<Lucas5AsReader::Session> m_session;
    std::atomic<bool> m_cancel{false};
};

#endif // LUCAS5ASREADER_H
