#include "storage.h"

#include <QStandardPaths>
#include <QtTest>

// Testes da persistência em JSON e da ponte de meia-noite. O modo de teste
// do QStandardPaths redireciona AppDataLocation para fora dos dados reais.
class TestStorage : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setApplicationName(QStringLiteral("tracker-horas-test"));
        QStandardPaths::setTestModeEnabled(true);
    }

    void init() {  // cada teste parte de um arquivo limpo
        const QString dir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile::remove(dir + QStringLiteral("/data.json"));
    }

    void findReturnsNullForMissingDay() {
        Storage storage;
        QCOMPARE(storage.find(QDate(2026, 7, 1)), nullptr);
    }

    void dayCreatesRecordWithDate() {
        Storage storage;
        const QDate date(2026, 7, 1);
        DayRecord& record = storage.day(date);
        QCOMPARE(record.date, date);
        QVERIFY(storage.find(date) != nullptr);
    }

    void allDaysNewestFirst() {
        Storage storage;
        storage.day(QDate(2026, 7, 1)).punches.append({PunchType::In, QTime(8, 0)});
        storage.day(QDate(2026, 7, 3)).punches.append({PunchType::In, QTime(8, 0)});
        storage.day(QDate(2026, 7, 2)).punches.append({PunchType::In, QTime(8, 0)});

        const QList<DayRecord> days = storage.allDays();
        QCOMPARE(days.size(), 3);
        QCOMPARE(days[0].date, QDate(2026, 7, 3));
        QCOMPARE(days[1].date, QDate(2026, 7, 2));
        QCOMPARE(days[2].date, QDate(2026, 7, 1));
    }

    void saveLoadRoundTrip() {
        const QDate date(2026, 7, 6);
        {
            Storage storage;
            DayRecord& day = storage.day(date);
            day.punches = {{PunchType::In, QTime(8, 0, 30)},
                           {PunchType::BreakStart, QTime(12, 0)},
                           {PunchType::BreakEnd, QTime(13, 0)},
                           {PunchType::Out, QTime(17, 15)}};
            Task done{QStringLiteral("Relatório"), true,
                      {{QTime(9, 0), QTime(10, 30)}}};
            Task running{QStringLiteral("Revisão"), false,
                         {{QTime(11, 0), QTime()}}};  // intervalo aberto
            day.tasks = {done, running};
            storage.save();
        }

        Storage reloaded;
        const DayRecord* day = reloaded.find(date);
        QVERIFY(day != nullptr);

        QCOMPARE(day->punches.size(), 4);
        QCOMPARE(day->punches[0].type, PunchType::In);
        QCOMPARE(day->punches[0].time, QTime(8, 0, 30));  // segundos preservados
        QCOMPARE(day->punches[3].type, PunchType::Out);
        QCOMPARE(day->punches[3].time, QTime(17, 15));

        QCOMPARE(day->tasks.size(), 2);
        QCOMPARE(day->tasks[0].text, QStringLiteral("Relatório"));
        QVERIFY(day->tasks[0].done);
        QCOMPARE(day->tasks[0].spentSeconds(), 90 * 60);
        QVERIFY(day->tasks[1].isRunning());  // "end" ausente volta como aberto
    }

    void manualAdjustRoundTrip() {
        const QDate date(2026, 7, 9);
        {
            Storage storage;
            storage.day(date).tasks = {
                Task{QStringLiteral("Só ajuste"), false, {}, 45 * 60},
                Task{QStringLiteral("Sem ajuste"), false, {{QTime(9, 0), QTime(9, 30)}}}};
            storage.save();
        }

        Storage reloaded;
        const DayRecord* day = reloaded.find(date);
        QVERIFY(day != nullptr);
        QCOMPARE(day->tasks[0].adjustSeconds, 45 * 60);
        QCOMPARE(day->tasks[0].spentSeconds(), 45 * 60);
        QCOMPARE(day->tasks[1].adjustSeconds, 0);  // ausente no JSON volta zerado
        QCOMPARE(day->tasks[1].spentSeconds(), 30 * 60);
    }

    void emptyDaysAreNotPersisted() {
        const QDate date(2026, 7, 7);
        {
            Storage storage;
            storage.day(date);  // criado mas sem registros
            storage.save();
        }
        Storage reloaded;
        QCOMPARE(reloaded.find(date), nullptr);
    }

    // -------------------------------------------------- bridgeMidnight
    void bridgeMidnightWhileWorking() {
        Storage storage;
        const QDate date(2026, 7, 8);
        storage.day(date).punches = {{PunchType::In, QTime(22, 0)}};

        QVERIFY(storage.bridgeMidnight(date));

        const DayRecord* previous = storage.find(date);
        QCOMPARE(previous->status(), DayRecord::Status::Done);
        QCOMPARE(previous->lastOut(), QTime(23, 59, 59));
        QCOMPARE(previous->workedSeconds(), 2 * 3600 - 1);  // 22:00 → 23:59:59

        const DayRecord* next = storage.find(date.addDays(1));
        QVERIFY(next != nullptr);
        QCOMPARE(next->status(), DayRecord::Status::Working);
        QCOMPARE(next->firstIn(), QTime(0, 0));
    }

    void bridgeMidnightWhileOnBreak() {
        Storage storage;
        const QDate date(2026, 7, 8);
        storage.day(date).punches = {{PunchType::In, QTime(20, 0)},
                                     {PunchType::BreakStart, QTime(23, 0)}};

        QVERIFY(storage.bridgeMidnight(date));

        const DayRecord* previous = storage.find(date);
        QCOMPARE(previous->status(), DayRecord::Status::Done);
        QCOMPARE(previous->workedSeconds(), 3 * 3600);       // 20:00 → 23:00
        QCOMPARE(previous->breakSeconds(), 3600 - 1);        // 23:00 → 23:59:59

        // O dia seguinte reabre já em pausa.
        const DayRecord* next = storage.find(date.addDays(1));
        QCOMPARE(next->status(), DayRecord::Status::OnBreak);
        QCOMPARE(next->punches.size(), 2);
        QCOMPARE(next->punches[0].type, PunchType::In);
        QCOMPARE(next->punches[1].type, PunchType::BreakStart);
    }

    void bridgeMidnightPausesRunningTask() {
        Storage storage;
        const QDate date(2026, 7, 8);
        DayRecord& day = storage.day(date);
        day.punches = {{PunchType::In, QTime(22, 0)}};
        day.tasks = {Task{QStringLiteral("Deploy"), false,
                          {{QTime(22, 30), QTime()}}}};

        QVERIFY(storage.bridgeMidnight(date));

        const DayRecord* previous = storage.find(date);
        QVERIFY(!previous->tasks[0].isRunning());
        QCOMPARE(previous->tasks[0].spentSeconds(), 90 * 60 - 1);  // até 23:59:59
    }

    void bridgeMidnightKeepsNextDayPunchesInOrder() {
        Storage storage;
        const QDate date(2026, 7, 8);
        storage.day(date).punches = {{PunchType::In, QTime(22, 0)}};
        // A saída da madrugada já tinha sido lançada no dia seguinte.
        storage.day(date.addDays(1)).punches = {{PunchType::Out, QTime(2, 0)}};

        QVERIFY(storage.bridgeMidnight(date));

        const DayRecord* next = storage.find(date.addDays(1));
        QCOMPARE(next->punches.size(), 2);
        QCOMPARE(next->punches[0].type, PunchType::In);   // 00:00 entra antes
        QCOMPARE(next->punches[1].type, PunchType::Out);
        QCOMPARE(next->workedSeconds(), 2 * 3600);
        QCOMPARE(next->status(), DayRecord::Status::Done);
    }

    void bridgeMidnightNoopWhenShiftClosed() {
        Storage storage;
        const QDate date(2026, 7, 8);
        storage.day(date).punches = {{PunchType::In, QTime(8, 0)},
                                     {PunchType::Out, QTime(17, 0)}};

        QVERIFY(!storage.bridgeMidnight(date));
        QCOMPARE(storage.find(date)->punches.size(), 2);
        QCOMPARE(storage.find(date.addDays(1)), nullptr);
    }

    void bridgeMidnightNoopWhenDayMissing() {
        Storage storage;
        QVERIFY(!storage.bridgeMidnight(QDate(2026, 7, 8)));
    }
};

QTEST_MAIN(TestStorage)
#include "tst_storage.moc"
