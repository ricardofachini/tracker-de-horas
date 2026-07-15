#include "taskedit.h"

#include "widgets.h"

#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace {

QPushButton* makeDialogButton(const QString& text, const char* kind) {
    auto* button = new QPushButton(text);
    button->setProperty("kind", kind);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

}  // namespace

bool editTaskTimeDialog(QWidget* parent, Task& task) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Ajustar tempo"));
    dialog.setModal(true);
    dialog.setMinimumWidth(340);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(8);
    layout->addWidget(makeLabel(QStringLiteral("Ajustar tempo dedicado"), "h2"));
    auto* taskLabel = makeLabel(task.text, "muted");
    taskLabel->setWordWrap(true);
    layout->addWidget(taskLabel);
    layout->addSpacing(10);

    // O campo mostra o total atual arredondado ao minuto; se o valor exibido
    // não mudar, nada é alterado (preserva os segundos do cronômetro).
    const int spent = task.spentSeconds(task.isRunning() ? QTime::currentTime() : QTime());
    const QTime shown = QTime(0, 0).addSecs(qMin(spent - spent % 60, 24 * 3600 - 60));

    auto* timeEdit = new QTimeEdit(shown);
    timeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    timeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    timeEdit->setToolTip(QStringLiteral("Digite o tempo ou ajuste com as setas do teclado"));

    auto* form = new QFormLayout;
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(10);
    form->addRow(makeLabel(QStringLiteral("Tempo dedicado"), "muted"), timeEdit);
    layout->addLayout(form);

    if (task.isRunning()) {
        auto* hint = makeLabel(
            QStringLiteral("A tarefa está em andamento: o tempo continua "
                           "correndo a partir do valor salvo."),
            "muted");
        hint->setWordWrap(true);
        layout->addWidget(hint);
    }
    layout->addSpacing(4);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();
    auto* cancel = makeDialogButton(QStringLiteral("Cancelar"), "neutral");
    auto* save = makeDialogButton(QStringLiteral("Salvar"), "primary");
    save->setDefault(true);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    layout->addLayout(buttons);

    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(save, &QPushButton::clicked, &dialog, &QDialog::accept);

    timeEdit->setFocus();
    if (dialog.exec() != QDialog::Accepted)
        return false;

    const QTime edited = timeEdit->time();
    if (edited == shown)
        return false;
    // O ajuste é a diferença para o total no momento do salvamento — assim
    // o cronômetro de uma tarefa em andamento retoma do valor digitado.
    const int target = QTime(0, 0).secsTo(edited);
    task.adjustSeconds +=
        target - task.spentSeconds(task.isRunning() ? QTime::currentTime() : QTime());
    return true;
}

bool askTaskOnPunchInDialog(QWidget* parent, DayRecord& day, const QTime& start) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Entrada registrada"));
    dialog.setModal(true);
    dialog.setMinimumWidth(420);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(8);
    layout->addWidget(makeLabel(QStringLiteral("Qual tarefa você vai fazer agora?"), "h2"));
    auto* detail = makeLabel(
        QStringLiteral("O cronômetro da tarefa escolhida começa a correr às %1. "
                       "Dá para trocar a qualquer momento na lista de tarefas.")
            .arg(start.toString(QStringLiteral("HH:mm"))),
        "muted");
    detail->setWordWrap(true);
    layout->addWidget(detail);
    layout->addSpacing(10);

    // Uma opção por tarefa pendente + a opção de criar uma nova.
    QList<QRadioButton*> taskOptions;
    QList<int> taskIndexes;
    for (int i = 0; i < day.tasks.size(); ++i) {
        if (day.tasks[i].done)
            continue;
        auto* option = new QRadioButton(day.tasks[i].text);
        option->setCursor(Qt::PointingHandCursor);
        layout->addWidget(option);
        taskOptions.append(option);
        taskIndexes.append(i);
    }

    auto* newOption = new QRadioButton(QStringLiteral("Criar nova tarefa:"));
    newOption->setCursor(Qt::PointingHandCursor);
    auto* newEdit = new QLineEdit;
    newEdit->setPlaceholderText(QStringLiteral("O que você vai fazer?"));
    newEdit->setEnabled(false);
    QObject::connect(newOption, &QRadioButton::toggled, newEdit, &QWidget::setEnabled);
    auto* newRow = new QHBoxLayout;
    newRow->setSpacing(8);
    newRow->addWidget(newOption);
    newRow->addWidget(newEdit, 1);
    layout->addLayout(newRow);
    layout->addSpacing(12);

    if (taskOptions.isEmpty())
        newOption->setChecked(true);
    else
        taskOptions.first()->setChecked(true);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();
    auto* skip = makeDialogButton(QStringLiteral("Agora não"), "neutral");
    auto* begin = makeDialogButton(QStringLiteral("Começar"), "primary");
    begin->setDefault(true);
    buttons->addWidget(skip);
    buttons->addWidget(begin);
    layout->addLayout(buttons);

    QObject::connect(skip, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(begin, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(newEdit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    if (newOption->isChecked())
        newEdit->setFocus();
    if (dialog.exec() != QDialog::Accepted)
        return false;

    int chosen = -1;
    if (newOption->isChecked()) {
        const QString text = newEdit->text().trimmed();
        if (text.isEmpty())  // nada digitado: o mesmo que "agora não"
            return false;
        day.tasks.append({text, false, {}});
        chosen = day.tasks.size() - 1;
    } else {
        for (int i = 0; i < taskOptions.size(); ++i) {
            if (taskOptions[i]->isChecked()) {
                chosen = taskIndexes[i];
                break;
            }
        }
    }
    if (chosen < 0 || day.tasks[chosen].isRunning())
        return false;

    day.pauseRunningTask(start);  // preempção, como no play das linhas
    day.tasks[chosen].intervals.append({start, QTime()});
    return true;
}
