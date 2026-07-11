#include "appsettings.h"

#include <QSettings>
#include <QtGlobal>

namespace {

QSettings settings() {
    return QSettings(QStringLiteral("tracker-horas"), QStringLiteral("tracker-horas"));
}

int clampJourney(int seconds) {
    return qBound(AppSettings::kMinJourneySeconds, seconds,
                  AppSettings::kMaxJourneySeconds);
}

}  // namespace

namespace AppSettings {

int journeySeconds() {
    bool ok = false;
    const int stored = settings()
                           .value(QStringLiteral("journeySeconds"), kDefaultJourneySeconds)
                           .toInt(&ok);
    return ok ? clampJourney(stored) : kDefaultJourneySeconds;
}

void setJourneySeconds(int seconds) {
    settings().setValue(QStringLiteral("journeySeconds"), clampJourney(seconds));
}

}  // namespace AppSettings
