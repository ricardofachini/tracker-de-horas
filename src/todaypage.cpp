#include "todaypage.h"

#include "appsettings.h"
#include "punchedit.h"
#include "storage.h"
#include "theme.h"
#include "widgets.h"

#include <QCheckBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

TodayPage::TodayPage(Storage* storage, QWidget* parent)
    : QWidget(parent), m_storage(storage) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(18);

    // Cabeçalho: título + data à esquerda, relógio ao vivo à direita.
    auto* header = new QHBoxLayout;
    auto* titles = new QVBoxLayout;
    titles->setSpacing(2);
    titles->addWidget(makeLabel(QStringLiteral("Hoje"), "h1"));
    m_dateLabel = makeLabel({}, "muted");
    titles->addWidget(m_dateLabel);
    header->addLayout(titles);
    header->addStretch();
    m_clockLabel = new QLabel;
    m_clockLabel->setObjectName("clockNow");
    header->addWidget(m_clockLabel, 0, Qt::AlignTop);
    root->addLayout(header);

    // Cartão principal: status, tempo trabalhado e botões de ponto.
    auto* hero = makeCard();
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(24, 26, 24, 26);
    heroLayout->setSpacing(8);

    m_statusPill = new QLabel;
    m_statusPill->setObjectName("statusPill");
    heroLayout->addWidget(m_statusPill, 0, Qt::AlignHCenter);

    m_workedLabel = new QLabel(QStringLiteral("00:00:00"));
    m_workedLabel->setObjectName("workedLabel");
    heroLayout->addWidget(m_workedLabel, 0, Qt::AlignHCenter);
    heroLayout->addWidget(makeLabel(QStringLiteral("trabalhadas hoje"), "muted"), 0, Qt::AlignHCenter);
    heroLayout->addSpacing(10);

    auto makePunchButton = [this](const QString& text, const char* kind, PunchType type) {
        auto* button = new QPushButton(text);
        button->setProperty("kind", kind);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, [this, type] { punch(type); });
        return button;
    };
    m_btnIn = makePunchButton(QStringLiteral("Registrar entrada"), "success", PunchType::In);
    m_btnBreak = makePunchButton(QStringLiteral("Iniciar pausa"), "neutral", PunchType::BreakStart);
    m_btnResume = makePunchButton(QStringLiteral("Retomar trabalho"), "success", PunchType::BreakEnd);
    m_btnOut = makePunchButton(QStringLiteral("Encerrar expediente"), "danger", PunchType::Out);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(10);
    buttons->addStretch();
    for (QPushButton* b : {m_btnIn, m_btnBreak, m_btnResume, m_btnOut})
        buttons->addWidget(b);
    buttons->addStretch();
    heroLayout->addLayout(buttons);

    heroLayout->addSpacing(14);
    m_journeyBar = new QProgressBar;
    m_journeyBar->setObjectName("journeyBar");
    m_journeyBar->setRange(0, AppSettings::journeySeconds());
    m_journeyBar->setValue(0);
    m_journeyBar->setTextVisible(false);
    m_journeyBar->setFixedHeight(6);
    m_journeyBar->setProperty("complete", QStringLiteral("false"));
    heroLayout->addWidget(m_journeyBar);
    m_journeyCaption = makeLabel({}, "muted");
    heroLayout->addWidget(m_journeyCaption, 0, Qt::AlignHCenter);
    root->addWidget(hero);

    // Duas colunas: registros de ponto | tarefas do dia.
    auto* columns = new QHBoxLayout;
    columns->setSpacing(18);

    auto* punchCard = makeCard();
    auto* punchLayout = new QVBoxLayout(punchCard);
    punchLayout->setContentsMargins(20, 18, 20, 18);
    punchLayout->setSpacing(10);
    punchLayout->addWidget(makeLabel(QStringLiteral("Registros do dia"), "h2"));
    m_punchEmpty = makeLabel(QStringLiteral("Nenhum registro ainda.\nBata o ponto para começar o dia."), "muted");
    punchLayout->addWidget(m_punchEmpty);
    m_punchList = new QListWidget;
    m_punchList->setSelectionMode(QAbstractItemView::NoSelection);
    m_punchList->setFocusPolicy(Qt::NoFocus);
    punchLayout->addWidget(m_punchList, 1);
    columns->addWidget(punchCard, 2);

    auto* taskCard = makeCard();
    auto* taskLayout = new QVBoxLayout(taskCard);
    taskLayout->setContentsMargins(20, 18, 20, 18);
    taskLayout->setSpacing(10);
    taskLayout->addWidget(makeLabel(QStringLiteral("Tarefas do dia"), "h2"));

    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(8);
    m_taskInput = new QLineEdit;
    m_taskInput->setPlaceholderText(QStringLiteral("O que você fez ou vai fazer hoje?"));
    connect(m_taskInput, &QLineEdit::returnPressed, this, [this] { addTask(); });
    inputRow->addWidget(m_taskInput, 1);
    auto* addButton = new QPushButton(QStringLiteral("Adicionar"));
    addButton->setProperty("kind", "primary");
    addButton->setCursor(Qt::PointingHandCursor);
    connect(addButton, &QPushButton::clicked, this, [this] { addTask(); });
    inputRow->addWidget(addButton);
    taskLayout->addLayout(inputRow);

    m_taskEmpty = makeLabel(QStringLiteral("Nenhuma tarefa registrada hoje."), "muted");
    taskLayout->addWidget(m_taskEmpty);
    m_taskList = new QListWidget;
    m_taskList->setSelectionMode(QAbstractItemView::NoSelection);
    m_taskList->setFocusPolicy(Qt::NoFocus);
    taskLayout->addWidget(m_taskList, 1);

    auto* taskFooter = new QHBoxLayout;
    taskFooter->addStretch();
    m_btnClearDone = new QPushButton(QStringLiteral("Limpar concluídas"));
    m_btnClearDone->setProperty("kind", "ghost");
    m_btnClearDone->setCursor(Qt::PointingHandCursor);
    connect(m_btnClearDone, &QPushButton::clicked, this, [this] { clearDoneTasks(); });
    taskFooter->addWidget(m_btnClearDone);
    taskLayout->addLayout(taskFooter);
    columns->addWidget(taskCard, 3);

    root->addLayout(columns, 1);

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] { tick(); });
    timer->start(1000);

    refresh();
    tick();
}

DayRecord& TodayPage::today() const {
    return m_storage->day(QDate::currentDate());
}

void TodayPage::punch(PunchType type) {
    DayRecord& day = today();
    const QTime now = QTime::currentTime();
    day.punches.append({type, now});
    // Pausar ou encerrar o expediente também pausa a tarefa em andamento.
    if (type == PunchType::BreakStart || type == PunchType::Out) {
        const int running = day.runningTaskIndex();
        if (running >= 0)
            day.tasks[running].intervals.last().end = now;
    }
    m_storage->save();
    refresh();
    tick();
}

void TodayPage::addTask() {
    const QString text = m_taskInput->text().trimmed();
    if (text.isEmpty())
        return;
    today().tasks.append({text, false, {}});
    m_storage->save();
    m_taskInput->clear();
    m_taskInput->setFocus();
    refresh();
}

void TodayPage::setTaskDone(int index, bool done) {
    DayRecord& day = today();
    if (index < 0 || index >= day.tasks.size())
        return;
    Task& task = day.tasks[index];
    task.done = done;
    if (done && task.isRunning())  // concluir também para o cronômetro
        task.intervals.last().end = QTime::currentTime();
    m_storage->save();
    scheduleRefresh();
}

void TodayPage::startTask(int index) {
    DayRecord& day = today();
    if (index < 0 || index >= day.tasks.size())
        return;
    const QTime now = QTime::currentTime();
    const int running = day.runningTaskIndex();
    if (running == index)
        return;
    if (running >= 0)  // preempção: só uma tarefa corre por vez
        day.tasks[running].intervals.last().end = now;
    day.tasks[index].intervals.append({now, QTime()});
    m_storage->save();
    scheduleRefresh();
}

void TodayPage::pauseTask(int index) {
    DayRecord& day = today();
    if (index < 0 || index >= day.tasks.size())
        return;
    Task& task = day.tasks[index];
    if (!task.isRunning())
        return;
    task.intervals.last().end = QTime::currentTime();
    m_storage->save();
    scheduleRefresh();
}

void TodayPage::clearDoneTasks() {
    QList<Task>& tasks = today().tasks;
    tasks.removeIf([](const Task& t) { return t.done; });
    m_storage->save();
    refresh();
}

void TodayPage::scheduleRefresh() {
    // Adiado para o próximo ciclo do event loop: handlers de botões dentro
    // das linhas não podem destruir a própria linha enquanto executam.
    QTimer::singleShot(0, this, [this] {
        refresh();
        tick();
    });
}

void TodayPage::refresh() {
    const DayRecord& day = today();
    m_shownDate = day.date;

    m_dateLabel->setText(QLocale().toString(day.date, QStringLiteral("dddd, d 'de' MMMM 'de' yyyy")));

    // Status e botões visíveis dependem do último registro de ponto.
    const DayRecord::Status status = day.status();
    m_btnIn->setVisible(status == DayRecord::Status::Off || status == DayRecord::Status::Done);
    m_btnIn->setText(status == DayRecord::Status::Done ? QStringLiteral("Registrar nova entrada")
                                                       : QStringLiteral("Registrar entrada"));
    m_btnBreak->setVisible(status == DayRecord::Status::Working);
    m_btnResume->setVisible(status == DayRecord::Status::OnBreak);
    m_btnOut->setVisible(status == DayRecord::Status::Working || status == DayRecord::Status::OnBreak);

    QString stateName;
    switch (status) {
    case DayRecord::Status::Off:
        m_statusPill->setText(QStringLiteral("Fora do expediente"));
        stateName = QStringLiteral("off");
        break;
    case DayRecord::Status::Working:
        m_statusPill->setText(QStringLiteral("● Trabalhando"));
        stateName = QStringLiteral("working");
        break;
    case DayRecord::Status::OnBreak:
        m_statusPill->setText(QStringLiteral("● Em pausa"));
        stateName = QStringLiteral("break");
        break;
    case DayRecord::Status::Done:
        m_statusPill->setText(QStringLiteral("✓ Expediente encerrado"));
        stateName = QStringLiteral("done");
        break;
    }
    // A mesma propriedade dinâmica colore a pílula e o timer grande via QSS.
    setUiState(m_statusPill, "state", stateName);
    setUiState(m_workedLabel, "state", stateName);

    // Registros do dia: cada linha com botões de editar e remover.
    m_punchList->clear();
    for (int i = 0; i < day.punches.size(); ++i) {
        QWidget* row = makePunchRow(
            day.punches[i],
            [this, i] {
                DayRecord& record = today();
                if (i >= record.punches.size())
                    return;
                Punch punch = record.punches[i];
                if (!editPunchDialog(this, punch))
                    return;
                record.punches[i] = punch;
                record.sortPunches();
                m_storage->save();
                scheduleRefresh();
            },
            [this, i] {
                DayRecord& record = today();
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
                scheduleRefresh();
            });
        auto* item = new QListWidgetItem(m_punchList);
        item->setSizeHint(row->sizeHint());
        m_punchList->setItemWidget(item, row);
    }
    m_punchEmpty->setVisible(day.punches.isEmpty());
    m_punchList->setVisible(!day.punches.isEmpty());

    // Tarefas: checkbox + tempo dedicado + play/pause (preemptivo).
    m_taskList->clear();
    m_runningTimeLabel = nullptr;
    const QTime nowTime = QTime::currentTime();
    for (int i = 0; i < day.tasks.size(); ++i) {
        const Task& task = day.tasks[i];
        auto* row = new QWidget;
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(6, 7, 2, 7);
        rowLayout->setSpacing(8);

        auto* check = new QCheckBox(task.text);
        check->setCursor(Qt::PointingHandCursor);
        check->setChecked(task.done);
        check->setProperty("done", task.done ? "true" : "false");
        QFont font = check->font();
        font.setStrikeOut(task.done);
        check->setFont(font);
        connect(check, &QCheckBox::toggled, this,
                [this, i](bool checked) { setTaskDone(i, checked); });
        rowLayout->addWidget(check, 1);

        const bool running = task.isRunning();
        auto* timeLabel = makeLabel({}, "taskTime");
        timeLabel->setProperty("running", running ? "true" : "false");
        const int spent = task.spentSeconds(running ? nowTime : QTime());
        if (running) {
            timeLabel->setText(formatDuration(spent, true));
            m_runningTimeLabel = timeLabel;
        } else if (spent > 0) {
            timeLabel->setText(formatDuration(spent));
        }
        rowLayout->addWidget(timeLabel);

        QToolButton* action;
        if (running) {
            action = makeRowActionButton(ActionGlyph::Pause, Theme::warnText(),
                                         Theme::warnText(), QStringLiteral("Pausar tarefa"));
            connect(action, &QToolButton::clicked, this, [this, i] { pauseTask(i); });
        } else {
            action = makeRowActionButton(ActionGlyph::Play, Theme::successText(),
                                         Theme::successText(),
                                         spent > 0 ? QStringLiteral("Retomar tarefa")
                                                   : QStringLiteral("Iniciar tarefa"));
            connect(action, &QToolButton::clicked, this, [this, i] { startTask(i); });
        }
        if (task.done) {  // some, mas mantém o espaço para alinhar as linhas
            QSizePolicy policy = action->sizePolicy();
            policy.setRetainSizeWhenHidden(true);
            action->setSizePolicy(policy);
            action->hide();
        }
        rowLayout->addWidget(action);

        auto* item = new QListWidgetItem(m_taskList);
        item->setSizeHint(row->sizeHint());
        m_taskList->setItemWidget(item, row);
    }
    m_taskEmpty->setVisible(day.tasks.isEmpty());
    m_taskList->setVisible(!day.tasks.isEmpty());
    m_btnClearDone->setVisible(std::any_of(day.tasks.cbegin(), day.tasks.cend(),
                                           [](const Task& t) { return t.done; }));
}

void TodayPage::handleDayChange(const QDateTime& now) {
    const QDate previous = m_shownDate;
    const DayRecord* record = m_storage->find(previous);
    const DayRecord::Status status = record ? record->status() : DayRecord::Status::Off;
    const bool shiftOpen = status == DayRecord::Status::Working
                           || status == DayRecord::Status::OnBreak;
    // Meia-noite observada (app rodando) vs. salto de relógio (suspend/retomada):
    // no salto não dá para saber se o usuário seguiu trabalhando — perguntamos.
    const bool observedMidnight = m_lastTick.isValid() && m_lastTick.secsTo(now) < 120
                                  && previous.addDays(1) == now.date();

    refresh();  // antes de qualquer diálogo: evita reentrada pelo timer
    if (!shiftOpen)
        return;
    if (observedMidnight) {
        m_storage->bridgeMidnight(previous);
        m_storage->save();
        refresh();
    } else if (previous == now.date().addDays(-1)) {
        if (resolveOpenShiftDialog(m_storage, previous, this))
            refresh();
    }
}

void TodayPage::tick() {
    const QDateTime now = QDateTime::currentDateTime();
    if (m_shownDate != now.date()) {  // virada de dia com o app aberto
        handleDayChange(now);
        m_lastTick = now;
        return;
    }
    m_lastTick = now;
    m_clockLabel->setText(now.toString(QStringLiteral("HH:mm:ss")));

    const int worked = today().workedSeconds(now.time());
    m_workedLabel->setText(formatDuration(worked, true));

    // Cronômetro ao vivo da tarefa em andamento.
    if (m_runningTimeLabel) {
        const DayRecord& day = today();
        const int running = day.runningTaskIndex();
        if (running >= 0)
            m_runningTimeLabel->setText(
                formatDuration(day.tasks[running].spentSeconds(now.time()), true));
    }

    const int journeySeconds = AppSettings::journeySeconds();
    if (m_journeyBar->maximum() != journeySeconds)  // meta alterada em Relatórios
        m_journeyBar->setRange(0, journeySeconds);
    m_journeyBar->setValue(qMin(worked, journeySeconds));
    const bool complete = worked >= journeySeconds;
    const QString goal = formatDurationCompact(journeySeconds);
    m_journeyCaption->setText(
        complete ? QStringLiteral("meta de %1 atingida ✓").arg(goal)
                 : QStringLiteral("faltam %1 para a meta de %2")
                       .arg(formatDuration(journeySeconds - worked), goal));
    if (complete != m_journeyComplete) {  // repolir o QSS só quando muda
        m_journeyComplete = complete;
        setUiState(m_journeyBar, "complete", complete ? QStringLiteral("true")
                                                      : QStringLiteral("false"));
    }
}
