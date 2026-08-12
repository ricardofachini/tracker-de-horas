#include "hourbank.h"

namespace HourBank {

bool dayCounts(const DayRecord& day) {
    return !day.punches.isEmpty();
}

int monthBalance(int workedSeconds, int monthlyGoalSeconds) {
    return workedSeconds - monthlyGoalSeconds;
}

QString formatBalance(int seconds) {
    if (seconds == 0)
        return QStringLiteral("0min");
    const QChar sign = seconds > 0 ? QChar(u'+') : QChar(u'−');
    return sign + formatDuration(qAbs(seconds));
}

}  // namespace HourBank
