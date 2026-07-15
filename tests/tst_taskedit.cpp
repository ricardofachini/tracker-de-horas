#include "taskedit.h"

#include "testutils.h"

#include <QLineEdit>
#include <QRadioButton>
#include <QTimeEdit>
#include <QtTest>

// Testes dos diálogos de tarefa: ajuste manual do tempo dedicado e a
// escolha da tarefa ao registrar a entrada. Os diálogos são modais (exec),
// então cada teste agenda a interação com onNextModal antes de abri-los.
class TestTaskEdit : public QObject {
    Q_OBJECT

private slots:
    // ------------------------------------------------ editTaskTimeDialog
    void editTimeSetsManualAdjust() {
        Task task{QStringLiteral("Relatório"), false, {{QTime(9, 0), QTime(10, 0)}}};

        onNextModal(this, [](QDialog* dialog) {
            auto* timeEdit = dialog->findChild<QTimeEdit*>();
            QVERIFY(timeEdit);
            QCOMPARE(timeEdit->time(), QTime(1, 0));  // total atual do cronômetro
            timeEdit->setTime(QTime(2, 30));
            clickButton(dialog, QStringLiteral("Salvar"));
        });
        QVERIFY(editTaskTimeDialog(nullptr, task));

        QCOMPARE(task.adjustSeconds, 3600 + 30 * 60);
        QCOMPARE(task.spentSeconds(), 2 * 3600 + 30 * 60);
    }

    void editTimeCanReduceBelowChronometer() {
        Task task{QStringLiteral("t"), false, {{QTime(9, 0), QTime(10, 0)}}};

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QTimeEdit*>()->setTime(QTime(0, 15));
            clickButton(dialog, QStringLiteral("Salvar"));
        });
        QVERIFY(editTaskTimeDialog(nullptr, task));

        QCOMPARE(task.adjustSeconds, -45 * 60);
        QCOMPARE(task.spentSeconds(), 15 * 60);
    }

    void editTimeRegistersTimeWithoutChronometer() {
        Task task{QStringLiteral("Reunião não cronometrada"), false, {}};

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QTimeEdit*>()->setTime(QTime(1, 30));
            clickButton(dialog, QStringLiteral("Salvar"));
        });
        QVERIFY(editTaskTimeDialog(nullptr, task));

        QCOMPARE(task.spentSeconds(), 90 * 60);
        QVERIFY(task.intervals.isEmpty());  // lançamento manual não cria sessões
    }

    void editTimeUnchangedValueIsNoop() {
        // O campo mostra o total arredondado ao minuto; salvar sem mexer
        // não pode zerar os segundos do cronômetro.
        Task task{QStringLiteral("t"), false, {{QTime(9, 0), QTime(9, 45, 30)}}};

        onNextModal(this, [](QDialog* dialog) {
            clickButton(dialog, QStringLiteral("Salvar"));
        });
        QVERIFY(!editTaskTimeDialog(nullptr, task));

        QCOMPARE(task.adjustSeconds, 0);
        QCOMPARE(task.spentSeconds(), 45 * 60 + 30);
    }

    void editTimeCancelKeepsTask() {
        Task task{QStringLiteral("t"), false, {}, 600};

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QTimeEdit*>()->setTime(QTime(5, 0));
            clickButton(dialog, QStringLiteral("Cancelar"));
        });
        QVERIFY(!editTaskTimeDialog(nullptr, task));

        QCOMPARE(task.adjustSeconds, 600);
    }

    // -------------------------------------------- askTaskOnPunchInDialog
    void askTaskStartsChosenPendingTask() {
        DayRecord day;
        day.tasks = {Task{QStringLiteral("Concluída"), true, {}},
                     Task{QStringLiteral("Pendente"), false, {}}};

        onNextModal(this, [](QDialog* dialog) {
            const auto radios = dialog->findChildren<QRadioButton*>();
            QCOMPARE(radios.size(), 2);  // pendente + "criar nova"; concluída fica de fora
            QCOMPARE(radios.first()->text(), QStringLiteral("Pendente"));
            QVERIFY(radios.first()->isChecked());  // pré-selecionada
            clickButton(dialog, QStringLiteral("Começar"));
        });
        QVERIFY(askTaskOnPunchInDialog(nullptr, day, QTime(8, 0)));

        QCOMPARE(day.runningTaskIndex(), 1);
        QCOMPARE(day.tasks[1].intervals.last().start, QTime(8, 0));
    }

    void askTaskCreatesNewTask() {
        DayRecord day;  // sem tarefas: só a opção de criar, já selecionada

        onNextModal(this, [](QDialog* dialog) {
            auto* radio = dialog->findChild<QRadioButton*>();
            QVERIFY(radio->isChecked());
            auto* edit = dialog->findChild<QLineEdit*>();
            QVERIFY(edit->isEnabled());
            edit->setText(QStringLiteral("  Deploy  "));
            clickButton(dialog, QStringLiteral("Começar"));
        });
        QVERIFY(askTaskOnPunchInDialog(nullptr, day, QTime(9, 0)));

        QCOMPARE(day.tasks.size(), 1);
        QCOMPARE(day.tasks[0].text, QStringLiteral("Deploy"));  // sem espaços
        QVERIFY(day.tasks[0].isRunning());
        QCOMPARE(day.tasks[0].intervals.last().start, QTime(9, 0));
    }

    void askTaskSkipChangesNothing() {
        DayRecord day;
        day.tasks = {Task{QStringLiteral("Pendente"), false, {}}};

        onNextModal(this, [](QDialog* dialog) {
            clickButton(dialog, QStringLiteral("Agora não"));
        });
        QVERIFY(!askTaskOnPunchInDialog(nullptr, day, QTime(8, 0)));

        QCOMPARE(day.runningTaskIndex(), -1);
    }

    void askTaskEmptyNewNameIsSkip() {
        DayRecord day;

        onNextModal(this, [](QDialog* dialog) {
            clickButton(dialog, QStringLiteral("Começar"));  // campo vazio
        });
        QVERIFY(!askTaskOnPunchInDialog(nullptr, day, QTime(8, 0)));

        QVERIFY(day.tasks.isEmpty());
    }

    void askTaskPreemptsRunningTask() {
        DayRecord day;
        day.tasks = {Task{QStringLiteral("Antiga"), false, {{QTime(7, 0), QTime()}}},
                     Task{QStringLiteral("Nova"), false, {}}};

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChildren<QRadioButton*>()[1]->setChecked(true);  // "Nova"
            clickButton(dialog, QStringLiteral("Começar"));
        });
        QVERIFY(askTaskOnPunchInDialog(nullptr, day, QTime(8, 0)));

        QVERIFY(!day.tasks[0].isRunning());
        QCOMPARE(day.tasks[0].spentSeconds(), 3600);  // fechada na hora da entrada
        QVERIFY(day.tasks[1].isRunning());
    }
};

QTEST_MAIN(TestTaskEdit)
#include "tst_taskedit.moc"
