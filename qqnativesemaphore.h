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

#if defined(ATOMIC_BOOL_LOCK_FREE) && defined(ATOMIC_INT_LOCK_FREE)
#define QQNATIVESEMAPHORE_LOCK_FREE
#else
#undef QQNATIVESEMAPHORE_LOCK_FREE
#endif


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
 * in async signal handlers (on platforms here std::atomic_bool and std::atomic_int
 * are lock free).
 */
class QQNativeSemaphore : public QObject
{
   Q_OBJECT
public:
    QQNativeSemaphore(bool enabled = false, bool nativeMode = false, int initialValue = 0, QObject *parent = nullptr);
    /**
     * releases all the resources used by this instance. In native mode
     * this unlocks the internal native semaphore which means ongoing
     * waits are unblocked.
     */
    virtual ~QQNativeSemaphore();

    /**
     * Enable or disable the QQNativeSemaphore instance.
     * In native mode this just toggles a flag without affecting
     * waits that are currently ongoing (and which cannot be unblocked
     * until the semaphore is reactivated). In non-native mode this
     * takes down the monitoring thread and the internal native semaphore
     * (but without sending a signal).
     */
    bool setEnabled(bool enabled);
    bool isEnabled() const
    {
        return m_monitorEnabled;
    }
    /**
     * Returns true if this instance is in non-native mode
     * or else if it has a valid, initialised semaphore.
     * Note that a valid QQNativeSemaphore can be inactive (disabled)!
     */
    bool isValid() const
    {
        return m_hasSemaphore || !m_nativeMode;
    }

    int value() const
    {
        return m_currentValue;
    }

//     bool setValue(int val)
//     {
//         if (val >= 0) {
//             m_currentValue = val;
//             return true;
//         }
//         return false;
//     }

    /**
     * Returns true if QQNativeSemaphore is lock free.
     * See also the QQNATIVESEMAPHORE_LOCK_FREE macro.
     */
    static bool is_lock_free()
    {
#ifdef QQNATIVESEMAPHORE_LOCK_FREE
        return true;
#else
        return false;
#endif
    }

    /**
     * Native mode only.
     * Returns true after a blocking wait and will have emitted the
     * triggered(@p val) signal in that case. The argument passed
     * with the signal can be set through the call to QQNativeSemaphore::trigger()
     * which unlocks the semaphore, or through this function (which takes
     * precedence).
     * @p checkFirst : return true if the operation would block but do not
     * actually block.
     */
    bool wait(bool checkFirst = false, QVariant val = QVariant());
    /**
     * As QQNativeSemaphore::wait() but with a timeout (in seconds).
     */
    bool timedWait(double timeOut, QVariant val = QVariant());

Q_SIGNALS:
    /**
     * signal sent when the semaphore triggers (unlocks)
     *
     * @p val arbitrary value set through QQNativeSemaphore::trigger()
     * or QQNativeSemaphore::wait()/timedWait() (in native mode).
     */
    void triggered(QVariant val);

public Q_SLOTS:
    /**
     * Native mode:
     * INCREASE the semaphore, unlocking if it was locked.
     * Non-native (trigger) mode:
     * DECREASE the current value by one; when zero is reached
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
#ifdef Q_OS_MACOS
    int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
#endif

    bool m_nativeMode;
    bool m_hasSemaphore;

#ifdef Q_OS_UNIX
    sem_t m_sem;
#endif
    std::atomic_bool m_monitorEnabled;
    std::atomic_int m_currentValue;
    pthread_t m_monitorThread;
};

#endif
