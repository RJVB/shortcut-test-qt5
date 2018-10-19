#ifndef QQNATIVESEMAPHORE_H
#define QQNATIVESEMAPHORE_H

#include <QObject>
#include <QVariant>

#ifdef Q_OS_UNIX
#ifdef Q_OS_MACOS
#include <mach/mach.h>
typedef semaphore_t sem_t;
#else
#include <semaphore.h>
#endif
#include <pthread.h>
#include <unistd.h>
#endif

#include <atomic>
#include <csignal>

/**
 * QQNativeSemaphore : a thin wrapper around platform-native semaphores.
 *
 * Each instance sets up a single semaphore when enabled and spawns a thread
 * that waits on the semaphore. When the semaphore is released/signalled (the
 * wait ends) the QQNativeSemaphore::triggered signal is emitted and the instance
 * rearms itself. Releasing (triggering) the semaphore is done with the
 * QQNativeSemaphore::trigger() method.
 *
 * POSIX semaphores emulate a queue; they lock when empty and become (remain)
 * unlicked as long as they are not empty. The initial value is thus equivalent
 * to an amount or resources that can be spent before locking.
 * QQNativeSemaphore adopts the opposite approach: its initial value represents
 * the number of triggers that must be sent in order to unlock the semaphore (and
 * emit the triggered signal).
 *
 * Care has been taken to consume as little resources as possible (including open
 * file descriptors), and that QQNativeSemaphore::trigger() is safe to be called
 * in async signal handlers.
 */
class QQNativeSemaphore : public QObject
{
   Q_OBJECT
public:
    QQNativeSemaphore(bool enabled = false, int initialValue = 0, QObject *parent = nullptr);
    virtual ~QQNativeSemaphore();

    bool setEnabled(bool enabled);
    bool isEnabled() const
    {
        return m_monitorEnabled;
    }

    int value() const
    {
        return m_currentValue;
    }

    bool setValue(int val)
    {
        if (val >= 0) {
            m_currentValue = val;
            return true;
        }
        return false;
    }

Q_SIGNALS:
    /**
     * signal sent when the semaphore triggers (unlocks)
     *
     * @p val arbitrary valueset through QQNativeSemaphore::trigger().
     */
    void triggered(QVariant val);

public Q_SLOTS:
    /**
     * Decrease the current value by one; when zero is reached
     * the internal semaphore is released and the triggered() signal is sent.
     *
     * @p val an arbitrary value that will be sent out with the trigger signal.
     * Note that this feature is not thread or signal safe; when multiple calls
     * are made to trigger() before the monitor thread has had a chance to send
     * signal out the last value will be sent with the signal.
     */
    bool trigger(QVariant val = QVariant());

private:
    QVariant m_triggerValue;
    void semaphoreMonitor();
    static void *monitorStarter(void*);

#ifdef Q_OS_UNIX
    sem_t m_sem;
#endif
    // std::atomic_bool would be fine but only if is_lock_free
#ifdef ATOMIC_BOOL_LOCK_FREE
    std::atomic_bool m_monitorEnabled;
#else
    sig_atomic_t m_monitorEnabled;
#endif
#ifdef ATOMIC_INT_LOCK_FREE
    std::atomic_int m_currentValue;
#else
    sig_atomic_t m_currentValue;
#endif
    pthread_t m_monitorThread;
};

#endif
