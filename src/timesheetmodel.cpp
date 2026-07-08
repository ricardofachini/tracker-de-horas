#include "timesheetmodel.h"

#include "storage.h"
#include "theme.h"

#include <QColor>
#include <QFont>
#include <QLocale>

TimesheetModel::TimesheetModel(Storage* storage, QObject* parent)
    : QAbstractTableModel(parent), m_storage(storage) {
    const QDate today = QDate::currentDate();
    m_month = QDate(today.year(), today.month(), 1);
}

void TimesheetModel::setMonth(const QDate& firstDay) {
    // Reset avisa a view de que tudo mudou; ela então repinta perguntando
    // os dados de novo. É assim que view e modelo se mantêm em sincronia.
    beginResetModel();
    m_month = QDate(firstDay.year(), firstDay.month(), 1);
    endResetModel();
}

int TimesheetModel::monthTotalSeconds() const {
    const QDate today = QDate::currentDate();
    int total = 0;
    for (int row = 0; row < rowCount(); ++row) {
        const QDate date = dateForRow(row);
        if (const DayRecord* day = m_storage->find(date))
            total += day->workedSeconds(date == today ? QTime::currentTime() : QTime());
    }
    return total;
}

int TimesheetModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_month.daysInMonth();
}

int TimesheetModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QDate TimesheetModel::dateForRow(int row) const {
    return m_month.addDays(row);
}

QString TimesheetModel::cellText(const DayRecord* day, const QDate& date, Column column) const {
    static const QString empty = QStringLiteral("—");
    const bool isToday = date == QDate::currentDate();
    const QTime now = isToday ? QTime::currentTime() : QTime();

    switch (column) {
    case Day:
        return QStringLiteral("%1 · %2")
            .arg(date.day(), 2, 10, QChar('0'))
            .arg(QLocale().dayName(date.dayOfWeek(), QLocale::ShortFormat));
    case In: {
        const QTime t = day ? day->firstIn() : QTime();
        return t.isValid() ? t.toString(QStringLiteral("HH:mm")) : empty;
    }
    case Out: {
        const QTime t = day ? day->lastOut() : QTime();
        return t.isValid() ? t.toString(QStringLiteral("HH:mm")) : empty;
    }
    case Breaks: {
        const int secs = day ? day->breakSeconds(now) : 0;
        return secs > 0 ? formatDuration(secs) : empty;
    }
    case Worked: {
        const int secs = day ? day->workedSeconds(now) : 0;
        return secs > 0 ? formatDuration(secs) : empty;
    }
    case Tasks:
        return day && !day->tasks.isEmpty() ? QString::number(day->tasks.size()) : empty;
    case ColumnCount:
        break;
    }
    return {};
}

QVariant TimesheetModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return {};
    const QDate date = dateForRow(index.row());
    const DayRecord* day = m_storage->find(date);
    const Column column = Column(index.column());

    switch (role) {
    case Qt::DisplayRole:
        return cellText(day, date, column);
    case Qt::TextAlignmentRole:
        return column == Day ? QVariant(int(Qt::AlignLeft | Qt::AlignVCenter))
                             : QVariant(int(Qt::AlignCenter));
    case Qt::FontRole:
        if (date == QDate::currentDate()) {
            QFont font;
            font.setBold(true);
            return font;
        }
        return {};
    case Qt::BackgroundRole:
        if (date == QDate::currentDate())
            return Theme::todayHighlight();  // faixa azulada no dia atual
        return {};
    case Qt::ForegroundRole:
        if (date.dayOfWeek() >= 6)  // fim de semana esmaecido
            return Theme::weekendText();
        if (cellText(day, date, column) == QStringLiteral("—"))
            return Theme::faintText();
        return {};
    }
    return {};
}

QVariant TimesheetModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal)
        return {};
    if (role == Qt::DisplayRole) {
        switch (Column(section)) {
        case Day: return QStringLiteral("Dia");
        case In: return QStringLiteral("Entrada");
        case Out: return QStringLiteral("Saída");
        case Breaks: return QStringLiteral("Pausas");
        case Worked: return QStringLiteral("Trabalhadas");
        case Tasks: return QStringLiteral("Tarefas");
        case ColumnCount: break;
        }
    }
    if (role == Qt::TextAlignmentRole)
        return section == Day ? QVariant(int(Qt::AlignLeft | Qt::AlignVCenter))
                              : QVariant(int(Qt::AlignCenter));
    return {};
}
