#include "punchedit.h"

#include "storage.h"
#include "testutils.h"

#include <QComboBox>
#include <QListWidget>
#include <QRadioButton>
#include <QStandardPaths>
#include <QTimeEdit>
#include <QToolButton>
#include <QtTest>

// Testes dos diálogos de registro de ponto: edição de um registro, a
// confirmação de remoção, o editor de dias inteiros (Folha) e a resolução
// de turno aberto na véspera. Diálogos modais são dirigidos por onNextModal.
class TestPunchEdit : public QObject {
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

    // --------------------------------------------------- editPunchDialog
    void editPunchUpdatesTypeAndTime() {
        Punch punch{PunchType::In, QTime(8, 0, 45)};

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QComboBox*>()->setCurrentIndex(int(PunchType::Out));
            dialog->findChild<QTimeEdit*>()->setTime(QTime(17, 30));
            clickButton(dialog, QStringLiteral("Salvar"));
        });
        QVERIFY(editPunchDialog(nullptr, punch));

        QCOMPARE(punch.type, PunchType::Out);
        QCOMPARE(punch.time, QTime(17, 30));  // horário mudou: segundos zerados
    }

    void editPunchKeepsSecondsWhenTimeUntouched() {
        Punch punch{PunchType::In, QTime(8, 0, 45)};

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QComboBox*>()->setCurrentIndex(int(PunchType::BreakStart));
            clickButton(dialog, QStringLiteral("Salvar"));  // só o tipo
        });
        QVERIFY(editPunchDialog(nullptr, punch));

        QCOMPARE(punch.type, PunchType::BreakStart);
        QCOMPARE(punch.time, QTime(8, 0, 45));  // segundos preservados
    }

    void editPunchCancelKeepsPunch() {
        Punch punch{PunchType::In, QTime(8, 0)};

        onNextModal(this, [](QDialog* dialog) {
            dialog->findChild<QTimeEdit*>()->setTime(QTime(9, 0));
            clickButton(dialog, QStringLiteral("Cancelar"));
        });
        QVERIFY(!editPunchDialog(nullptr, punch));

        QCOMPARE(punch.type, PunchType::In);
        QCOMPARE(punch.time, QTime(8, 0));
    }

    // ----------------------------------------------- confirmRemoveDialog
    void confirmRemoveReturnsUserChoice() {
        onNextModal(this, [](QDialog* dialog) {
            clickButton(dialog, QStringLiteral("Remover"));
        });
        QVERIFY(confirmRemoveDialog(nullptr, QStringLiteral("Remover?"),
                                    QStringLiteral("detalhe")));

        onNextModal(this, [](QDialog* dialog) {
            clickButton(dialog, QStringLiteral("Cancelar"));
        });
        QVERIFY(!confirmRemoveDialog(nullptr, QStringLiteral("Remover?"),
                                     QStringLiteral("detalhe")));
    }

    // ----------------------------------------------- DayPunchesDialog
    void dayPunchesRemovesWithConfirmation() {
        Storage storage;
        const QDate date(2026, 7, 11);
        storage.day(date).punches = {{PunchType::In, QTime(8, 0)},
                                     {PunchType::Out, QTime(17, 0)}};

        DayPunchesDialog dialog(&storage, date);
        auto* list = dialog.findChild<QListWidget*>();
        QVERIFY(list);
        QCOMPARE(list->count(), 2);

        // Cada linha tem [editar, remover]; a lixeira abre a confirmação.
        const auto buttons =
            list->itemWidget(list->item(0))->findChildren<QToolButton*>();
        QCOMPARE(buttons.size(), 2);
        onNextModal(this, [](QDialog* confirm) {
            clickButton(confirm, QStringLiteral("Remover"));
        });
        buttons.last()->click();

        QVERIFY(dialog.changed());
        QCOMPARE(storage.find(date)->punches.size(), 1);
        QCOMPARE(storage.find(date)->punches[0].type, PunchType::Out);
    }

    void dayPunchesAddsPunchSorted() {
        Storage storage;
        const QDate date(2026, 7, 11);
        storage.day(date).punches = {{PunchType::Out, QTime(17, 0)}};

        DayPunchesDialog dialog(&storage, date);
        onNextModal(this, [](QDialog* editor) {
            editor->findChild<QComboBox*>()->setCurrentIndex(int(PunchType::In));
            editor->findChild<QTimeEdit*>()->setTime(QTime(8, 30));
            clickButton(editor, QStringLiteral("Salvar"));
        });
        clickButton(&dialog, QStringLiteral("Adicionar registro"));

        QVERIFY(dialog.changed());
        const DayRecord* day = storage.find(date);
        QCOMPARE(day->punches.size(), 2);
        QCOMPARE(day->punches[0].time, QTime(8, 30));  // entra em ordem cronológica
        QCOMPARE(day->punches[0].type, PunchType::In);
    }

    // ------------------------------------------- resolveOpenShiftDialog
    void resolveStillWorkingBridgesToNextDay() {
        Storage storage;
        const QDate date(2026, 7, 10);
        storage.day(date).punches = {{PunchType::In, QTime(22, 0)}};

        onNextModal(this, [](QDialog* dialog) {
            // A primeira opção ("ainda estou trabalhando") já vem marcada.
            clickButton(dialog, QStringLiteral("Aplicar"));
        });
        QVERIFY(resolveOpenShiftDialog(&storage, date, nullptr));

        QCOMPARE(storage.find(date)->status(), DayRecord::Status::Done);
        QCOMPARE(storage.find(date)->lastOut(), QTime(23, 59, 59));
        QCOMPARE(storage.find(date.addDays(1))->status(), DayRecord::Status::Working);
    }

    void resolveWorkedUntilClosesNextDay() {
        Storage storage;
        const QDate date(2026, 7, 10);
        storage.day(date).punches = {{PunchType::In, QTime(22, 0)}};

        onNextModal(this, [](QDialog* dialog) {
            const auto radios = dialog->findChildren<QRadioButton*>();
            radios[1]->setChecked(true);  // "trabalhei até as"
            dialog->findChild<QTimeEdit*>()->setTime(QTime(2, 30));
            clickButton(dialog, QStringLiteral("Aplicar"));
        });
        QVERIFY(resolveOpenShiftDialog(&storage, date, nullptr));

        const DayRecord* next = storage.find(date.addDays(1));
        QCOMPARE(next->status(), DayRecord::Status::Done);
        QCOMPARE(next->workedSeconds(), 2 * 3600 + 30 * 60);  // 00:00 → 02:30
    }

    void resolveLaterKeepsShiftOpen() {
        Storage storage;
        const QDate date(2026, 7, 10);
        storage.day(date).punches = {{PunchType::In, QTime(22, 0)}};

        onNextModal(this, [](QDialog* dialog) {
            clickButton(dialog, QStringLiteral("Deixar para depois"));
        });
        QVERIFY(!resolveOpenShiftDialog(&storage, date, nullptr));

        QCOMPARE(storage.find(date)->status(), DayRecord::Status::Working);
        QCOMPARE(storage.find(date.addDays(1)), nullptr);
    }
};

QTEST_MAIN(TestPunchEdit)
#include "tst_punchedit.moc"
