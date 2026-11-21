#ifndef RC5PROTOCOL_H
#define RC5PROTOCOL_H

// RC5/EC5 Airbag SRS, independent of engine MEMS and ROSCO.
// Reference: archived RoverMEMS i386 ecu-rc5.go, James Portman (GPLv3).
#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <QtGlobal>
#include <atomic>
#include <functional>
#include <memory>

namespace Rc5Protocol {
inline quint8 byte(const QByteArray &p,int i)
{ return quint8(static_cast<unsigned char>(p.at(i))); }

// Header: 0x80/0xC0 + length-1. The sum of all bytes, including checksum,
// is 0xFF modulo 256. This is NOT the MEMS3 XOR frame.
inline int packetLength(quint8 header)
{
    if((header&0xC0u)!=0x80u && (header&0xC0u)!=0xC0u)return -1;
    const int n=int(header&0x3Fu)+1;
    return n>=3 && n<=64 ? n : -1;
}
inline bool validPacket(const QByteArray &p)
{
    if(p.isEmpty() || packetLength(byte(p,0))!=p.size())return false;
    unsigned int sum=0;
    for(char c:p)sum+=quint8(static_cast<unsigned char>(c));
    return (sum&0xFFu)==0xFFu;
}
inline QByteArray request(quint8 command)
{
    QByteArray p;
    p.append(char(0x82));
    p.append(char(command));
    p.append(char((0xFFu-0x82u-command)&0xFFu));
    return p;
}
inline int descriptionKey(quint16 code)
{
    const quint16 normal=(code>=0x2500u && code<=0x26FFu)
        ? quint16(code-0x1000u) : code;
    static const quint16 ids[]={
        0x150A,0x150B,0x150C,0x150D,0x150E,
        0x1512,0x1513,0x1514,0x1515,0x1516,
        0x151A,0x151B,0x151C,0x151D,0x151E,
        0x1524,0x1525,0x1526,0x152C,0x152D,0x152E,
        0x160C,0x160D,0x160E
    };
    for(int i=0;i<int(sizeof(ids)/sizeof(ids[0]));++i)
        if(normal==ids[i])return 16840+i;
    return 16880;
}
// One fully validated frame only: don't decode the trailing checksum as a DTC.
inline bool decodeFaultPacket(const QByteArray &p,QVariantList *faults)
{
    if(!faults || !validPacket(p) || (byte(p,0)&0xC0u)!=0xC0u ||
       p.size()<3 || byte(p,1)!=0x33 || (p.size()-3)%2!=0)
        return false;
    QVariantList result;
    for(int i=2;i+1<p.size()-1;i+=2){
        const quint16 id=quint16((quint16(byte(p,i))<<8)|byte(p,i+1));
        QVariantMap f;
        f.insert(QStringLiteral("code"),int(id));
        f.insert(QStringLiteral("historical"),id>=0x2500u && id<=0x26FFu);
        f.insert(QStringLiteral("known"),descriptionKey(id)!=16880);
        f.insert(QStringLiteral("descriptionKey"),descriptionKey(id));
        result.append(f);
    }
    *faults=result;
    return true;
}

class Session
{
public:
    using Cancelled=std::function<bool()>;
    ~Session(){close();}
    bool connectTo(const QString &port,const Cancelled &cancelled)
    {
        close();
        m_serial.setPortName(port);
        if(!m_serial.setBaudRate(2400) ||
           !m_serial.setDataBits(QSerialPort::Data8) ||
           !m_serial.setParity(QSerialPort::NoParity) ||
           !m_serial.setStopBits(QSerialPort::OneStop) ||
           !m_serial.setFlowControl(QSerialPort::NoFlowControl) ||
           !m_serial.open(QIODevice::ReadWrite)){
            close();return false;
        }
        m_serial.clear(QSerialPort::AllDirections);
        // RC5 wake, as used by archived RoverMEMS i386.
        const bool levels[]={false,true,false,true,false,true,false};
        const unsigned long times[]={2000,200,400,400,400,400,200};
        for(int i=0;i<7;++i){
            if(cancelled() || !m_serial.setBreakEnabled(levels[i]) ||
               !pause(times[i],cancelled)){close();return false;}
        }
        if(!waitWake(1100,cancelled) || !pause(700,cancelled)){
            close();return false;
        }
        QByteArray rx;
        if(!exchange(request(0x00),QByteArray::fromHex("c2003d"),
                     false,1100,cancelled,&rx)){close();return false;}
        m_ready=true; // Confirmed by ECU response, never just COM presence.
        return true;
    }
    bool readFaults(QVariantList *faults,QString *raw,const Cancelled &cancelled)
    {
        if(!isOpen() || cancelled())return false;
        QByteArray rx;
        if(!exchange(request(0x33),QByteArray(),true,1300,cancelled,&rx))
            return false;
        QVariantList decoded;
        if(!decodeFaultPacket(rx,&decoded))return false;
        if(faults)*faults=decoded;
        if(raw)*raw=QString::fromLatin1(rx.toHex(' ').toUpper());
        return true;
    }
    // Only called by explicit Airbag "Effacer les défauts" action.
    bool clearFaults(const Cancelled &cancelled)
    {
        if(!isOpen() || cancelled())return false;
        QByteArray rx;
        return exchange(request(0xC3),QByteArray::fromHex("c2c37a"),
                        false,1300,cancelled,&rx);
    }
    bool isOpen()const{return m_ready && m_serial.isOpen();}
    void close()
    {
        m_ready=false;
        if(m_serial.isOpen()){
            m_serial.setBreakEnabled(false);
            m_serial.close();
        }
        m_rx.clear();
    }
private:
    bool pause(unsigned long ms,const Cancelled &cancelled)
    {
        unsigned long done=0;
        while(done<ms && !cancelled()){
            const unsigned long step=qMin(30ul,ms-done);
            QThread::msleep(step);
            done+=step;
        }
        return !cancelled();
    }
    bool waitWake(int timeout,const Cancelled &cancelled)
    {
        const QByteArray wake=QByteArray::fromHex("55063b");
        QElapsedTimer timer;timer.start();
        while(timer.elapsed()<timeout && !cancelled()){
            m_rx.append(m_serial.readAll());
            const int at=m_rx.indexOf(wake);
            if(at>=0){m_rx.remove(0,at+wake.size());return true;}
            if(m_rx.size()>96)m_rx.remove(0,m_rx.size()-96);
            const int left=timeout-int(timer.elapsed());
            if(left<=0)break;
            m_serial.waitForReadyRead(qMin(60,left));
            if(!m_serial.isOpen() ||
               m_serial.error()==QSerialPort::ResourceError ||
               m_serial.error()==QSerialPort::DeviceNotFoundError)return false;
        }
        return false;
    }
    bool exchange(const QByteArray &sent,const QByteArray &expect,
                  bool faults,int timeout,const Cancelled &cancelled,
                  QByteArray *response)
    {
        if(cancelled() || !validPacket(sent) ||
           m_serial.write(sent)!=sent.size() ||
           !m_serial.waitForBytesWritten(400))return false;
        QElapsedTimer timer;timer.start();
        while(timer.elapsed()<timeout && !cancelled()){
            m_rx.append(m_serial.readAll());
            while(!m_rx.isEmpty()){
                if(m_rx.startsWith(sent)){m_rx.remove(0,sent.size());continue;}
                if(sent.startsWith(m_rx))break; // Partial local cable echo.
                const int n=packetLength(byte(m_rx,0));
                if(n<0){m_rx.remove(0,1);continue;}
                if(m_rx.size()<n)break;
                const QByteArray p=m_rx.left(n);
                if(!validPacket(p)){m_rx.remove(0,1);continue;}
                m_rx.remove(0,n);
                if(p==sent || (byte(p,0)&0xC0u)!=0xC0u)continue;
                if(faults){
                    QVariantList unused;
                    if(decodeFaultPacket(p,&unused)){
                        if(response)*response=p;
                        return true;
                    }
                }else if(p==expect){
                    if(response)*response=p;
                    return true;
                }
            }
            const int left=timeout-int(timer.elapsed());
            if(left<=0)break;
            m_serial.waitForReadyRead(qMin(60,left));
            if(!m_serial.isOpen() ||
               m_serial.error()==QSerialPort::ResourceError ||
               m_serial.error()==QSerialPort::DeviceNotFoundError)return false;
        }
        return false;
    }
    QSerialPort m_serial; // Session instantiated on worker thread.
    QByteArray m_rx;
    bool m_ready=false;
};
} // namespace Rc5Protocol

// One independent worker for the Airbag column. Blocking wake timings run
// off the GUI thread; other two columns stay reserved.
class Rc5Worker final : public QObject
{
    Q_OBJECT
public:
    explicit Rc5Worker(QObject *parent=nullptr):QObject(parent){}
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
            std::unique_ptr<Rc5Protocol::Session> candidate(
                new Rc5Protocol::Session);
            if(!candidate->connectTo(port,cancelled))continue;
            if(cancelled())break;
            m_session=std::move(candidate);
            emit connectionChanged(true,port);
            readFaults();
            return;
        }
        closeSession();
        emit connectionChanged(false,QString());
    }
    void disconnectModule()
    {
        requestCancel();closeSession();
        emit connectionChanged(false,QString());
    }
    void readFaults()
    {
        if(!m_session || !m_session->isOpen()){
            emit faultsRead(false,QVariantList(),QString());return;
        }
        const auto cancelled=[this](){
            return m_cancel.load(std::memory_order_relaxed);
        };
        QVariantList faults;
        QString raw;
        if(m_session->readFaults(&faults,&raw,cancelled))
            emit faultsRead(true,faults,raw);
        else emit faultsRead(false,QVariantList(),QString());
    }
    void clearFaults()
    {
        const auto cancelled=[this](){
            return m_cancel.load(std::memory_order_relaxed);
        };
        if(!m_session || !m_session->isOpen() || cancelled()){
            emit faultsCleared(false);return;
        }
        const bool success=m_session->clearFaults(cancelled);
        emit faultsCleared(success);
        if(success && !cancelled()){
            QThread::msleep(200);
            readFaults();
        }
    }
signals:
    void probingPort(const QString &port);
    void connectionChanged(bool connected,const QString &port);
    void faultsRead(bool success,const QVariantList &faults,const QString &raw);
    void faultsCleared(bool success);
private:
    void closeSession()
    {
        if(m_session){
            m_session->close();
            m_session.reset();
        }
    }
    std::unique_ptr<Rc5Protocol::Session> m_session;
    std::atomic<bool> m_cancel{false};
};
#endif // RC5PROTOCOL_H
