#include "timesheetpage.h"

#include "csvexport.h"
#include "model.h"
#include "punchedit.h"
#include "timesheetmodel.h"
#include "widgets.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLocale>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

TimesheetPage::TimesheetPage(Storage* storage, QWidget* parent) : QWidget(parent) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(18);

    // Cabeçalho com navegação de mês (‹ julho de 2026 ›).
    auto* header = new QHBoxLayout;
    auto* titles = new QVBoxLayout;
    titles->setSpacing(2);
    titles->addWidget(makeLabel(QStringLiteral("Folha de ponto"), "h1"));
    titles->addWidget(makeLabel(
        QStringLiteral("Entrada, saída e horas dia a dia · clique duplo em um dia para corrigir"),
        "muted"));
    header->addLayout(titles);
    header->addStretch();

    auto* prevButton = new QPushButton(QStringLiteral("‹"));
    prevButton->setObjectName("monthNav");
    prevButton->setCursor(Qt::PointingHandCursor);
    m_monthLabel = new QLabel;
    m_monthLabel->setObjectName("monthLabel");
    m_monthLabel->setAlignment(Qt::AlignCenter);
    m_monthLabel->setMinimumWidth(150);
    m_nextButton = new QPushButton(QStringLiteral("›"));
    m_nextButton->setObjectName("monthNav");
    m_nextButton->setCursor(Qt::PointingHandCursor);
    header->addWidget(prevButton);
    header->addWidget(m_monthLabel);
    header->addWidget(m_nextButton);
    root->addLayout(header);

    // A tabela vive dentro de um card, como o resto do app.
    auto* card = makeCard();
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 8, 12, 10);
    cardLayout->setSpacing(6);

    m_model = new TimesheetModel(storage, this);
    auto* table = new QTableView;
    table->setModel(m_model);
    table->setFrameShape(QFrame::NoFrame);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(36);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(TimesheetModel::Day, QHeaderView::ResizeToContents);
    cardLayout->addWidget(table, 1);

    auto* footer = new QHBoxLayout;
    footer->setContentsMargins(8, 0, 8, 2);
    auto* exportButton = new QPushButton(QStringLiteral("Exportar CSV"));
    exportButton->setProperty("kind", "compact");
    exportButton->setCursor(Qt::PointingHandCursor);
    exportButton->setToolTip(QStringLiteral("Salvar o mês exibido como planilha CSV"));
    connect(exportButton, &QPushButton::clicked, this,
            [this, storage] { exportMonthCsv(this, *storage, m_model->month()); });
    footer->addWidget(exportButton);
    footer->addStretch();
    footer->addWidget(makeLabel(QStringLiteral("Total do mês:"), "muted"));
    m_totalLabel = makeLabel({}, "h2");
    footer->addWidget(m_totalLabel);
    footer->addSpacing(18);
    footer->addWidget(makeLabel(QStringLiteral("Saldo do mês:"), "muted"));
    m_balanceLabel = makeLabel({}, "h2");
    m_balanceLabel->setToolTip(
        QStringLiteral("Banco de horas do mês: soma dos saldos diários (trabalhadas − meta)"));
    footer->addWidget(m_balanceLabel);
    cardLayout->addLayout(footer);
    root->addWidget(card, 1);

    connect(prevButton, &QPushButton::clicked, this,
            [this] { setMonth(m_model->month().addMonths(-1)); });
    connect(m_nextButton, &QPushButton::clicked, this,
            [this] { setMonth(m_model->month().addMonths(1)); });

    // Clique duplo em um dia: corrigir/remover/adicionar registros de ponto.
    connect(table, &QTableView::doubleClicked, this,
            [this, storage](const QModelIndex& index) {
                if (!index.isValid())
                    return;
                const QDate date = m_model->dateForRow(index.row());
                if (date > QDate::currentDate())  // dia futuro: nada a corrigir
                    return;
                DayPunchesDialog dialog(storage, date, this);
                dialog.exec();
                if (dialog.changed()) {
                    refresh();
                    emit dayEdited(date);
                }
            });

    setMonth(QDate::currentDate());
}

void TimesheetPage::setMonth(const QDate& firstDay) {
    m_model->setMonth(firstDay);
    m_monthLabel->setText(QLocale().toString(m_model->month(), QStringLiteral("MMMM 'de' yyyy")));
    m_totalLabel->setText(formatDuration(m_model->monthTotalSeconds()));
    const int balance = m_model->monthBalanceSeconds();
    m_balanceLabel->setText(HourBank::formatBalance(balance));
    setUiState(m_balanceLabel, "balance", QLatin1String(balanceState(balance)));

    // Não faz sentido navegar para meses futuros.
    const QDate today = QDate::currentDate();
    m_nextButton->setEnabled(m_model->month() < QDate(today.year(), today.month(), 1));
}

void TimesheetPage::refresh() {
    setMonth(m_model->month());
}
