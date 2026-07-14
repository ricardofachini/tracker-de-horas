#include "hourbank.h"

#include <QtTest>

// Testes do banco de horas: quais dias contam, o saldo diário e o formato.
class TestHourBank : public QObject {
    Q_OBJECT

    static constexpr int kGoal = 8 * 3600;

    static DayRecord dayWith(std::initializer_list<Punch> punches) {
        DayRecord day;
        day.date = QDate(2026, 7, 6);
        day.punches = punches;
        return day;
    }

private slots:
    void emptyDayDoesNotCount() {
        QVERIFY(!HourBank::dayCounts(DayRecord{}));
    }

    void taskOnlyDayDoesNotCount() {
        DayRecord day;
        day.tasks = {Task{QStringLiteral("Só tarefa"), false, {}}};
        QVERIFY(!HourBank::dayCounts(day));
    }

    void punchedDayCounts() {
        QVERIFY(HourBank::dayCounts(dayWith({{PunchType::In, QTime(9, 0)}})));
    }

    void debitWhenUnderGoal() {
        const DayRecord day = dayWith({{PunchType::In, QTime(9, 0)},
                                       {PunchType::Out, QTime(15, 0)}});  // 6h
        QCOMPARE(HourBank::dayBalance(day, kGoal), -2 * 3600);
    }

    void creditWhenOverGoal() {
        const DayRecord day = dayWith({{PunchType::In, QTime(8, 0)},
                                       {PunchType::BreakStart, QTime(12, 0)},
                                       {PunchType::BreakEnd, QTime(13, 0)},
                                       {PunchType::Out, QTime(18, 30)}});  // 9h30
        QCOMPARE(HourBank::dayBalance(day, kGoal), 90 * 60);
    }

    void exactGoalIsZero() {
        const DayRecord day = dayWith({{PunchType::In, QTime(8, 0)},
                                       {PunchType::Out, QTime(16, 0)}});
        QCOMPARE(HourBank::dayBalance(day, kGoal), 0);
    }

    void openShiftUsesNow() {
        const DayRecord day = dayWith({{PunchType::In, QTime(9, 0)}});
        QCOMPARE(HourBank::dayBalance(day, kGoal, QTime(12, 0)), -5 * 3600);
        // Sem `now`, o intervalo aberto é ignorado: o dia inteiro fica em débito.
        QCOMPARE(HourBank::dayBalance(day, kGoal), -kGoal);
    }

    void formatShowsSign() {
        QCOMPARE(HourBank::formatBalance(3900), QStringLiteral("+1h 05min"));
        QCOMPARE(HourBank::formatBalance(-2700), QStringLiteral("−45min"));
        QCOMPARE(HourBank::formatBalance(0), QStringLiteral("0min"));
    }
};

QTEST_MAIN(TestHourBank)
#include "tst_hourbank.moc"
