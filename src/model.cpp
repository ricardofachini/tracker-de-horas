#include "model.h"

DayRecord::Status DayRecord::status() const {
    if (punches.isEmpty())
        return Status::Off;
    switch (punches.last().type) {
    case PunchType::In:
    case PunchType::BreakEnd:
        return Status::Working;
    case PunchType::BreakStart:
        return Status::OnBreak;
    case PunchType::Out:
        return Status::Done;
    }
    return Status::Off;
}

int DayRecord::workedSeconds(const QTime& now) const {
    int total = 0;
    QTime start;
    bool working = false;
    for (const Punch& p : punches) {
        const bool opens = p.type == PunchType::In || p.type == PunchType::BreakEnd;
        if (opens && !working) {
            start = p.time;
            working = true;
        } else if (!opens && working) {
            total += start.secsTo(p.time);
            working = false;
        }
    }
    if (working && now.isValid())
        total += start.secsTo(now);
    return qMax(total, 0);
}

int DayRecord::breakSeconds(const QTime& now) const {
    int total = 0;
    QTime start;
    bool onBreak = false;
    for (const Punch& p : punches) {
        if (p.type == PunchType::BreakStart && !onBreak) {
            start = p.time;
            onBreak = true;
        } else if ((p.type == PunchType::BreakEnd || p.type == PunchType::Out) && onBreak) {
            total += start.secsTo(p.time);
            onBreak = false;
        }
    }
    if (onBreak && now.isValid())
        total += start.secsTo(now);
    return qMax(total, 0);
}

QTime DayRecord::firstIn() const {
    for (const Punch& p : punches)
        if (p.type == PunchType::In)
            return p.time;
    return {};
}

QTime DayRecord::lastOut() const {
    for (auto it = punches.crbegin(); it != punches.crend(); ++it)
        if (it->type == PunchType::Out)
            return it->time;
    return {};
}

QString punchLabel(PunchType type) {
    switch (type) {
    case PunchType::In: return QStringLiteral("Entrada");
    case PunchType::BreakStart: return QStringLiteral("Início da pausa");
    case PunchType::BreakEnd: return QStringLiteral("Fim da pausa");
    case PunchType::Out: return QStringLiteral("Saída");
    }
    return {};
}

QString formatDuration(int seconds, bool withSeconds) {
    seconds = qMax(seconds, 0);
    const int h = seconds / 3600;
    const int m = (seconds % 3600) / 60;
    const int s = seconds % 60;
    if (withSeconds)
        return QStringLiteral("%1:%2:%3")
            .arg(h, 2, 10, QChar('0'))
            .arg(m, 2, 10, QChar('0'))
            .arg(s, 2, 10, QChar('0'));
    if (h == 0)
        return QStringLiteral("%1min").arg(m);
    return QStringLiteral("%1h %2min").arg(h).arg(m, 2, 10, QChar('0'));
}
