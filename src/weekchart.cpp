#include "weekchart.h"

#include "appsettings.h"
#include "model.h"
#include "theme.h"

#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

// Margens do plot: eixo Y à esquerda, rótulos de valor acima das barras
// e nomes dos dias abaixo.
static constexpr qreal kLeft = 48;
static constexpr qreal kRight = 12;
static constexpr qreal kTop = 26;
static constexpr qreal kBottom = 30;

WeekChart::WeekChart(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);  // hover por coluna, com tooltip
    m_seconds = QList<int>(7, 0);
}

void WeekChart::setWeek(const QDate& monday, const QList<int>& workedSeconds) {
    m_monday = monday;
    m_seconds = workedSeconds;
    while (m_seconds.size() < 7)
        m_seconds.append(0);
    m_hoverIndex = -1;
    update();
}

QRectF WeekChart::plotRect() const {
    return QRectF(kLeft, kTop, qMax<qreal>(width() - kLeft - kRight, 10),
                  qMax<qreal>(height() - kTop - kBottom, 10));
}

int WeekChart::topSeconds() const {
    // A meta é lida na hora de pintar (como as cores do Theme): mudou em
    // Relatórios, o próximo repaint já mostra certo.
    int maxSeconds = AppSettings::journeySeconds();
    for (int s : m_seconds)
        maxSeconds = qMax(maxSeconds, s);
    // Duas horas acima da hora cheia do maior valor (ou da meta), para o
    // rótulo da barra e a linha de meta respirarem.
    return (maxSeconds / 3600 + 2) * 3600;
}

int WeekChart::indexAt(const QPointF& pos) const {
    const QRectF plot = plotRect();
    if (!plot.adjusted(0, -kTop, 0, kBottom).contains(pos))
        return -1;
    const int index = int((pos.x() - plot.left()) / (plot.width() / 7.0));
    return index >= 0 && index < 7 ? index : -1;
}

void WeekChart::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF plot = plotRect();
    const qreal slot = plot.width() / 7.0;
    const int top = topSeconds();
    const QDate today = QDate::currentDate();
    const QLocale locale;

    QFont small = font();
    small.setPixelSize(11);
    QFont smallBold = small;
    smallBold.setBold(true);

    // Grade horizontal recessiva, a cada 2h, com rótulos no eixo Y.
    QColor grid = Theme::faintText();
    grid.setAlpha(90);
    painter.setFont(small);
    for (int secs = 0; secs <= top; secs += 2 * 3600) {
        const qreal y = plot.bottom() - plot.height() * secs / top;
        painter.setPen(QPen(grid, 1));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(Theme::iconMuted());
        painter.drawText(QRectF(0, y - 8, kLeft - 8, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("%1h").arg(secs / 3600));
    }

    // Realce sutil da coluna sob o cursor.
    if (m_hoverIndex >= 0) {
        QColor hover = Theme::iconMuted();
        hover.setAlpha(22);
        painter.setPen(Qt::NoPen);
        painter.setBrush(hover);
        painter.drawRoundedRect(
            QRectF(plot.left() + m_hoverIndex * slot + 2, plot.top(),
                   slot - 4, plot.height()),
            6, 6);
    }

    // Linha de meta, tracejada, com rótulo discreto.
    const int goalSeconds = AppSettings::journeySeconds();
    const qreal goalY = plot.bottom() - plot.height() * goalSeconds / top;
    QPen goalPen(Theme::accentStrong(), 1, Qt::DashLine);
    goalPen.setDashPattern({4, 4});
    painter.setPen(goalPen);
    painter.drawLine(QPointF(plot.left(), goalY), QPointF(plot.right(), goalY));
    painter.setFont(small);
    painter.setPen(Theme::iconMuted());
    painter.drawText(QRectF(plot.left(), goalY - 16, plot.width(), 14),
                     Qt::AlignRight | Qt::AlignBottom,
                     QStringLiteral("meta %1").arg(formatDurationCompact(goalSeconds)));

    // Barras: um único matiz (accent); a magnitude está no comprimento.
    const qreal barWidth = qBound<qreal>(18, slot * 0.52, 44);
    bool anyData = false;
    for (int i = 0; i < 7; ++i) {
        const int secs = m_seconds.value(i);
        const QDate date = m_monday.addDays(i);

        // Rótulo do dia ("seg 6"), hoje em negrito, fim de semana esmaecido.
        painter.setFont(date == today ? smallBold : small);
        painter.setPen(date == today ? Theme::accentStrong()
                       : date.dayOfWeek() >= 6 ? Theme::weekendText()
                                               : Theme::iconMuted());
        const QRectF labelRect(plot.left() + i * slot, plot.bottom() + 6, slot, 18);
        painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop,
                         QStringLiteral("%1 %2")
                             .arg(locale.dayName(date.dayOfWeek(), QLocale::ShortFormat))
                             .arg(date.day()));

        if (secs <= 0)
            continue;
        anyData = true;

        const qreal h = plot.height() * qMin(secs, top) / top;
        const QRectF bar(plot.left() + i * slot + (slot - barWidth) / 2,
                         plot.bottom() - h, barWidth, h);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Theme::accent());
        if (h > 4) {  // topo arredondado, base reta no eixo
            QPainterPath path;
            path.addRoundedRect(bar, 4, 4);
            path.addRect(QRectF(bar.left(), bar.bottom() - 4, bar.width(), 4));
            painter.drawPath(path.simplified());
        } else {
            painter.drawRect(bar);
        }

        // Valor sobre a barra, em tinta de texto (não na cor da série).
        painter.setFont(small);
        painter.setPen(Theme::iconMuted());
        painter.drawText(QRectF(plot.left() + i * slot, bar.top() - 18, slot, 16),
                         Qt::AlignHCenter | Qt::AlignBottom, formatDurationCompact(secs));
    }

    if (!anyData) {
        painter.setFont(font());
        painter.setPen(Theme::iconMuted());
        painter.drawText(plot, Qt::AlignCenter,
                         QStringLiteral("Nenhum registro nesta semana."));
    }
}

void WeekChart::mouseMoveEvent(QMouseEvent* event) {
    const int index = indexAt(event->position());
    if (index != m_hoverIndex) {
        m_hoverIndex = index;
        update();
    }
    if (index < 0) {
        QToolTip::hideText();
        return;
    }
    const QDate date = m_monday.addDays(index);
    const int secs = m_seconds.value(index);
    const QString when = QLocale().toString(date, QStringLiteral("dddd, d 'de' MMMM"));
    QToolTip::showText(event->globalPosition().toPoint(),
                       secs > 0 ? QStringLiteral("%1\n%2 trabalhadas")
                                      .arg(when, formatDuration(secs))
                                : QStringLiteral("%1\nsem registros").arg(when),
                       this);
}

void WeekChart::leaveEvent(QEvent*) {
    if (m_hoverIndex != -1) {
        m_hoverIndex = -1;
        update();
    }
    QToolTip::hideText();
}
