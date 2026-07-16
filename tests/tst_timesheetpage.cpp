#include "timesheetpage.h"

#include "storage.h"
#include "testutils.h"

#include <QStandardPaths>
#include <QTableView>
#include <QTemporaryDir>
#include <QtTest>

// Testes da página Folha: o clique duplo abre o editor de registros apenas
// para dias já encerrados — o dia atual com expediente aberto é protegido
// (os ajustes dele são feitos pela página Hoje).
class TestTimesheetPage : public QObject {
    Q_OBJECT

private:
    // Dispara o doubleClicked da tabela na linha do dia exibido.
    static void doubleClickDay(TimesheetPage& page, const QDate& date) {
        auto* table = page.findChild<QTableView*>();
        QVERIFY(table);
        const QModelIndex index = table->model()->index(date.day() - 1, 0);
        QMetaObject::invokeMethod(table, "doubleClicked", Qt::DirectConnection,
                                  Q_ARG(QModelIndex, index));
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

    void openTodayIsBlocked() {
        Storage storage;
        const QDate today = QDate::currentDate();
        storage.day(today).punches = {{PunchType::In, QTime(8, 0)}};

        TimesheetPage page(&storage);
        QSignalSpy edited(&page, &TimesheetPage::dayEdited);

        bool sawWarning = false;
        onNextModal(this, [&sawWarning](QDialog* dialog) {
            sawWarning =
                dialog->windowTitle() == QStringLiteral("Expediente em andamento");
            clickButton(dialog, QStringLiteral("Entendi"));
        });
        doubleClickDay(page, today);

        QVERIFY(sawWarning);
        QCOMPARE(edited.count(), 0);
        QCOMPARE(storage.find(today)->punches.size(), 1);  // nada mudou
    }

    void closedTodayOpensEditor() {
        Storage storage;
        const QDate today = QDate::currentDate();
        storage.day(today).punches = {{PunchType::In, QTime(8, 0)},
                                      {PunchType::Out, QTime(17, 0)}};

        TimesheetPage page(&storage);

        bool sawEditor = false;
        onNextModal(this, [&sawEditor](QDialog* dialog) {
            sawEditor =
                dialog->windowTitle() == QStringLiteral("Editar registros do dia");
            clickButton(dialog, QStringLiteral("Fechar"));
        });
        doubleClickDay(page, today);

        QVERIFY(sawEditor);
    }

    void pastDayEditEmitsDayEdited() {
        Storage storage;
        const QDate today = QDate::currentDate();
        const QDate past = today.addDays(-1);
        // Dia antigo que ficou com o expediente aberto: continua editável —
        // corrigir esse esquecimento é justamente o papel do diálogo.
        storage.day(past).punches = {{PunchType::In, QTime(8, 0)}};

        TimesheetPage page(&storage);
        if (past.month() != today.month())  // ontem caiu no mês anterior
            QVERIFY(clickButton(&page, QStringLiteral("‹")));
        QSignalSpy edited(&page, &TimesheetPage::dayEdited);

        onNextModal(this, [this](QDialog* dayDialog) {
            // Adiciona a saída que faltou; o editor aceita o palpite padrão.
            onNextModal(this, [dayDialog](QDialog* editor) {
                clickButton(editor, QStringLiteral("Salvar"));
                QTimer::singleShot(0, dayDialog, [dayDialog] {
                    clickButton(dayDialog, QStringLiteral("Fechar"));
                });
            });
            clickButton(dayDialog, QStringLiteral("Adicionar registro"));
        });
        doubleClickDay(page, past);

        QCOMPARE(edited.count(), 1);
        QCOMPARE(edited.first().first().toDate(), past);
        QCOMPARE(storage.find(past)->status(), DayRecord::Status::Done);
    }

private:
    QTemporaryDir m_settingsDir;
};

QTEST_MAIN(TestTimesheetPage)
#include "tst_timesheetpage.moc"
