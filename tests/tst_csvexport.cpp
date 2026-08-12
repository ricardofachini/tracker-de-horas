#include "appsettings.h"
#include "csvexport.h"
#include "storage.h"

#include <QLocale>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

// Testes do conteúdo do CSV mensal (monthCsv — a parte pura, sem diálogos).
// Usa um mês passado fixo para o resultado ser determinístico. O QSettings
// é redirecionado (a linha "Saldo do mês" depende da meta mensal, padrão 176h).
class TestCsvExport : public QObject {
    Q_OBJECT

    static QStringList csvLines(const Storage& storage) {
        return monthCsv(storage, QDate(2026, 6, 1)).split(QStringLiteral("\r\n"));
    }

private slots:
    void initTestCase() {
        QCoreApplication::setApplicationName(QStringLiteral("tracker-horas-test"));
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_settingsDir.isValid());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                           m_settingsDir.path());
        QLocale::setDefault(QLocale(QLocale::Portuguese, QLocale::Brazil));
    }

    void init() {
        const QString dir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile::remove(dir + QStringLiteral("/data.json"));
    }

    void headerAndShape() {
        Storage storage;
        const QStringList lines = csvLines(storage);
        QCOMPARE(lines[0],
                 QStringLiteral("Data;Dia;Entrada;Saída;Pausas;Trabalhadas;Tarefas"));
        // 1 cabeçalho + 30 dias de junho + total + saldo + vazio após o último \r\n.
        QCOMPARE(lines.size(), 34);
        QVERIFY(lines[31].startsWith(QStringLiteral("Total do mês")));
        QVERIFY(lines[32].startsWith(QStringLiteral("Saldo do mês")));
        QCOMPARE(lines[33], QString());
    }

    void emptyDayHasEmptyFields() {
        Storage storage;
        const QStringList lines = csvLines(storage);
        QCOMPARE(lines[1], QStringLiteral("01/06/2026;segunda-feira;;;;;"));
    }

    void fullDayRow() {
        Storage storage;
        DayRecord& day = storage.day(QDate(2026, 6, 15));  // segunda-feira
        day.punches = {{PunchType::In, QTime(8, 0)},
                       {PunchType::BreakStart, QTime(12, 0)},
                       {PunchType::BreakEnd, QTime(13, 0)},
                       {PunchType::Out, QTime(17, 0)}};
        day.tasks = {Task{QStringLiteral("Relatório"), true,
                          {{QTime(8, 0), QTime(9, 30)}}}};

        const QStringList lines = csvLines(storage);
        // Sem coluna de saldo por dia: trabalhadas seguidas das tarefas.
        QCOMPARE(lines[15],
                 QStringLiteral("15/06/2026;segunda-feira;08:00;17:00;01:00:00;"
                                "08:00:00;Relatório (1h 30min) ✓"));
    }

    void totalSumsAllDays() {
        Storage storage;
        AppSettings::setMonthlyGoalSeconds(10 * 3600);  // meta pequena p/ o teste
        storage.day(QDate(2026, 6, 15)).punches = {{PunchType::In, QTime(8, 0)},
                                                   {PunchType::Out, QTime(12, 0)}};
        storage.day(QDate(2026, 6, 16)).punches = {{PunchType::In, QTime(9, 0)},
                                                   {PunchType::Out, QTime(12, 30)}};

        const QStringList lines = csvLines(storage);
        // 4h + 3h30 = 7h30 trabalhadas; saldo = 7h30 − 10h = −02:30:00.
        QCOMPARE(lines[31], QStringLiteral("Total do mês;;;;;07:30:00;"));
        QCOMPARE(lines[32], QStringLiteral("Saldo do mês;;;;;-02:30:00;"));
    }

    void monthlyBalanceRowShowsCredit() {
        Storage storage;
        AppSettings::setMonthlyGoalSeconds(10 * 3600);  // meta pequena p/ o teste
        storage.day(QDate(2026, 6, 15)).punches = {{PunchType::In, QTime(8, 0)},
                                                   {PunchType::Out, QTime(17, 0)}};  // 9h
        storage.day(QDate(2026, 6, 16)).punches = {{PunchType::In, QTime(9, 0)},
                                                   {PunchType::Out, QTime(16, 30)}};  // 7h30

        const QStringList lines = csvLines(storage);
        // 9h + 7h30 = 16h30 trabalhadas; saldo = 16h30 − 10h = +06:30:00.
        QCOMPARE(lines[32], QStringLiteral("Saldo do mês;;;;;+06:30:00;"));
    }

    void tasksWithSeparatorAreQuoted() {
        Storage storage;
        DayRecord& day = storage.day(QDate(2026, 6, 15));
        day.punches = {{PunchType::In, QTime(8, 0)}, {PunchType::Out, QTime(9, 0)}};
        day.tasks = {Task{QStringLiteral("Reunião; planejamento"), false, {}},
                     Task{QStringLiteral("Outra"), false, {}}};

        const QStringList lines = csvLines(storage);
        QVERIFY(lines[15].endsWith(
            QStringLiteral(";\"Reunião; planejamento | Outra\"")));
    }

    void quotesInsideFieldsAreDoubled() {
        Storage storage;
        DayRecord& day = storage.day(QDate(2026, 6, 15));
        day.punches = {{PunchType::In, QTime(8, 0)}, {PunchType::Out, QTime(9, 0)}};
        day.tasks = {Task{QStringLiteral("Corrigir \"bug\" antigo"), false, {}}};

        const QStringList lines = csvLines(storage);
        QVERIFY(lines[15].endsWith(
            QStringLiteral(";\"Corrigir \"\"bug\"\" antigo\"")));
    }

    void openTaskIntervalIsIgnoredInPastMonth() {
        Storage storage;
        DayRecord& day = storage.day(QDate(2026, 6, 15));
        day.punches = {{PunchType::In, QTime(8, 0)}, {PunchType::Out, QTime(9, 0)}};
        day.tasks = {Task{QStringLiteral("Esquecida"), false,
                          {{QTime(8, 0), QTime()}}}};  // ficou aberta

        const QStringList lines = csvLines(storage);
        // Sem "(tempo)": o intervalo aberto de um dia passado não conta.
        QVERIFY(lines[15].endsWith(QStringLiteral(";Esquecida")));
    }

private:
    QTemporaryDir m_settingsDir;
};

QTEST_MAIN(TestCsvExport)
#include "tst_csvexport.moc"
