#include "qqnativesemaphore.h"

#include <QDebug>

#include <errno.h>

#ifdef Q_OS_MACOS
// Darwin doesn't have unnamed POSIX semaphores but can use MACH semaphores.
#define sem_init(s,x,value)     semaphore_create(mach_task_self(), (s), SYNC_POLICY_FIFO, (value))
#define sem_wait(s)             semaphore_wait(*(s))
#define sem_post(s)             semaphore_signal(*(s))
#define sem_destroy(s)          semaphore_destroy(mach_task_self(), *(s))
#define pthread_setname(t,n)    pthread_setname_np((n));
#endif // Q_OS_MACOS
#ifdef Q_OS_LINUX
#define pthread_setname(t,n)    pthread_setname_np((t),(n));
#endif

QQNativeSemaphore::QQNativeSemaphore::QQNativeSemaphore(bool enabled, bool nativeMode, int initialValue, QObject* parent)
    : QObject(parent)
    , m_triggerValue(QVariant())
    , m_nativeMode(nativeMode)
    , m_hasSemaphore(false)
    , m_monitorEnabled(false)
    , m_currentValue(initialValue)
{
    if (m_nativeMode) {
        if (sem_init(&m_sem, 0, initialValue) != -1) {
            m_hasSemaphore = true;
        }
    }
    setEnabled(enabled);
}

QQNativeSemaphore::~QQNativeSemaphore()
{
    if (m_nativeMode) {
        if (m_hasSemaphore) {
            m_monitorEnabled = false;
            sem_post(&m_sem);
            sem_destroy(&m_sem);
        }
    } else {
        setEnabled(false);
    }
}

bool QQNativeSemaphore::setEnabled(bool enabled)
{
    bool ret = true;
    if (m_nativeMode) {
        m_monitorEnabled = enabled && m_hasSemaphore;
    } else if (enabled && !m_monitorEnabled.exchange(true)) {
        if (sem_init(&m_sem, 0, 0) != -1) {
            if (pthread_create(&m_monitorThread, nullptr, monitorStarter, this) == 0) {
                m_hasSemaphore = true;
                pthread_detach(m_monitorThread);
            } else {
                m_monitorThread = 0;
                m_monitorEnabled = false;
                ret = false;
                sem_destroy(&m_sem);
            }
        } else {
            // we failed, so turn back off
            m_monitorEnabled = false;
            qCritical() << this << "couldn't create semaphore:" << strerror(errno);
            ret = false;
        }
    } else if (m_monitorEnabled.exchange(false)) {
        qWarning() << "\tsignalling semaphore monitor to exit";
        sem_post(&m_sem);
        sem_destroy(&m_sem);
        m_hasSemaphore = false;
    }
    return true;
}

void* QQNativeSemaphore::monitorStarter(void *arg)
{
    QQNativeSemaphore *that = static_cast<QQNativeSemaphore*>(arg);
    that->semaphoreMonitor();
    return nullptr;
}

void QQNativeSemaphore::semaphoreMonitor()
{
    if (objectName().isEmpty()) {
        pthread_setname(pthread_self(), "QQNativeSemaphore monitor thread");
    } else {
        pthread_setname(pthread_self(), objectName().toLocal8Bit().constData());
    }
    int s;
    while (m_monitorEnabled && (((s = sem_wait(&m_sem)) == -1 && errno == EINTR) || s == 0)) {
        if (m_monitorEnabled) {
            if (s == 0) {
                qWarning() << Q_FUNC_INFO << "semaphore triggered with" << m_triggerValue;
                emit triggered(m_triggerValue);
                m_triggerValue = QVariant();
            } else {
                perror("sem_wait");
            }
            qWarning() << "\tmonitor continues";
        }
        continue;       /* Restart if interrupted by handler */
    }
    qWarning() << Q_FUNC_INFO << "monitor exitting";
}

bool QQNativeSemaphore::trigger(QVariant val)
{
    m_triggerValue = val;
    bool ret = false;
    if (m_monitorEnabled) {
        if (m_nativeMode) {
            m_currentValue += 1;
            sem_post(&m_sem);
            ret = true;
        } else {
            if (m_currentValue.fetch_sub(1) == 0) {
                sem_post(&m_sem);
                ret = true;
            }
        }
    }
    return ret;
}

bool QQNativeSemaphore::wait(bool checkFirst, QVariant val)
{
    bool ret = false, waited = false;
    if (m_nativeMode && m_hasSemaphore && m_monitorEnabled) {
        if (m_currentValue.fetch_sub(1) == 0) {
            errno = 0;
            if (checkFirst) {
                ret = true;
                errno = EAGAIN;
            } else {
                ret = (sem_wait(&m_sem) == 0);
                waited = (errno != EINTR);
                ret &= waited;
            }
        }
    }
    if (ret) {
        if (val.isValid()) {
            m_triggerValue = val;
        }
        qWarning() << Q_FUNC_INFO << "semaphore triggered with" << m_triggerValue;
        emit triggered(m_triggerValue);
        m_triggerValue = QVariant();
    }
    return ret;
}

#ifdef Q_OS_MACOS
// from https://raw.githubusercontent.com/tumi8/vermont/master/src/osdep/osx/sem_timedwait.cpp
int QQNativeSemaphore::sem_timedwait(sem_t *sem, const struct timespec *timeout)
{
    int ret = -1;
    struct mach_timespec mts;
    mts.tv_sec = timeout->tv_sec;
    mts.tv_nsec = timeout->tv_nsec;
    switch (semaphore_timedwait(*sem, mts)) {
        case KERN_SUCCESS:
            ret = 0;
        case KERN_OPERATION_TIMED_OUT:
            errno = ETIMEDOUT;
            break;
        case KERN_ABORTED:
            errno = EINTR;
            break;
        default:
            errno =  EINVAL;
            break;
    }
    return -1;
}
#endif

bool QQNativeSemaphore::timedWait(double timeOut, QVariant val)
{
    bool ret = false, waited = false;
    if (m_nativeMode && m_hasSemaphore && m_monitorEnabled) {
        if (m_currentValue.fetch_sub(1) == 0) {
            struct timespec ts;
            ts.tv_sec = time_t(timeOut);
            ts.tv_nsec = long((timeOut - ts.tv_sec) * 1e9);
            errno = 0;
            ret = (sem_timedwait(&m_sem, &ts) == 0);
            waited = (errno != EINTR) && (errno != ETIMEDOUT);
            ret &= waited;
            // it is not entirely clear if we should restore (re-increment)
            // m_currentValue if the wait timed out?
        }
    }
    if (ret) {
        if (val.isValid()) {
            m_triggerValue = val;
        }
        qWarning() << Q_FUNC_INFO << "semaphore triggered with" << m_triggerValue;
        emit triggered(val);
        m_triggerValue = QVariant();
    }
    return ret;
}

