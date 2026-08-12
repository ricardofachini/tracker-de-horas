#include "bankpage.h"

#include "appsettings.h"
#include "storage.h"
#include "theme.h"
#include "widgets.h"

#include <QHBoxLayout>
#include <QLocale>
#include <QMap>
#include <QPainter>
#include <QProgressBar>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace {

// Barra divergente do saldo: cresce do zero central para a direita (verde,
// crédito) ou para a esquerda (vermelha, débito). Todas as barras da lista
// compartilham a mesma escala, então o comprimento é comparável entre meses.
class BalanceBar : public QWidget {
public:
    BalanceBar(int balanceSeconds, int scaleSeconds, QWidget* parent = nullptr)
        : QWidget(parent), m_balance(balanceSeconds), m_scale(qMax(scaleSeconds, 1)) {
        setFixedSize(150, 14);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        const qreal mid = width() / 2.0;
        const qreal trackY = height() / 2.0 - 3;

        QColor track = Theme::faintText();
        track.setAlpha(70);
        painter.setBrush(track);
        painter.drawRoundedRect(QRectF(0, trackY, width(), 6), 3, 3);

        const qreal half = mid - 1;
        const qreal length = qMin(half, half * qAbs(m_balance) / qreal(m_scale));
        if (length >= 1.0) {
            painter.setBrush(m_balance > 0 ? Theme::successText() : Theme::dangerText());
            painter.drawRoundedRect(m_balance > 0 ? QRectF(mid, trackY, length, 6)
                                                  : QRectF(mid - length, trackY, length, 6),
                                    3, 3);
        }

        // risco central marcando o zero, por cima da barra
        painter.setBrush(Theme::weekendText());
        painter.drawRect(QRectF(mid - 0.75, height() / 2.0 - 6, 1.5, 12));
    }

private:
    int m_balance;
    int m_scale;
};

}  // namespace

BankPage::BankPage(Storage* storage, QWidget* parent)
    : QWidget(parent), m_storage(storage) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 28);
    root->setSpacing(4);
    root->addWidget(makeLabel(QStringLiteral("Banco de horas"), "h1"));
    m_subtitle = makeLabel({}, "muted");
    root->addWidget(m_subtitle);
    root->addSpacing(14);

    // Cartão principal: o saldo acumulado, grande e colorido.
    auto* heroCard = makeCard();
    auto* hero = new QVBoxLayout(heroCard);
    hero->setContentsMargins(24, 20, 24, 20);
    hero->setSpacing(2);
    hero->addWidget(makeLabel(QStringLiteral("Saldo acumulado"), "muted"));
    auto* heroRow = new QHBoxLayout;
    heroRow->setSpacing(14);
    m_totalValue = new QLabel(QStringLiteral("—"));
    m_totalValue->setObjectName("bankTotal");
    heroRow->addWidget(m_totalValue);
    m_totalPill = new QLabel;
    m_totalPill->setObjectName("balancePill");
    heroRow->addWidget(m_totalPill, 0, Qt::AlignVCenter);
    heroRow->addStretch();
    hero->addLayout(heroRow);
    m_totalHint = makeLabel({}, "muted");
    m_totalHint->setWordWrap(true);
    hero->addWidget(m_totalHint);
    root->addWidget(heroCard);
    root->addSpacing(14);

    // Cartão do mês em andamento: progresso rumo à meta mensal.
    auto* currentCard = makeCard();
    auto* current = new QVBoxLayout(currentCard);
    current->setContentsMargins(24, 20, 24, 20);
    current->setSpacing(6);
    current->addWidget(makeLabel(QStringLiteral("Mês atual"), "muted"));
    m_currentMonthLabel = makeLabel({}, "h2");
    current->addWidget(m_currentMonthLabel);
    m_monthBar = new QProgressBar;
    m_monthBar->setObjectName("journeyBar");  // reaproveita o estilo da barra da página Hoje
    m_monthBar->setRange(0, AppSettings::monthlyGoalSeconds());
    m_monthBar->setValue(0);
    m_monthBar->setTextVisible(false);
    m_monthBar->setFixedHeight(6);
    m_monthBar->setProperty("complete", QStringLiteral("false"));
    current->addWidget(m_monthBar);
    m_monthCaption = makeLabel({}, "muted");
    current->addWidget(m_monthCaption);
    root->addWidget(currentCard);
    root->addSpacing(14);

    // Lista dos meses já fechados (mais recente primeiro).
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

    root->addSpacing(8);
    auto* note = makeLabel(
        QStringLiteral("Meses sem registro não geram débito · o mês atual entra no saldo quando "
                       "termina · a meta mensal é configurável em Relatórios."),
        "muted");
    note->setWordWrap(true);
    root->addWidget(note);

    refresh();
}

void BankPage::refresh() {
    const int goal = AppSettings::monthlyGoalSeconds();
    const QDate today = QDate::currentDate();
    const QTime now = QTime::currentTime();
    const QDate currentMonth(today.year(), today.month(), 1);

    m_subtitle->setText(
        QStringLiteral("Horas a mais (crédito) ou a menos (débito) que a meta mensal de %1")
            .arg(formatDurationCompact(goal)));

    // Uma única varredura agrega as horas trabalhadas por mês (chave: dia 1).
    QMap<QDate, int> monthWorked;
    QMap<QDate, bool> monthHasPunch;
    for (const DayRecord& day : m_storage->allDays()) {
        const QTime closeAt = day.date == today ? now : QTime();
        const QDate key(day.date.year(), day.date.month(), 1);
        monthWorked[key] += day.workedSeconds(closeAt);
        if (HourBank::dayCounts(day))
            monthHasPunch[key] = true;
    }

    // Meses fechados (com atividade e diferentes do mês atual), do mais recente
    // ao mais antigo, alimentam o saldo acumulado e a lista.
    QList<QDate> closedMonths;
    int total = 0, maxAbs = 3600;  // escala mínima de 1h para as barrinhas
    for (auto it = monthHasPunch.constBegin(); it != monthHasPunch.constEnd(); ++it) {
        if (it.key() == currentMonth)
            continue;
        closedMonths.append(it.key());
        const int balance = HourBank::monthBalance(monthWorked.value(it.key()), goal);
        total += balance;
        maxAbs = qMax(maxAbs, qAbs(balance));
    }
    std::sort(closedMonths.begin(), closedMonths.end(), std::greater<QDate>());
    const int closedCount = closedMonths.size();

    // Cartão principal.
    m_totalValue->setText(HourBank::formatBalance(total));
    setUiState(m_totalValue, "balance", balanceState(total));
    m_totalPill->setText(total > 0 ? QStringLiteral("crédito")
                         : total < 0 ? QStringLiteral("débito")
                                     : QStringLiteral("zerado"));
    setUiState(m_totalPill, "balance", balanceState(total));
    m_totalPill->setVisible(closedCount > 0);
    if (closedCount > 0)
        m_totalHint->setText(closedCount == 1
                                 ? QStringLiteral("Somando o único mês já fechado.")
                                 : QStringLiteral("Somando os %1 meses já fechados.")
                                       .arg(closedCount));
    else
        m_totalHint->setText(QStringLiteral(
            "Nenhum mês fechado ainda — o saldo começa a contar quando o mês atual terminar."));

    // Cartão do mês em andamento.
    const QLocale locale;
    const int currentWorked = monthWorked.value(currentMonth);
    m_currentMonthLabel->setText(locale.toString(currentMonth, QStringLiteral("MMMM 'de' yyyy")));
    if (m_monthBar->maximum() != goal)
        m_monthBar->setRange(0, goal);
    m_monthBar->setValue(qMin(currentWorked, goal));
    const bool complete = currentWorked >= goal;
    setUiState(m_monthBar, "complete", complete ? QStringLiteral("true") : QStringLiteral("false"));
    if (complete)
        m_monthCaption->setText(QStringLiteral("meta de %1 atingida ✓ · +%2 de crédito")
                                    .arg(formatDurationCompact(goal),
                                         formatDurationCompact(currentWorked - goal)));
    else
        m_monthCaption->setText(QStringLiteral("%1 de %2 · faltam %3 para a meta do mês")
                                    .arg(formatDurationCompact(currentWorked),
                                         formatDurationCompact(goal),
                                         formatDurationCompact(goal - currentWorked)));

    // Lista dos meses fechados.
    while (m_list->count() > 1) {  // preserva o stretch final
        QLayoutItem* item = m_list->takeAt(0);
        delete item->widget();
        delete item;
    }

    if (closedMonths.isEmpty()) {
        m_list->insertWidget(0, makeLabel(QStringLiteral("Nenhum mês fechado ainda."), "muted"));
        return;
    }

    for (const QDate& month : closedMonths) {
        const int worked = monthWorked.value(month);
        const int balance = HourBank::monthBalance(worked, goal);

        auto* card = makeCard();
        auto* rowLayout = new QHBoxLayout(card);
        rowLayout->setContentsMargins(18, 14, 18, 14);
        rowLayout->setSpacing(14);

        auto* texts = new QVBoxLayout;
        texts->setSpacing(2);
        texts->addWidget(
            makeLabel(locale.toString(month, QStringLiteral("MMMM 'de' yyyy")), "h2"));
        texts->addWidget(makeLabel(QStringLiteral("%1 de %2")
                                       .arg(formatDurationCompact(worked),
                                            formatDurationCompact(goal)),
                                   "muted"));
        rowLayout->addLayout(texts);
        rowLayout->addStretch();

        rowLayout->addWidget(new BalanceBar(balance, maxAbs), 0, Qt::AlignVCenter);
        auto* pill = makeBalancePill(balance);
        pill->setToolTip(QStringLiteral("Saldo do mês: trabalhadas − meta mensal."));
        rowLayout->addWidget(pill, 0, Qt::AlignVCenter);

        m_list->insertWidget(m_list->count() - 1, card);
    }
}
