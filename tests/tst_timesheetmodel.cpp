#include "timesheetmodel.h"

#include "appsettings.h"
#include "storage.h"

#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

// Testes do modelo da Folha: uma linha por dia do mês, células derivadas do
// Storage. Usa um mês fixo no passado para não depender do dia corrente.
class TestTimesheetModel : public QObject {
    Q_OBJECT

private:
    // Um dia cheio: 08:00–17:00 com 1h de almoço (8h trabalhadas).
    static void fillWorkday(DayRecord& day) {
        day.punches = {{PunchType::In, QTime(8, 0)},
                       {PunchType::BreakStart, QTime(12, 0)},
                       {PunchType::BreakEnd, QTime(13, 0)},
                       {PunchType::Out, QTime(17, 0)}};
    }

    static QString cell(const TimesheetModel& model, int row, TimesheetModel::Column column) {
        return model.index(row, column).data().toString();
    }

private slots:
    void initTestCase() {
        QCoreApplication::setApplicationName(QStringLiteral("tracker-horas-test"));
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_settingsDir.isValid());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                           m_settingsDir.path());
    }

    void init() {
        const QString dir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile::remove(dir + QStringLiteral("/data.json"));
    }

    void oneRowPerDayOfMonth() {
        Storage storage;
        TimesheetModel model(&storage);

        model.setMonth(QDate(2026, 6, 1));
        QCOMPARE(model.rowCount(), 30);
        QCOMPARE(model.columnCount(), int(TimesheetModel::ColumnCount));
        QCOMPARE(model.dateForRow(0), QDate(2026, 6, 1));
        QCOMPARE(model.dateForRow(29), QDate(2026, 6, 30));

        model.setMonth(QDate(2026, 2, 15));  // normaliza para o dia 1
        QCOMPARE(model.month(), QDate(2026, 2, 1));
        QCOMPARE(model.rowCount(), 28);
    }

    void headersMatchColumns() {
        Storage storage;
        TimesheetModel model(&storage);

        QCOMPARE(model.headerData(TimesheetModel::In, Qt::Horizontal, Qt::DisplayRole)
                     .toString(),
                 QStringLiteral("Entrada"));
        QCOMPARE(model.headerData(TimesheetModel::Balance, Qt::Horizontal, Qt::DisplayRole)
                     .toString(),
                 QStringLiteral("Saldo"));
        QCOMPARE(model.headerData(TimesheetModel::Tasks, Qt::Horizontal, Qt::DisplayRole)
                     .toString(),
                 QStringLiteral("Tarefas"));
    }

    void workdayCellsShowPunchData() {
        Storage storage;
        DayRecord& day = storage.day(QDate(2026, 6, 10));
        fillWorkday(day);
        day.tasks = {Task{QStringLiteral("a"), true, {}},
                     Task{QStringLiteral("b"), false, {}}};

        TimesheetModel model(&storage);
        model.setMonth(QDate(2026, 6, 1));
        const int row = 9;  // dia 10

        QCOMPARE(cell(model, row, TimesheetModel::In), QStringLiteral("08:00"));
        QCOMPARE(cell(model, row, TimesheetModel::Out), QStringLiteral("17:00"));
        QCOMPARE(cell(model, row, TimesheetModel::Breaks), QStringLiteral("1h 00min"));
        QCOMPARE(cell(model, row, TimesheetModel::Worked), QStringLiteral("8h 00min"));
        QCOMPARE(cell(model, row, TimesheetModel::Balance), QStringLiteral("0min"));
        QCOMPARE(cell(model, row, TimesheetModel::Tasks), QStringLiteral("2"));
    }

    void emptyDayShowsDashes() {
        Storage storage;
        TimesheetModel model(&storage);
        model.setMonth(QDate(2026, 6, 1));

        for (TimesheetModel::Column column : {TimesheetModel::In, TimesheetModel::Out,
                                              TimesheetModel::Breaks, TimesheetModel::Worked,
                                              TimesheetModel::Balance, TimesheetModel::Tasks})
            QCOMPARE(cell(model, 0, column), QStringLiteral("—"));
    }

    void balanceShowsCreditAndDebit() {
        Storage storage;
        // 10h trabalhadas → +2h; 6h trabalhadas → −2h.
        storage.day(QDate(2026, 6, 8)).punches = {{PunchType::In, QTime(8, 0)},
                                                  {PunchType::Out, QTime(18, 0)}};
        storage.day(QDate(2026, 6, 9)).punches = {{PunchType::In, QTime(8, 0)},
                                                  {PunchType::Out, QTime(14, 0)}};

        TimesheetModel model(&storage);
        model.setMonth(QDate(2026, 6, 1));

        QCOMPARE(cell(model, 7, TimesheetModel::Balance), QStringLiteral("+2h 00min"));
        QCOMPARE(cell(model, 8, TimesheetModel::Balance), QStringLiteral("−2h 00min"));
        QCOMPARE(model.monthBalanceSeconds(), 0);
        QCOMPARE(model.monthTotalSeconds(), 16 * 3600);
    }

    void manualTaskAdjustCountsInTaskColumnData() {
        // O ajuste manual não muda a contagem de tarefas, mas garante que
        // dias com tarefa sem ponto continuam aparecendo na Folha.
        Storage storage;
        storage.day(QDate(2026, 6, 12)).tasks = {
            Task{QStringLiteral("Reunião"), false, {}, 45 * 60}};

        TimesheetModel model(&storage);
        model.setMonth(QDate(2026, 6, 1));

        QCOMPARE(cell(model, 11, TimesheetModel::Tasks), QStringLiteral("1"));
        QCOMPARE(cell(model, 11, TimesheetModel::Worked), QStringLiteral("—"));
        QCOMPARE(cell(model, 11, TimesheetModel::Balance), QStringLiteral("—"));
    }

private:
    QTemporaryDir m_settingsDir;
};

QTEST_MAIN(TestTimesheetModel)
#include "tst_timesheetmodel.moc"
