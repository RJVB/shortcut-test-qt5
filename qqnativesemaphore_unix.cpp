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

QQNativeSemaphore::QQNativeSemaphore::QQNativeSemaphore(bool enabled, int initialValue, QObject* parent)
    : QObject(parent)
    , m_triggerValue(QVariant())
    , m_monitorEnabled(false)
    , m_currentValue(initialValue)
{
    setEnabled(enabled);
}

QQNativeSemaphore::~QQNativeSemaphore()
{
    setEnabled(false);
}

bool QQNativeSemaphore::setEnabled(bool enabled)
{
    if (enabled) {
        if (sem_init(&m_sem, 0, 0) != -1) {
            m_monitorEnabled = true;
            if (pthread_create(&m_monitorThread, nullptr, monitorStarter, this) == 0) {
                pthread_detach(m_monitorThread);
            } else {
                m_monitorThread = 0;
                m_monitorEnabled = false;
                sem_destroy(&m_sem);
            }
        } else {
            qCritical() << this << "couldn't create semaphore:" << strerror(errno);
        }
    } else if (m_monitorEnabled) {
        m_monitorEnabled = false;
        qWarning() << "\tsignalling semaphore monitor to exit";
        sem_post(&m_sem);
        sem_destroy(&m_sem);
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
                m_currentValue = 0;
                qWarning() << Q_FUNC_INFO << "semaphore triggered with" << m_triggerValue;
                emit triggered(m_triggerValue);
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
    if (m_monitorEnabled) {
        if (m_currentValue > 0) {
            m_currentValue -= 1;
            return false;
        } else {
            sem_post(&m_sem);
        }
    }
    return m_monitorEnabled;
}
