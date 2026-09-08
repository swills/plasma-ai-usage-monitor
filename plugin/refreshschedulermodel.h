#ifndef REFRESHSCHEDULERMODEL_H
#define REFRESHSCHEDULERMODEL_H

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QtQmlIntegration/qqmlintegration.h>

class RefreshSchedulerModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit RefreshSchedulerModel(QObject *parent = nullptr);

    Q_INVOKABLE void startMonitoring();
    void observeWakeClock(const QDateTime &now);
    void observeReachability(bool online);

    Q_INVOKABLE int deterministicJitterMs(const QString &providerKey) const;
    Q_INVOKABLE int effectiveIntervalMs(int providerSeconds,
                                        int globalSeconds,
                                        bool popupOpen) const;
    Q_INVOKABLE double backoffMultiplier(int consecutiveErrors, bool retryable) const;
    Q_INVOKABLE int scheduledIntervalMs(const QString &providerKey,
                                        int providerSeconds,
                                        int globalSeconds,
                                        bool popupOpen,
                                        int consecutiveErrors,
                                        bool retryable) const;
    Q_INVOKABLE bool isFresh(const QDateTime &lastSuccess,
                             int providerSeconds,
                             int globalSeconds,
                             bool popupOpen,
                             const QDateTime &now = QDateTime::currentDateTimeUtc()) const;
    Q_INVOKABLE QDateTime nextScheduledRefresh(const QDateTime &lastSuccess,
                                               const QString &providerKey,
                                               int providerSeconds,
                                               int globalSeconds,
                                               bool popupOpen,
                                               int consecutiveErrors,
                                               bool retryable) const;
  Q_SIGNALS:
    void recoveryRequested();

  private:
    QTimer m_wakeTimer;
    QTimer m_recoveryTimer;
    QDateTime m_lastWakeCheck;
    bool m_seenOffline = false;
    bool m_monitoring = false;
};

#endif
