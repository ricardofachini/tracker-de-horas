#include "bankpage.h"

#include "appsettings.h"
#include "storage.h"
#include "theme.h"
#include "widgets.h"

#include <QHBoxLayout>
#include <QLocale>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

// Barra divergente do saldo: cresce do zero central para a direita (verde,
// crédito) ou para a esquerda (vermelha, débito). Todas as barras da lista
// compartilham a mesma escala, então o comprimento é comparável entre dias.
class BalanceBar : public QWidget {
public:
    BalanceBar(int balanceSeconds, int scaleSeconds, bool partial, QWidget* parent = nullptr)
        : QWidget(parent), m_balance(balanceSeconds),
          m_scale(qMax(scaleSeconds, 1)), m_partial(partial) {
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
            painter.setBrush(m_partial ? Theme::warnText()
                             : m_balance > 0 ? Theme::successText()
                                             : Theme::dangerText());
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
    bool m_partial;
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

    // Resumo em três cartões, como na página Relatórios.
    auto makeTile = [](const QString& caption, QLabel*& valueOut, QLabel*& captionOut) {
        auto* tile = makeCard();
        auto* layout = new QVBoxLayout(tile);
        layout->setContentsMargins(20, 18, 20, 18);
        layout->setSpacing(4);
        valueOut = new QLabel(QStringLiteral("—"));
        valueOut->setObjectName("statValue");
        layout->addWidget(valueOut);
        captionOut = makeLabel(caption, "muted");
        layout->addWidget(captionOut);
        return tile;
    };
    QLabel* weekCaption;
    QLabel* monthCaption;
    auto* tiles = new QHBoxLayout;
    tiles->setSpacing(14);
    tiles->addWidget(makeTile(QStringLiteral("hoje"), m_todayValue, m_todayCaption));
    tiles->addWidget(makeTile(QStringLiteral("esta semana"), m_weekValue, weekCaption));
    tiles->addWidget(makeTile(QStringLiteral("este mês"), m_monthValue, monthCaption));
    root->addLayout(tiles);
    root->addSpacing(14);

    // Lista dia a dia, agrupada por mês (cabeçalho com o saldo do mês).
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
        QStringLiteral("Dias sem ponto não geram débito · o dia em andamento entra no saldo "
                       "ao encerrar o expediente · a meta é configurável em Relatórios."),
        "muted");
    note->setWordWrap(true);
    root->addWidget(note);

    refresh();
}

void BankPage::refresh() {
    const int goal = AppSettings::journeySeconds();
    const QDate today = QDate::currentDate();
    const QTime now = QTime::currentTime();
    const QDate weekStart = today.addDays(-(today.dayOfWeek() - 1));  // segunda-feira

    m_subtitle->setText(
        QStringLiteral("Horas a mais (crédito) ou a menos (débito) que a meta diária de %1")
            .arg(formatDurationCompact(goal)));

    // Uma única varredura alimenta o saldo total, os cartões e a lista.
    struct Row {
        QDate date;
        int worked;
        int balance;
        bool open;  // expediente de hoje ainda em andamento
    };
    QList<Row> rows;
    QMap<QDate, int> monthSum;    // saldo fechado por mês (chave: dia 1)
    QMap<QDate, int> monthDays;   // quantos dias fechados o mês tem
    int total = 0, weekTotal = 0, monthTotal = 0, countedDays = 0;
    int todayBalance = 0;
    bool todayCounts = false, todayOpen = false;
    int maxAbs = 3600;  // escala mínima de 1h para as barrinhas

    for (const DayRecord& day : m_storage->allDays()) {
        if (!HourBank::dayCounts(day))
            continue;
        const bool isToday = day.date == today;
        const bool open = isToday && day.status() != DayRecord::Status::Done;
        const QTime closeAt = open ? now : QTime();
        const int worked = day.workedSeconds(closeAt);
        const int balance = HourBank::dayBalance(day, goal, closeAt);
        rows.append({day.date, worked, balance, open});
        maxAbs = qMax(maxAbs, qAbs(balance));
        if (isToday) {
            todayCounts = true;
            todayOpen = open;
            todayBalance = balance;
        }
        if (open)
            continue;  // o dia em andamento ainda não entra nos saldos
        const QDate monthKey(day.date.year(), day.date.month(), 1);
        monthSum[monthKey] += balance;
        ++monthDays[monthKey];
        total += balance;
        ++countedDays;
        if (day.date >= weekStart && day.date <= today)
            weekTotal += balance;
        if (day.date.year() == today.year() && day.date.month() == today.month())
            monthTotal += balance;
    }

    // Cartão principal.
    m_totalValue->setText(HourBank::formatBalance(total));
    setUiState(m_totalValue, "balance", balanceState(total));
    m_totalPill->setText(total > 0 ? QStringLiteral("crédito")
                         : total < 0 ? QStringLiteral("débito")
                                     : QStringLiteral("zerado"));
    setUiState(m_totalPill, "balance", balanceState(total));
    m_totalPill->setVisible(countedDays > 0);
    if (todayOpen)
        m_totalHint->setText(QStringLiteral(
            "O dia de hoje está em andamento e entra no saldo quando você encerrar o expediente."));
    else if (countedDays > 0)
        m_totalHint->setText(countedDays == 1
                                 ? QStringLiteral("Somando o único dia com ponto registrado.")
                                 : QStringLiteral("Somando os %1 dias com ponto registrado.")
                                       .arg(countedDays));
    else
        m_totalHint->setText(QStringLiteral(
            "Nenhum dia registrado ainda — o saldo começa a contar no primeiro ponto."));

    // Cartões de resumo.
    auto setTile = [](QLabel* value, int balance, const char* state) {
        value->setText(HourBank::formatBalance(balance));
        setUiState(value, "balance", state);
    };
    if (!todayCounts) {
        m_todayValue->setText(QStringLiteral("—"));
        setUiState(m_todayValue, "balance", "zero");
        m_todayCaption->setText(QStringLiteral("hoje"));
    } else {
        setTile(m_todayValue, todayBalance,
                todayOpen ? "partial" : balanceState(todayBalance));
        m_todayCaption->setText(todayOpen ? QStringLiteral("hoje · em andamento")
                                          : QStringLiteral("hoje"));
    }
    setTile(m_weekValue, weekTotal, balanceState(weekTotal));
    setTile(m_monthValue, monthTotal, balanceState(monthTotal));

    // Lista dia a dia.
    while (m_list->count() > 1) {  // preserva o stretch final
        QLayoutItem* item = m_list->takeAt(0);
        delete item->widget();
        delete item;
    }

    if (rows.isEmpty()) {
        m_list->insertWidget(0, makeLabel(QStringLiteral("Nenhum dia registrado ainda."), "muted"));
        return;
    }

    const QLocale locale;
    QDate currentMonth;
    for (const Row& row : rows) {
        const QDate monthKey(row.date.year(), row.date.month(), 1);
        if (monthKey != currentMonth) {
            currentMonth = monthKey;
            auto* header = new QWidget;
            auto* headerLayout = new QHBoxLayout(header);
            // respiro extra antes dos meses seguintes (o 1º cola no topo)
            headerLayout->setContentsMargins(4, m_list->count() > 1 ? 10 : 0, 4, 0);
            headerLayout->setSpacing(10);
            headerLayout->addWidget(
                makeLabel(locale.toString(monthKey, QStringLiteral("MMMM 'de' yyyy")), "h2"));
            headerLayout->addStretch();
            if (monthDays.contains(monthKey)) {
                headerLayout->addWidget(makeLabel(QStringLiteral("saldo do mês"), "muted"),
                                        0, Qt::AlignVCenter);
                headerLayout->addWidget(makeBalancePill(monthSum[monthKey]), 0, Qt::AlignVCenter);
            } else {
                headerLayout->addWidget(makeLabel(QStringLiteral("em andamento"), "muted"),
                                        0, Qt::AlignVCenter);
            }
            m_list->insertWidget(m_list->count() - 1, header);
        }

        auto* card = makeCard();
        auto* rowLayout = new QHBoxLayout(card);
        rowLayout->setContentsMargins(18, 14, 18, 14);
        rowLayout->setSpacing(14);

        auto* texts = new QVBoxLayout;
        texts->setSpacing(2);
        QString dateText =
            locale.toString(row.date, QStringLiteral("dddd, d 'de' MMMM"));
        if (row.date == today)
            dateText = QStringLiteral("Hoje · ") + dateText;
        texts->addWidget(makeLabel(dateText, "h2"));
        texts->addWidget(makeLabel(QStringLiteral("%1 trabalhadas · meta de %2")
                                       .arg(formatDuration(row.worked),
                                            formatDurationCompact(goal)),
                                   "muted"));
        rowLayout->addLayout(texts);
        rowLayout->addStretch();

        rowLayout->addWidget(new BalanceBar(row.balance, maxAbs, row.open), 0, Qt::AlignVCenter);
        auto* pill = makeBalancePill(row.balance, row.open);
        pill->setToolTip(row.open
                             ? QStringLiteral("Saldo parcial — o dia entra no banco "
                                              "quando o expediente é encerrado.")
                             : QStringLiteral("Saldo do dia: trabalhadas − meta."));
        rowLayout->addWidget(pill, 0, Qt::AlignVCenter);

        m_list->insertWidget(m_list->count() - 1, card);
    }
}
