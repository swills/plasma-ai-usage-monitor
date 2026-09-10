#include "browsersyncservice.h"

#include <QTemporaryDir>
#include <QtTest>

namespace {
class EnvironmentGuard
{
public:
    explicit EnvironmentGuard(const char *name)
        : m_name(name)
        , m_wasSet(qEnvironmentVariableIsSet(name))
        , m_value(qgetenv(name))
    {
    }

    ~EnvironmentGuard()
    {
        if (m_wasSet) {
            qputenv(m_name.constData(), m_value);
        } else {
            qunsetenv(m_name.constData());
        }
    }

private:
    QByteArray m_name;
    bool m_wasSet;
    QByteArray m_value;
};

class FakeMonitor : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE void syncFromBrowser(const QString &cookieHeader, int browserType)
    {
        ++invocationCount;
        receivedCookie = cookieHeader;
        receivedBrowserType = browserType;
    }

    int invocationCount = 0;
    QString receivedCookie;
    int receivedBrowserType = -1;
};
}

class BrowserSyncServiceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void codexSyncDispatchesWithoutCookie();
    void claudeSyncRequiresCookie();
};

void BrowserSyncServiceTest::codexSyncDispatchesWithoutCookie()
{
    QTemporaryDir tempHome;
    QTemporaryDir tempConfig;
    QVERIFY(tempHome.isValid());
    QVERIFY(tempConfig.isValid());

    EnvironmentGuard homeGuard("HOME");
    EnvironmentGuard configGuard("XDG_CONFIG_HOME");
    qputenv("HOME", tempHome.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", tempConfig.path().toUtf8());

    BrowserSyncService service;
    FakeMonitor monitor;

    const bool synced = service.sync(QStringLiteral("codex"), &monitor);

    QVERIFY(synced);
    QCOMPARE(monitor.invocationCount, 1);
    QVERIFY(monitor.receivedCookie.isEmpty());
    QCOMPARE(monitor.receivedBrowserType, int(BrowserCookieExtractor::Firefox));
}

void BrowserSyncServiceTest::claudeSyncRequiresCookie()
{
    QTemporaryDir tempHome;
    QTemporaryDir tempConfig;
    QVERIFY(tempHome.isValid());
    QVERIFY(tempConfig.isValid());

    EnvironmentGuard homeGuard("HOME");
    EnvironmentGuard configGuard("XDG_CONFIG_HOME");
    qputenv("HOME", tempHome.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", tempConfig.path().toUtf8());

    BrowserSyncService service;
    FakeMonitor monitor;

    const bool synced = service.sync(QStringLiteral("claude"), &monitor);

    QVERIFY(!synced);
    QCOMPARE(monitor.invocationCount, 0);
}

QTEST_MAIN(BrowserSyncServiceTest)
#include "test_browsersyncservice.moc"
