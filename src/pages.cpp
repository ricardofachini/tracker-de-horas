#include "pages.h"

#include "appsettings.h"
#include "storage.h"
#include "weekchart.h"
#include "widgets.h"

#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTimeEdit>
#include <QVBoxLayout>

// ---------------------------------------------------------------- Histórico

HistoryPage::HistoryPage(Storage* storage, QWidget* parent)
    : QWidget(parent), m_storage(storage) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(4);
    root->addWidget(makeLabel(QStringLiteral("Histórico"), "h1"));
    root->addWidget(makeLabel(QStringLiteral("Seus dias de trabalho registrados"), "muted"));
    root->addSpacing(14);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* holder = new QWidget;
    m_list = new QVBoxLayout(holder);
    m_list->setContentsMargins(0, 0, 0, 0);
    m_list->setSpacing(10);
    m_list->addStretch();
    scroll->setWidget(holder);
    root->addWidget(scroll, 1);

    refresh();
}

void HistoryPage::refresh() {
    while (m_list->count() > 1) {  // preserva o stretch final
        QLayoutItem* item = m_list->takeAt(0);
        delete item->widget();
        delete item;
    }

    const QDate today = QDate::currentDate();
    bool empty = true;
    for (const DayRecord& day : m_storage->allDays()) {
        if (day.punches.isEmpty() && day.tasks.isEmpty())
            continue;
        empty = false;

        auto* row = makeCard();
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(18, 14, 18, 14);

        auto* left = new QVBoxLayout;
        left->setSpacing(2);
        QString dateText = QLocale().toString(day.date, QStringLiteral("dddd, d 'de' MMMM 'de' yyyy"));
        if (day.date == today)
            dateText = QStringLiteral("Hoje · ") + dateText;
        left->addWidget(makeLabel(dateText, "h2"));
        const auto doneCount = std::count_if(day.tasks.cbegin(), day.tasks.cend(),
                                             [](const Task& t) { return t.done; });
        left->addWidget(makeLabel(QStringLiteral("%1 registros de ponto · %2 tarefas (%3 concluídas)")
                                      .arg(day.punches.size())
                                      .arg(day.tasks.size())
                                      .arg(doneCount),
                                  "muted"));
        rowLayout->addLayout(left);
        rowLayout->addStretch();

        auto* right = new QVBoxLayout;
        right->setSpacing(2);
        const int worked = day.workedSeconds(day.date == today ? QTime::currentTime() : QTime());
        right->addWidget(makeLabel(formatDuration(worked), "h2"), 0, Qt::AlignRight);
        right->addWidget(makeLabel(QStringLiteral("trabalhadas"), "muted"), 0, Qt::AlignRight);
        rowLayout->addLayout(right);

        m_list->insertWidget(m_list->count() - 1, row);
    }

    if (empty)
        m_list->insertWidget(0, makeLabel(QStringLiteral("Nenhum dia registrado ainda."), "muted"));
}

// ---------------------------------------------------------------- Relatórios

ReportsPage::ReportsPage(Storage* storage, QWidget* parent)
    : QWidget(parent), m_storage(storage) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(4);
    root->addWidget(makeLabel(QStringLiteral("Relatórios"), "h1"));
    root->addWidget(makeLabel(QStringLiteral("Resumo do seu tempo de trabalho"), "muted"));
    root->addSpacing(14);

    auto makeTile = [](const QString& caption, QLabel*& valueOut) {
        auto* tile = makeCard();
        auto* layout = new QVBoxLayout(tile);
        layout->setContentsMargins(20, 18, 20, 18);
        layout->setSpacing(4);
        valueOut = new QLabel(QStringLiteral("—"));
        valueOut->setObjectName("statValue");
        layout->addWidget(valueOut);
        layout->addWidget(makeLabel(caption, "muted"));
        return tile;
    };

    auto* tiles = new QHBoxLayout;
    tiles->setSpacing(14);
    tiles->addWidget(makeTile(QStringLiteral("hoje"), m_todayValue));
    tiles->addWidget(makeTile(QStringLiteral("esta semana"), m_weekValue));
    tiles->addWidget(makeTile(QStringLiteral("este mês"), m_monthValue));
    tiles->addWidget(makeTile(QStringLiteral("média diária na semana"), m_avgValue));
    root->addLayout(tiles);
    root->addSpacing(14);

    // Gráfico de barras da semana, com navegação ‹ › como na Folha.
    auto* chartCard = makeCard();
    auto* chartLayout = new QVBoxLayout(chartCard);
    chartLayout->setContentsMargins(20, 18, 20, 16);
    chartLayout->setSpacing(10);

    auto* chartHeader = new QHBoxLayout;
    chartHeader->setSpacing(8);
    chartHeader->addWidget(makeLabel(QStringLiteral("Horas por semana"), "h2"));
    chartHeader->addStretch();
    auto* prevWeek = new QPushButton(QStringLiteral("‹"));
    prevWeek->setObjectName("monthNav");
    prevWeek->setCursor(Qt::PointingHandCursor);
    m_weekLabel = new QLabel;
    m_weekLabel->setObjectName("monthLabel");
    m_weekLabel->setAlignment(Qt::AlignCenter);
    m_weekLabel->setMinimumWidth(190);
    m_nextWeekButton = new QPushButton(QStringLiteral("›"));
    m_nextWeekButton->setObjectName("monthNav");
    m_nextWeekButton->setCursor(Qt::PointingHandCursor);
    chartHeader->addWidget(prevWeek);
    chartHeader->addWidget(m_weekLabel);
    chartHeader->addWidget(m_nextWeekButton);
    chartLayout->addLayout(chartHeader);

    m_chart = new WeekChart;
    chartLayout->addWidget(m_chart, 1);
    root->addWidget(chartCard, 1);
    root->addSpacing(14);

    connect(prevWeek, &QPushButton::clicked, this,
            [this] { setWeek(m_weekStart.addDays(-7)); });
    connect(m_nextWeekButton, &QPushButton::clicked, this,
            [this] { setWeek(m_weekStart.addDays(7)); });

    // Meta de jornada configurável: alimenta a barra da página Hoje
    // (relida a cada segundo) e a linha de meta do gráfico acima.
    auto* goalCard = makeCard();
    auto* goalLayout = new QHBoxLayout(goalCard);
    goalLayout->setContentsMargins(20, 18, 20, 18);
    goalLayout->setSpacing(14);
    auto* goalTexts = new QVBoxLayout;
    goalTexts->setSpacing(2);
    goalTexts->addWidget(makeLabel(QStringLiteral("Meta de jornada diária"), "h2"));
    goalTexts->addWidget(makeLabel(
        QStringLiteral("Usada na barra da página Hoje e na linha de meta do gráfico "
                       "de horas por semana."),
        "muted"));
    goalLayout->addLayout(goalTexts);
    goalLayout->addStretch();
    auto* goalEdit = new QTimeEdit(QTime(0, 0).addSecs(AppSettings::journeySeconds()));
    goalEdit->setDisplayFormat(QStringLiteral("HH:mm"));
    goalEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    goalEdit->setMinimumTime(QTime(0, 0).addSecs(AppSettings::kMinJourneySeconds));
    goalEdit->setToolTip(QStringLiteral("Digite a meta ou ajuste com as setas do teclado"));
    connect(goalEdit, &QTimeEdit::timeChanged, this, [this](const QTime& time) {
        AppSettings::setJourneySeconds(QTime(0, 0).secsTo(time));
        m_chart->update();  // reposiciona a linha de meta na hora
    });
    goalLayout->addWidget(goalEdit);
    root->addWidget(goalCard);
    root->addSpacing(14);

    // Meta de horas mensais: base do banco de horas e do "Saldo do mês" da
    // Folha (o contrato é por horas no mês).
    auto* monthlyCard = makeCard();
    auto* monthlyLayout = new QHBoxLayout(monthlyCard);
    monthlyLayout->setContentsMargins(20, 18, 20, 18);
    monthlyLayout->setSpacing(14);
    auto* monthlyTexts = new QVBoxLayout;
    monthlyTexts->setSpacing(2);
    monthlyTexts->addWidget(makeLabel(QStringLiteral("Meta de horas mensais"), "h2"));
    monthlyTexts->addWidget(makeLabel(
        QStringLiteral("Usada no banco de horas e no \"Saldo do mês\" da Folha — "
                       "seu contrato por horas no mês."),
        "muted"));
    monthlyLayout->addLayout(monthlyTexts);
    monthlyLayout->addStretch();
    auto* monthlyEdit = new QSpinBox;
    monthlyEdit->setRange(AppSettings::kMinMonthlyGoalSeconds / 3600,
                          AppSettings::kMaxMonthlyGoalSeconds / 3600);
    monthlyEdit->setSuffix(QStringLiteral(" h"));
    monthlyEdit->setValue(AppSettings::monthlyGoalSeconds() / 3600);
    monthlyEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    monthlyEdit->setToolTip(
        QStringLiteral("Digite a meta mensal ou ajuste com as setas do teclado"));
    connect(monthlyEdit, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [](int hours) { AppSettings::setMonthlyGoalSeconds(hours * 3600); });
    monthlyLayout->addWidget(monthlyEdit);
    root->addWidget(monthlyCard);

    const QDate today = QDate::currentDate();
    m_weekStart = today.addDays(-(today.dayOfWeek() - 1));  // segunda-feira
    refresh();
}

void ReportsPage::setWeek(const QDate& monday) {
    m_weekStart = monday;
    const QDate today = QDate::currentDate();
    const QDate sunday = monday.addDays(6);

    QList<int> worked;
    for (int i = 0; i < 7; ++i) {
        const QDate date = monday.addDays(i);
        const DayRecord* day = m_storage->find(date);
        worked.append(day ? day->workedSeconds(date == today ? QTime::currentTime()
                                                             : QTime())
                          : 0);
    }
    m_chart->setWeek(monday, worked);

    const QLocale locale;
    m_weekLabel->setText(
        monday.month() == sunday.month()
            ? QStringLiteral("%1 – %2")
                  .arg(monday.day())
                  .arg(locale.toString(sunday, QStringLiteral("d 'de' MMMM 'de' yyyy")))
            : QStringLiteral("%1 – %2")
                  .arg(locale.toString(monday, QStringLiteral("d 'de' MMM")),
                       locale.toString(sunday, QStringLiteral("d 'de' MMM 'de' yyyy"))));

    // Não faz sentido navegar para semanas futuras.
    m_nextWeekButton->setEnabled(sunday < today);
}

void ReportsPage::refresh() {
    const QDate today = QDate::currentDate();
    const QDate weekStart = today.addDays(-(today.dayOfWeek() - 1));  // segunda-feira

    int todaySecs = 0, weekSecs = 0, monthSecs = 0, weekDays = 0;
    for (const DayRecord& day : m_storage->allDays()) {
        const int secs = day.workedSeconds(day.date == today ? QTime::currentTime() : QTime());
        if (secs <= 0)
            continue;
        if (day.date == today)
            todaySecs = secs;
        if (day.date >= weekStart && day.date <= today) {
            weekSecs += secs;
            ++weekDays;
        }
        if (day.date.year() == today.year() && day.date.month() == today.month())
            monthSecs += secs;
    }

    m_todayValue->setText(formatDuration(todaySecs));
    m_weekValue->setText(formatDuration(weekSecs));
    m_monthValue->setText(formatDuration(monthSecs));
    m_avgValue->setText(weekDays > 0 ? formatDuration(weekSecs / weekDays) : QStringLiteral("—"));

    setWeek(m_weekStart);  // re-lê os dados da semana exibida no gráfico
}
