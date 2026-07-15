#include "model.h"

#include <QtTest>

// Testes dos tipos de domínio: Punch, Task, DayRecord e formatação.
class TestModel : public QObject {
    Q_OBJECT

private slots:
    // ------------------------------------------------------------- Task
    void taskIsRunning() {
        Task task{QStringLiteral("t"), false, {}};
        QVERIFY(!task.isRunning());

        task.intervals.append({QTime(9, 0), QTime(10, 0)});
        QVERIFY(!task.isRunning());

        task.intervals.append({QTime(11, 0), QTime()});  // aberto
        QVERIFY(task.isRunning());
    }

    void taskSpentSeconds() {
        Task task{QStringLiteral("t"), false, {}};
        QCOMPARE(task.spentSeconds(), 0);

        task.intervals.append({QTime(9, 0), QTime(9, 30)});
        task.intervals.append({QTime(10, 0), QTime(10, 15)});
        QCOMPARE(task.spentSeconds(), 45 * 60);

        // Intervalo aberto: ignorado sem `now`, fechado por `now`.
        task.intervals.append({QTime(11, 0), QTime()});
        QCOMPARE(task.spentSeconds(), 45 * 60);
        QCOMPARE(task.spentSeconds(QTime(11, 10)), 55 * 60);
    }

    void taskSpentSecondsNeverNegative() {
        Task task{QStringLiteral("t"), false, {{QTime(10, 0), QTime(9, 0)}}};
        QCOMPARE(task.spentSeconds(), 0);
    }

    void taskSpentSecondsIncludesManualAdjust() {
        Task task{QStringLiteral("t"), false, {{QTime(9, 0), QTime(10, 0)}}, 30 * 60};
        QCOMPARE(task.spentSeconds(), 90 * 60);

        task.adjustSeconds = -30 * 60;  // correção para baixo
        QCOMPARE(task.spentSeconds(), 30 * 60);

        task.adjustSeconds = -2 * 3600;  // nunca fica negativo
        QCOMPARE(task.spentSeconds(), 0);
    }

    void taskManualAdjustAloneCounts() {
        // Tempo lançado à mão, sem nenhuma sessão de cronômetro.
        Task task{QStringLiteral("t"), false, {}, 45 * 60};
        QCOMPARE(task.spentSeconds(), 45 * 60);
        QVERIFY(!task.isRunning());
    }

    // -------------------------------------------------------- DayRecord
    void dayStatus() {
        DayRecord day;
        QCOMPARE(day.status(), DayRecord::Status::Off);

        day.punches.append({PunchType::In, QTime(8, 0)});
        QCOMPARE(day.status(), DayRecord::Status::Working);

        day.punches.append({PunchType::BreakStart, QTime(12, 0)});
        QCOMPARE(day.status(), DayRecord::Status::OnBreak);

        day.punches.append({PunchType::BreakEnd, QTime(13, 0)});
        QCOMPARE(day.status(), DayRecord::Status::Working);

        day.punches.append({PunchType::Out, QTime(17, 0)});
        QCOMPARE(day.status(), DayRecord::Status::Done);
    }

    void workedSecondsSimple() {
        DayRecord day;
        day.punches = {{PunchType::In, QTime(8, 0)}, {PunchType::Out, QTime(12, 0)}};
        QCOMPARE(day.workedSeconds(), 4 * 3600);
    }

    void workedSecondsExcludesBreaks() {
        DayRecord day;
        day.punches = {{PunchType::In, QTime(8, 0)},
                       {PunchType::BreakStart, QTime(12, 0)},
                       {PunchType::BreakEnd, QTime(13, 0)},
                       {PunchType::Out, QTime(17, 0)}};
        QCOMPARE(day.workedSeconds(), 8 * 3600);
        QCOMPARE(day.breakSeconds(), 3600);
    }

    void workedSecondsOpenShift() {
        DayRecord day;
        day.punches = {{PunchType::In, QTime(8, 0)}};
        QCOMPARE(day.workedSeconds(), 0);  // sem `now`, intervalo aberto não conta
        QCOMPARE(day.workedSeconds(QTime(9, 30)), 90 * 60);
    }

    void workedSecondsIgnoresDuplicateOpensAndCloses() {
        DayRecord day;
        day.punches = {{PunchType::In, QTime(8, 0)},
                       {PunchType::In, QTime(9, 0)},        // já trabalhando
                       {PunchType::Out, QTime(10, 0)},
                       {PunchType::Out, QTime(11, 0)}};     // já fora
        QCOMPARE(day.workedSeconds(), 2 * 3600);
    }

    void breakSecondsClosedByOut() {
        DayRecord day;
        day.punches = {{PunchType::In, QTime(8, 0)},
                       {PunchType::BreakStart, QTime(10, 0)},
                       {PunchType::Out, QTime(10, 30)}};
        QCOMPARE(day.breakSeconds(), 30 * 60);
    }

    void breakSecondsOpenBreak() {
        DayRecord day;
        day.punches = {{PunchType::In, QTime(8, 0)},
                       {PunchType::BreakStart, QTime(10, 0)}};
        QCOMPARE(day.breakSeconds(), 0);
        QCOMPARE(day.breakSeconds(QTime(10, 20)), 20 * 60);
    }

    void firstInLastOut() {
        DayRecord day;
        QVERIFY(!day.firstIn().isValid());
        QVERIFY(!day.lastOut().isValid());

        day.punches = {{PunchType::In, QTime(8, 0)},
                       {PunchType::Out, QTime(12, 0)},
                       {PunchType::In, QTime(13, 0)},
                       {PunchType::Out, QTime(17, 0)}};
        QCOMPARE(day.firstIn(), QTime(8, 0));
        QCOMPARE(day.lastOut(), QTime(17, 0));
    }

    void runningTaskIndex() {
        DayRecord day;
        QCOMPARE(day.runningTaskIndex(), -1);

        day.tasks = {Task{QStringLiteral("a"), false, {{QTime(9, 0), QTime(9, 30)}}},
                     Task{QStringLiteral("b"), false, {{QTime(10, 0), QTime()}}}};
        QCOMPARE(day.runningTaskIndex(), 1);
    }

    void pauseRunningTaskClosesOpenInterval() {
        DayRecord day;
        QVERIFY(!day.pauseRunningTask(QTime(10, 0)));  // nada correndo

        day.tasks = {Task{QStringLiteral("a"), false, {{QTime(9, 0), QTime()}}}};
        QVERIFY(day.pauseRunningTask(QTime(10, 0)));
        QVERIFY(!day.tasks[0].isRunning());
        QCOMPARE(day.tasks[0].spentSeconds(), 3600);

        QVERIFY(!day.pauseRunningTask(QTime(11, 0)));  // já pausada
    }

    void sortPunchesIsChronologicalAndStable() {
        DayRecord day;
        day.punches = {{PunchType::Out, QTime(17, 0)},
                       {PunchType::In, QTime(8, 0)},
                       {PunchType::BreakStart, QTime(12, 0)},
                       {PunchType::BreakEnd, QTime(12, 0)}};  // empate: mantém ordem
        day.sortPunches();
        QCOMPARE(day.punches[0].time, QTime(8, 0));
        QCOMPARE(day.punches[1].type, PunchType::BreakStart);
        QCOMPARE(day.punches[2].type, PunchType::BreakEnd);
        QCOMPARE(day.punches[3].time, QTime(17, 0));
    }

    // ------------------------------------------------------- Formatação
    void formatDurationLong() {
        QCOMPARE(formatDuration(0), QStringLiteral("0min"));
        QCOMPARE(formatDuration(45 * 60), QStringLiteral("45min"));
        QCOMPARE(formatDuration(3600 + 2 * 60), QStringLiteral("1h 02min"));
        QCOMPARE(formatDuration(-5), QStringLiteral("0min"));
    }

    void formatDurationWithSeconds() {
        QCOMPARE(formatDuration(6 * 3600 + 42 * 60 + 15, true), QStringLiteral("06:42:15"));
        QCOMPARE(formatDuration(0, true), QStringLiteral("00:00:00"));
    }

    void formatDurationCompactShort() {
        QCOMPARE(formatDurationCompact(8 * 3600), QStringLiteral("8h"));
        QCOMPARE(formatDurationCompact(7 * 3600 + 30 * 60), QStringLiteral("7h30"));
        QCOMPARE(formatDurationCompact(8 * 3600 + 5 * 60), QStringLiteral("8h05"));
        QCOMPARE(formatDurationCompact(45 * 60), QStringLiteral("45min"));
        QCOMPARE(formatDurationCompact(0), QStringLiteral("0min"));
        QCOMPARE(formatDurationCompact(-10), QStringLiteral("0min"));
    }

    void punchLabels() {
        QCOMPARE(punchLabel(PunchType::In), QStringLiteral("Entrada"));
        QCOMPARE(punchLabel(PunchType::BreakStart), QStringLiteral("Início da pausa"));
        QCOMPARE(punchLabel(PunchType::BreakEnd), QStringLiteral("Fim da pausa"));
        QCOMPARE(punchLabel(PunchType::Out), QStringLiteral("Saída"));
    }
};

QTEST_APPLESS_MAIN(TestModel)
#include "tst_model.moc"
