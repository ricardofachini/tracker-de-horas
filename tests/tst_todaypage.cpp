#include "todaypage.h"

#include "storage.h"
#include "testutils.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QListWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimeEdit>
#include <QToolButton>
#include <QtTest>

// Testes de UI da página Hoje: botões de ponto, tarefas com cronômetro
// preemptivo, ajuste manual de tempo e o diálogo de tarefa na entrada.
// A página opera sobre o dia corrente; os botões são acionados por click()
// (funciona no modo offscreen, sem geometria calculada) e as linhas das
// listas são alcançadas via itemWidget().
class TestTodayPage : public QObject {
    Q_OBJECT

private:
    // O refresh disparado pelos botões das linhas é adiado para o próximo
    // ciclo do event loop (scheduleRefresh) — os testes precisam esperá-lo.
    static void waitRefresh() { QTest::qWait(20); }

    static QListWidget* taskList(TodayPage& page) {
        return page.findChild<QListWidget*>(QStringLiteral("taskList"));
    }

    // Botões de ação de uma linha de tarefa: [ajustar tempo, play/pause].
    static QList<QToolButton*> taskRowButtons(TodayPage& page, int row) {
        QListWidget* list = taskList(page);
        return list->itemWidget(list->item(row))->findChildren<QToolButton*>();
    }

private slots:
    void initTestCase() {
        QCoreApplication::setApplicationName(QStringLiteral("tracker-horas-test"));
        QStandardPaths::setTestModeEnabled(true);
        // AppSettings/Theme usam QSettings: redirecionado para não tocar
        // a configuração real do usuário.
        QVERIFY(m_settingsDir.isValid());
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                           m_settingsDir.path());
    }

    void init() {  // cada teste parte de um arquivo limpo
        const QString dir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile::remove(dir + QStringLiteral("/data.json"));
    }

    // ------------------------------------------------- registros de ponto
    void startsOffWithOnlyEntryButton() {
        Storage storage;
        TodayPage page(&storage);

        QVERIFY(findButton(&page, QStringLiteral("Registrar entrada"))->isVisibleTo(&page));
        QVERIFY(!findButton(&page, QStringLiteral("Iniciar pausa"))->isVisibleTo(&page));
        QVERIFY(!findButton(&page, QStringLiteral("Retomar trabalho"))->isVisibleTo(&page));
        QVERIFY(!findButton(&page, QStringLiteral("Encerrar expediente"))->isVisibleTo(&page));
    }

    void punchInRegistersEntryAndOffersTask() {
        Storage storage;
        TodayPage page(&storage);

        onNextModal(this, [](QDialog* dialog) {
            clickButton(dialog, QStringLiteral("Agora não"));
        });
        clickButton(&page, QStringLiteral("Registrar entrada"));

        const DayRecord& day = storage.day(QDate::currentDate());
        QCOMPARE(day.punches.size(), 1);
        QCOMPARE(day.punches[0].type, PunchType::In);
        QCOMPARE(day.status(), DayRecord::Status::Working);
        QVERIFY(day.tasks.isEmpty());  // "agora não" não cria tarefa

        QVERIFY(findButton(&page, QStringLiteral("Iniciar pausa"))->isVisibleTo(&page));
        QVERIFY(findButton(&page, QStringLiteral("Encerrar expediente"))->isVisibleTo(&page));
        QVERIFY(!findButton(&page, QStringLiteral("Registrar entrada"))->isVisibleTo(&page));
    }

    void punchInCanCreateAndStartTask() {
        Storage storage;
        TodayPage page(&storage);

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QLineEdit*>()->setText(QStringLiteral("Revisão"));
            clickButton(dialog, QStringLiteral("Começar"));
        });
        clickButton(&page, QStringLiteral("Registrar entrada"));

        const DayRecord& day = storage.day(QDate::currentDate());
        QCOMPARE(day.tasks.size(), 1);
        QCOMPARE(day.tasks[0].text, QStringLiteral("Revisão"));
        QVERIFY(day.tasks[0].isRunning());
        // O cronômetro começa na hora da entrada, não na do diálogo.
        QCOMPARE(day.tasks[0].intervals.last().start, day.punches[0].time);
        QCOMPARE(taskList(page)->count(), 1);
    }

    void fullDayFlowPausesTaskOnBreakAndOut() {
        Storage storage;
        TodayPage page(&storage);

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QLineEdit*>()->setText(QStringLiteral("Deploy"));
            clickButton(dialog, QStringLiteral("Começar"));
        });
        clickButton(&page, QStringLiteral("Registrar entrada"));

        DayRecord& day = storage.day(QDate::currentDate());
        clickButton(&page, QStringLiteral("Iniciar pausa"));
        QCOMPARE(day.status(), DayRecord::Status::OnBreak);
        QVERIFY(!day.tasks[0].isRunning());  // pausar também pausa a tarefa

        clickButton(&page, QStringLiteral("Retomar trabalho"));
        QCOMPARE(day.status(), DayRecord::Status::Working);

        clickButton(&page, QStringLiteral("Encerrar expediente"));
        QCOMPARE(day.status(), DayRecord::Status::Done);
        QCOMPARE(day.punches.size(), 4);
        QVERIFY(findButton(&page, QStringLiteral("Registrar nova entrada"))
                    ->isVisibleTo(&page));
    }

    void removesPunchFromListWithConfirmation() {
        Storage storage;
        DayRecord& day = storage.day(QDate::currentDate());
        day.punches = {{PunchType::In, QTime(8, 0)}};
        TodayPage page(&storage);

        auto* list = page.findChild<QListWidget*>(QStringLiteral("punchList"));
        QCOMPARE(list->count(), 1);
        // Cada linha tem [editar, remover]; a lixeira abre a confirmação.
        const auto buttons =
            list->itemWidget(list->item(0))->findChildren<QToolButton*>();
        onNextModal(this, [](QDialog* confirm) {
            clickButton(confirm, QStringLiteral("Remover"));
        });
        buttons.last()->click();
        waitRefresh();

        QVERIFY(day.punches.isEmpty());
        QCOMPARE(list->count(), 0);
    }

    // ---------------------------------------------------------- tarefas
    void addsTaskWithEnterOnInput() {
        Storage storage;
        TodayPage page(&storage);

        auto* input = page.findChild<QLineEdit*>();
        input->setText(QStringLiteral("Escrever testes"));
        QTest::keyClick(input, Qt::Key_Return);

        const DayRecord& day = storage.day(QDate::currentDate());
        QCOMPARE(day.tasks.size(), 1);
        QCOMPARE(day.tasks[0].text, QStringLiteral("Escrever testes"));
        QVERIFY(input->text().isEmpty());  // campo pronto para a próxima
        QCOMPARE(taskList(page)->count(), 1);
    }

    void playPreemptsAndPauseStops() {
        Storage storage;
        DayRecord& day = storage.day(QDate::currentDate());
        day.tasks = {Task{QStringLiteral("a"), false, {}},
                     Task{QStringLiteral("b"), false, {}}};
        TodayPage page(&storage);

        taskRowButtons(page, 0).last()->click();  // play na "a"
        waitRefresh();
        QCOMPARE(day.runningTaskIndex(), 0);

        taskRowButtons(page, 1).last()->click();  // play na "b" pausa a "a"
        waitRefresh();
        QCOMPARE(day.runningTaskIndex(), 1);
        QVERIFY(!day.tasks[0].isRunning());

        taskRowButtons(page, 1).last()->click();  // agora o botão é pause
        waitRefresh();
        QCOMPARE(day.runningTaskIndex(), -1);
    }

    void editsTaskTimeFromRow() {
        Storage storage;
        DayRecord& day = storage.day(QDate::currentDate());
        day.tasks = {Task{QStringLiteral("a"), false, {{QTime(9, 0), QTime(10, 0)}}}};
        TodayPage page(&storage);

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QTimeEdit*>()->setTime(QTime(2, 0));
            clickButton(dialog, QStringLiteral("Salvar"));
        });
        taskRowButtons(page, 0).first()->click();  // lápis: ajustar tempo
        waitRefresh();

        QCOMPARE(day.tasks[0].spentSeconds(), 2 * 3600);
        QCOMPARE(day.tasks[0].adjustSeconds, 3600);
    }

    void doneStopsChronometerAndClearRemoves() {
        Storage storage;
        DayRecord& day = storage.day(QDate::currentDate());
        day.tasks = {Task{QStringLiteral("a"), false, {{QTime(9, 0), QTime()}}}};
        TodayPage page(&storage);

        taskList(page)->itemWidget(taskList(page)->item(0))
            ->findChild<QCheckBox*>()->click();
        waitRefresh();
        QVERIFY(day.tasks[0].done);
        QVERIFY(!day.tasks[0].isRunning());  // concluir também para o cronômetro

        clickButton(&page, QStringLiteral("Limpar concluídas"));
        QVERIFY(day.tasks.isEmpty());
        QCOMPARE(taskList(page)->count(), 0);
    }

private:
    QTemporaryDir m_settingsDir;
};

QTEST_MAIN(TestTodayPage)
#include "tst_todaypage.moc"
