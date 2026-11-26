#ifndef ECU_RECONNECT_POLICY_H
#define ECU_RECONNECT_POLICY_H

// UI-thread session lifecycle. Only an involuntary loss of an established
// session can arm an automatic retry. The worker must acknowledge closure
// before another attempt is allowed.
class EcuReconnectPolicy
{
public:
    enum class State { Idle, Connecting, Connected, LostClosing, RetryWaiting, Reconnecting, Cancelling };
    enum class Outcome { None, Retry, Alert, ManualFailure };
    static constexpr int maxRetries=3;

    State state() const { return m_state; }
    int attempts() const { return m_attempts; }
    bool startManual() {
        if(m_state!=State::Idle)return false;
        m_attempts=0;
        m_state=State::Connecting;
        return true;
    }
    bool startRetry() {
        if(m_state!=State::RetryWaiting || m_attempts>=maxRetries)return false;
        ++m_attempts;
        m_state=State::Reconnecting;
        return true;
    }
    bool established() {
        if(m_state!=State::Connecting && m_state!=State::Reconnecting)return false;
        m_state=State::Connected;
        return true;
    }
    void receivedData() {
        if(m_state==State::Connected)m_attempts=0;
    }
    bool lost() {
        if(m_state!=State::Connected)return false;
        m_state=State::LostClosing;
        return true;
    }
    // Return whether the worker still owes a terminal acknowledgement.
    bool cancel() {
        m_attempts=0;
        if(m_state==State::Idle || m_state==State::RetryWaiting) {
            m_state=State::Idle;
            return false;
        }
        m_state=State::Cancelling;
        return true;
    }
    Outcome failed() {
        if(m_state==State::Reconnecting)return nextRetry();
        if(m_state==State::Connecting) {
            m_state=State::Idle;
            return Outcome::ManualFailure;
        }
        if(m_state==State::Cancelling)m_state=State::Idle;
        return Outcome::None;
    }
    Outcome closed() {
        if(m_state==State::LostClosing || m_state==State::Connected ||
           m_state==State::Reconnecting)return nextRetry();
        if(m_state==State::Connecting) {
            m_state=State::Idle;
            return Outcome::ManualFailure;
        }
        if(m_state==State::Cancelling)m_state=State::Idle;
        return Outcome::None;
    }
    bool waiting() const { return m_state==State::RetryWaiting; }
    bool idle() const { return m_state==State::Idle; }
    bool connected() const { return m_state==State::Connected; }
    bool cancelling() const { return m_state==State::Cancelling; }
    bool starting() const { return m_state==State::Connecting || m_state==State::Reconnecting; }
private:
    Outcome nextRetry() {
        if(m_attempts>=maxRetries) {
            m_state=State::Idle;
            return Outcome::Alert;
        }
        m_state=State::RetryWaiting;
        return Outcome::Retry;
    }
    State m_state=State::Idle;
    int m_attempts=0;
};
#endif
