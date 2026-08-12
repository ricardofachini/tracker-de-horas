#include "hourbank.h"

#include <QtTest>

// Testes do banco de horas: quais dias contam, o saldo mensal e o formato.
class TestHourBank : public QObject {
    Q_OBJECT

    static constexpr int kMonthlyGoal = 176 * 3600;

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

    void debitWhenUnderMonthlyGoal() {
        // 170h trabalhadas no mês contra meta de 176h → −6h.
        QCOMPARE(HourBank::monthBalance(170 * 3600, kMonthlyGoal), -6 * 3600);
    }

    void creditWhenOverMonthlyGoal() {
        // 182h30 trabalhadas → +6h30.
        QCOMPARE(HourBank::monthBalance(182 * 3600 + 30 * 60, kMonthlyGoal),
                 6 * 3600 + 30 * 60);
    }

    void exactMonthlyGoalIsZero() {
        QCOMPARE(HourBank::monthBalance(kMonthlyGoal, kMonthlyGoal), 0);
    }

    void formatShowsSign() {
        QCOMPARE(HourBank::formatBalance(3900), QStringLiteral("+1h 05min"));
        QCOMPARE(HourBank::formatBalance(-2700), QStringLiteral("−45min"));
        QCOMPARE(HourBank::formatBalance(0), QStringLiteral("0min"));
    }
};

QTEST_MAIN(TestHourBank)
#include "tst_hourbank.moc"
