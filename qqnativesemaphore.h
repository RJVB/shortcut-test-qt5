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
    void triggered(QVariant val);

public Q_SLOTS:
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
