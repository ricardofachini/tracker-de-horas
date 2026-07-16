#include "punchedit.h"

#include "storage.h"
#include "theme.h"
#include "widgets.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLocale>
#include <QPushButton>
#include <QRadioButton>
#include <QTimeEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QPushButton* makeDialogButton(const QString& text, const char* kind) {
    auto* button = new QPushButton(text);
    button->setProperty("kind", kind);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

}  // namespace

bool editPunchDialog(QWidget* parent, Punch& punch, const QString& title) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(320);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(16);
    layout->addWidget(makeLabel(title, "h2"));

    auto* form = new QFormLayout;
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(10);

    auto* typeBox = new QComboBox;
    typeBox->setCursor(Qt::PointingHandCursor);
    for (PunchType type : {PunchType::In, PunchType::BreakStart,
                           PunchType::BreakEnd, PunchType::Out})
        typeBox->addItem(punchLabel(type), int(type));
    typeBox->setCurrentIndex(int(punch.type));

    auto* timeEdit = new QTimeEdit(punch.time);
    timeEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    timeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    timeEdit->setToolTip(QStringLiteral("Digite o horário ou ajuste com as setas do teclado"));

    form->addRow(makeLabel(QStringLiteral("Tipo"), "muted"), typeBox);
    form->addRow(makeLabel(QStringLiteral("Horário"), "muted"), timeEdit);
    layout->addLayout(form);
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

    punch.type = PunchType(typeBox->currentData().toInt());
    const QTime edited = timeEdit->time();
    // Preserva os segundos originais quando o horário exibido não mudou.
    if (edited.hour() != punch.time.hour() || edited.minute() != punch.time.minute())
        punch.time = QTime(edited.hour(), edited.minute());
    return true;
}

bool confirmRemoveDialog(QWidget* parent, const QString& question, const QString& detail) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Remover"));
    dialog.setModal(true);
    dialog.setMinimumWidth(340);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(8);
    layout->addWidget(makeLabel(question, "h2"));
    auto* detailLabel = makeLabel(detail, "muted");
    detailLabel->setWordWrap(true);
    layout->addWidget(detailLabel);
    layout->addSpacing(12);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();
    auto* cancel = makeDialogButton(QStringLiteral("Cancelar"), "neutral");
    auto* remove = makeDialogButton(QStringLiteral("Remover"), "danger");
    cancel->setDefault(true);  // remover exige um clique consciente
    buttons->addWidget(cancel);
    buttons->addWidget(remove);
    layout->addLayout(buttons);

    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(remove, &QPushButton::clicked, &dialog, &QDialog::accept);
    return dialog.exec() == QDialog::Accepted;
}

void infoDialog(QWidget* parent, const QString& title, const QString& detail) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setModal(true);
    dialog.setMinimumWidth(360);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(8);
    layout->addWidget(makeLabel(title, "h2"));
    auto* detailLabel = makeLabel(detail, "muted");
    detailLabel->setWordWrap(true);
    layout->addWidget(detailLabel);
    layout->addSpacing(12);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* ok = makeDialogButton(QStringLiteral("Entendi"), "primary");
    ok->setDefault(true);
    buttons->addWidget(ok);
    layout->addLayout(buttons);

    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

bool resolveOpenShiftDialog(Storage* storage, const QDate& openDay, QWidget* parent) {
    const DayRecord* record = storage->find(openDay);
    if (!record || !record->hasOpenShift())
        return false;

    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Expediente em aberto"));
    dialog.setModal(true);
    dialog.setMinimumWidth(420);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(8);
    layout->addWidget(makeLabel(QStringLiteral("O expediente de ontem ficou aberto"), "h2"));
    auto* detail = makeLabel(
        QStringLiteral("Em %1 a última marcação foi \"%2\" às %3, sem encerrar o expediente.")
            .arg(QLocale().toString(openDay, QStringLiteral("dddd, d 'de' MMMM")),
                 punchLabel(record->punches.last().type),
                 record->punches.last().time.toString(QStringLiteral("HH:mm"))),
        "muted");
    detail->setWordWrap(true);
    layout->addWidget(detail);
    layout->addSpacing(10);

    auto* stillWorking = new QRadioButton(
        QStringLiteral("Ainda estou trabalhando (o turno atravessou a meia-noite)"));
    auto* workedUntil = new QRadioButton(QStringLiteral("Trabalhei até as"));
    auto* fixManually = new QRadioButton(
        QStringLiteral("Encerrei ontem e esqueci de registrar — corrigir os registros"));
    stillWorking->setChecked(true);
    for (QRadioButton* option : {stillWorking, workedUntil, fixManually})
        option->setCursor(Qt::PointingHandCursor);

    auto* untilEdit = new QTimeEdit(QTime::currentTime());
    untilEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    untilEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    untilEdit->setEnabled(false);
    QObject::connect(workedUntil, &QRadioButton::toggled, untilEdit,
                     &QWidget::setEnabled);

    layout->addWidget(stillWorking);
    auto* untilRow = new QHBoxLayout;
    untilRow->setSpacing(8);
    untilRow->addWidget(workedUntil);
    untilRow->addWidget(untilEdit);
    untilRow->addWidget(new QLabel(QStringLiteral("de hoje")));
    untilRow->addStretch();
    layout->addLayout(untilRow);
    layout->addWidget(fixManually);
    layout->addSpacing(12);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();
    auto* later = makeDialogButton(QStringLiteral("Deixar para depois"), "neutral");
    auto* apply = makeDialogButton(QStringLiteral("Aplicar"), "primary");
    apply->setDefault(true);
    buttons->addWidget(later);
    buttons->addWidget(apply);
    layout->addLayout(buttons);

    QObject::connect(later, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(apply, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted)
        return false;

    if (fixManually->isChecked()) {
        DayPunchesDialog punches(storage, openDay, parent);
        punches.exec();
        return punches.changed();
    }

    storage->bridgeMidnight(openDay);
    if (workedUntil->isChecked()) {
        DayRecord& today = storage->day(openDay.addDays(1));
        today.punches.append({PunchType::Out, untilEdit->time()});
        today.sortPunches();
    }
    storage->save();
    return true;
}

QToolButton* makeRowActionButton(ActionGlyph glyph, const QColor& normal,
                                 const QColor& active, const QString& tooltip) {
    auto* button = new QToolButton;
    button->setProperty("rowAction", true);
    button->setAutoRaise(true);  // habilita o modo Active do ícone no hover
    button->setCursor(Qt::PointingHandCursor);
    button->setIconSize(QSize(16, 16));
    button->setFixedSize(26, 26);
    button->setIcon(actionIcon(glyph, normal, active));
    button->setToolTip(tooltip);
    return button;
}

QWidget* makePunchRow(const Punch& punch,
                      const std::function<void()>& onEdit,
                      const std::function<void()>& onRemove) {
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(6, 7, 2, 7);
    layout->setSpacing(10);

    auto* time = new QLabel(punch.time.toString(QStringLiteral("HH:mm")));
    time->setProperty("role", "punchTime");
    layout->addWidget(time);
    layout->addWidget(new QLabel(punchLabel(punch.type)), 1);

    auto* edit = makeRowActionButton(ActionGlyph::Edit, Theme::iconMuted(),
                                     Theme::accentStrong(), QStringLiteral("Editar registro"));
    QObject::connect(edit, &QToolButton::clicked, row, [onEdit] { onEdit(); });
    layout->addWidget(edit);

    auto* remove = makeRowActionButton(ActionGlyph::Remove, Theme::iconMuted(),
                                       Theme::dangerText(), QStringLiteral("Remover registro"));
    QObject::connect(remove, &QToolButton::clicked, row, [onRemove] { onRemove(); });
    layout->addWidget(remove);
    return row;
}

// ---------------------------------------------------------- DayPunchesDialog

DayPunchesDialog::DayPunchesDialog(Storage* storage, const QDate& date, QWidget* parent)
    : QDialog(parent), m_storage(storage), m_date(date) {
    setWindowTitle(QStringLiteral("Editar registros do dia"));
    setModal(true);
    resize(440, 460);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 18);
    layout->setSpacing(6);
    layout->addWidget(makeLabel(
        QLocale().toString(date, QStringLiteral("dddd, d 'de' MMMM 'de' yyyy")), "h2"));
    layout->addWidget(makeLabel(
        QStringLiteral("Corrija o horário e o tipo, remova ou adicione registros de ponto."),
        "muted"));
    layout->addSpacing(8);

    auto* card = makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 12, 16, 12);
    m_empty = makeLabel(QStringLiteral("Nenhum registro de ponto neste dia."), "muted");
    cardLayout->addWidget(m_empty);
    m_list = new QListWidget;
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setFocusPolicy(Qt::NoFocus);
    cardLayout->addWidget(m_list, 1);
    layout->addWidget(card, 1);
    layout->addSpacing(8);

    auto* footer = new QHBoxLayout;
    footer->setSpacing(10);
    m_totalLabel = makeLabel({}, "muted");
    footer->addWidget(m_totalLabel);
    footer->addStretch();
    auto* add = makeDialogButton(QStringLiteral("Adicionar registro"), "neutral");
    connect(add, &QPushButton::clicked, this, [this] { addPunch(); });
    footer->addWidget(add);
    auto* close = makeDialogButton(QStringLiteral("Fechar"), "primary");
    close->setDefault(true);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(close);
    layout->addLayout(footer);

    repopulate();
}

void DayPunchesDialog::repopulate() {
    m_list->clear();
    const DayRecord* day = m_storage->find(m_date);
    const QList<Punch> punches = day ? day->punches : QList<Punch>();

    for (int i = 0; i < punches.size(); ++i) {
        QWidget* row = makePunchRow(
            punches[i],
            [this, i] {
                DayRecord& record = m_storage->day(m_date);
                if (i >= record.punches.size())
                    return;
                Punch punch = record.punches[i];
                if (!editPunchDialog(this, punch))
                    return;
                record.punches[i] = punch;
                record.sortPunches();
                m_storage->save();
                m_changed = true;
                // Adiado: não destruir a linha enquanto o clique dela executa.
                QTimer::singleShot(0, this, [this] { repopulate(); });
            },
            [this, i] {
                DayRecord& record = m_storage->day(m_date);
                if (i >= record.punches.size())
                    return;
                const Punch& punch = record.punches[i];
                if (!confirmRemoveDialog(
                        this, QStringLiteral("Remover registro?"),
                        QStringLiteral("O registro \"%1 · %2\" será removido definitivamente.")
                            .arg(punch.time.toString(QStringLiteral("HH:mm")),
                                 punchLabel(punch.type))))
                    return;
                record.punches.removeAt(i);
                m_storage->save();
                m_changed = true;
                QTimer::singleShot(0, this, [this] { repopulate(); });
            });
        auto* item = new QListWidgetItem(m_list);
        item->setSizeHint(row->sizeHint());
        m_list->setItemWidget(item, row);
    }

    m_empty->setVisible(punches.isEmpty());
    m_list->setVisible(!punches.isEmpty());

    const bool isToday = m_date == QDate::currentDate();
    const int worked = day ? day->workedSeconds(isToday ? QTime::currentTime() : QTime()) : 0;
    m_totalLabel->setText(QStringLiteral("Trabalhadas: %1").arg(formatDuration(worked)));
}

void DayPunchesDialog::addPunch() {
    const bool isToday = m_date == QDate::currentDate();
    const DayRecord* existing = m_storage->find(m_date);

    // Palpite inicial: o tipo que normalmente viria depois do último registro.
    Punch punch{PunchType::In, isToday ? QTime::currentTime() : QTime(8, 0)};
    if (existing && !existing->punches.isEmpty()) {
        const Punch& last = existing->punches.last();
        switch (last.type) {
        case PunchType::In:
        case PunchType::BreakEnd: punch.type = PunchType::Out; break;
        case PunchType::BreakStart: punch.type = PunchType::BreakEnd; break;
        case PunchType::Out: punch.type = PunchType::In; break;
        }
        if (!isToday)
            punch.time = last.time;
    }

    if (!editPunchDialog(this, punch, QStringLiteral("Novo registro")))
        return;
    DayRecord& record = m_storage->day(m_date);
    record.punches.append(punch);
    record.sortPunches();
    m_storage->save();
    m_changed = true;
    repopulate();
}
