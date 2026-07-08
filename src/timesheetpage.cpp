#include "timesheetpage.h"

#include "model.h"
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
    titles->addWidget(makeLabel(QStringLiteral("Entrada, saída e horas dia a dia"), "muted"));
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
    footer->addStretch();
    footer->addWidget(makeLabel(QStringLiteral("Total do mês:"), "muted"));
    m_totalLabel = makeLabel({}, "h2");
    footer->addWidget(m_totalLabel);
    cardLayout->addLayout(footer);
    root->addWidget(card, 1);

    connect(prevButton, &QPushButton::clicked, this,
            [this] { setMonth(m_model->month().addMonths(-1)); });
    connect(m_nextButton, &QPushButton::clicked, this,
            [this] { setMonth(m_model->month().addMonths(1)); });

    setMonth(QDate::currentDate());
}

void TimesheetPage::setMonth(const QDate& firstDay) {
    m_model->setMonth(firstDay);
    m_monthLabel->setText(QLocale().toString(m_model->month(), QStringLiteral("MMMM 'de' yyyy")));
    m_totalLabel->setText(formatDuration(m_model->monthTotalSeconds()));

    // Não faz sentido navegar para meses futuros.
    const QDate today = QDate::currentDate();
    m_nextButton->setEnabled(m_model->month() < QDate(today.year(), today.month(), 1));
}

void TimesheetPage::refresh() {
    setMonth(m_model->month());
}
