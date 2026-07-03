#include "todaypage.h"

#include "storage.h"
#include "widgets.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QPushButton>
#include <QTimer>
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
    connect(m_taskList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) { toggleTask(item); });
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
    today().punches.append({type, QTime::currentTime()});
    m_storage->save();
    refresh();
    tick();
}

void TodayPage::addTask() {
    const QString text = m_taskInput->text().trimmed();
    if (text.isEmpty())
        return;
    today().tasks.append({text, false});
    m_storage->save();
    m_taskInput->clear();
    m_taskInput->setFocus();
    refresh();
}

void TodayPage::toggleTask(QListWidgetItem* item) {
    if (m_updating)
        return;
    DayRecord& day = today();
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= day.tasks.size())
        return;
    day.tasks[index].done = item->checkState() == Qt::Checked;
    m_storage->save();

    m_updating = true;
    QFont font = item->font();
    font.setStrikeOut(day.tasks[index].done);
    item->setFont(font);
    m_updating = false;
}

void TodayPage::clearDoneTasks() {
    QList<Task>& tasks = today().tasks;
    tasks.removeIf([](const Task& t) { return t.done; });
    m_storage->save();
    refresh();
}

void TodayPage::refresh() {
    m_updating = true;
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

    m_punchList->clear();
    for (const Punch& p : day.punches)
        m_punchList->addItem(QStringLiteral("%1  ·  %2").arg(p.time.toString(QStringLiteral("HH:mm")),
                                                             punchLabel(p.type)));
    m_punchEmpty->setVisible(day.punches.isEmpty());
    m_punchList->setVisible(!day.punches.isEmpty());

    m_taskList->clear();
    for (int i = 0; i < day.tasks.size(); ++i) {
        const Task& task = day.tasks[i];
        auto* item = new QListWidgetItem(task.text);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(task.done ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, i);
        QFont font = item->font();
        font.setStrikeOut(task.done);
        item->setFont(font);
        m_taskList->addItem(item);
    }
    m_taskEmpty->setVisible(day.tasks.isEmpty());
    m_taskList->setVisible(!day.tasks.isEmpty());
    m_btnClearDone->setVisible(std::any_of(day.tasks.cbegin(), day.tasks.cend(),
                                           [](const Task& t) { return t.done; }));
    m_updating = false;
}

void TodayPage::tick() {
    const QDateTime now = QDateTime::currentDateTime();
    if (m_shownDate != now.date()) {  // virada de dia com o app aberto
        refresh();
        return;
    }
    m_clockLabel->setText(now.toString(QStringLiteral("HH:mm:ss")));
    m_workedLabel->setText(formatDuration(today().workedSeconds(now.time()), true));
}
