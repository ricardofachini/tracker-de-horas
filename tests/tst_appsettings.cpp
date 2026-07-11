#include "appsettings.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

// Testes da meta de jornada. O QSettings é redirecionado para um diretório
// temporário para não tocar a configuração real do usuário.
class TestAppSettings : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QVERIFY(m_dir.isValid());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_dir.path());
    }

    void init() {
        QSettings(QStringLiteral("tracker-horas"), QStringLiteral("tracker-horas")).clear();
    }

    void defaultsToEightHours() {
        QCOMPARE(AppSettings::journeySeconds(), 8 * 3600);
    }

    void setAndGetRoundTrip() {
        AppSettings::setJourneySeconds(6 * 3600);
        QCOMPARE(AppSettings::journeySeconds(), 6 * 3600);

        AppSettings::setJourneySeconds(7 * 3600 + 30 * 60);
        QCOMPARE(AppSettings::journeySeconds(), 7 * 3600 + 30 * 60);
    }

    void clampsBelowMinimum() {
        AppSettings::setJourneySeconds(60);
        QCOMPARE(AppSettings::journeySeconds(), AppSettings::kMinJourneySeconds);
    }

    void clampsAboveMaximum() {
        AppSettings::setJourneySeconds(48 * 3600);
        QCOMPARE(AppSettings::journeySeconds(), AppSettings::kMaxJourneySeconds);
    }

    void clampsCorruptStoredValue() {
        // Valor editado à mão no arquivo de configuração.
        QSettings settings(QStringLiteral("tracker-horas"), QStringLiteral("tracker-horas"));
        settings.setValue(QStringLiteral("journeySeconds"), -100);
        settings.sync();
        QCOMPARE(AppSettings::journeySeconds(), AppSettings::kMinJourneySeconds);

        settings.setValue(QStringLiteral("journeySeconds"), QStringLiteral("banana"));
        settings.sync();
        QCOMPARE(AppSettings::journeySeconds(), AppSettings::kDefaultJourneySeconds);
    }

private:
    QTemporaryDir m_dir;
};

QTEST_MAIN(TestAppSettings)
#include "tst_appsettings.moc"
