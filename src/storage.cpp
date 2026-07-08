#include "storage.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <algorithm>

static QString typeKey(PunchType t) {
    switch (t) {
    case PunchType::In: return QStringLiteral("in");
    case PunchType::BreakStart: return QStringLiteral("break_start");
    case PunchType::BreakEnd: return QStringLiteral("break_end");
    case PunchType::Out: return QStringLiteral("out");
    }
    return QStringLiteral("in");
}

static PunchType typeFromKey(const QString& key) {
    if (key == QLatin1String("break_start")) return PunchType::BreakStart;
    if (key == QLatin1String("break_end")) return PunchType::BreakEnd;
    if (key == QLatin1String("out")) return PunchType::Out;
    return PunchType::In;
}

Storage::Storage() {
    m_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
             + QStringLiteral("/data.json");
    load();
}

DayRecord& Storage::day(const QDate& date) {
    DayRecord& record = m_days[date];
    record.date = date;
    return record;
}

const DayRecord* Storage::find(const QDate& date) const {
    const auto it = m_days.constFind(date);
    return it == m_days.cend() ? nullptr : &it.value();
}

QList<DayRecord> Storage::allDays() const {
    QList<DayRecord> days = m_days.values();
    std::sort(days.begin(), days.end(),
              [](const DayRecord& a, const DayRecord& b) { return a.date > b.date; });
    return days;
}

void Storage::load() {
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonArray days = QJsonDocument::fromJson(file.readAll())
                                .object()
                                .value(QLatin1String("days"))
                                .toArray();
    for (const QJsonValue& value : days) {
        const QJsonObject obj = value.toObject();
        DayRecord record;
        record.date = QDate::fromString(obj.value(QLatin1String("date")).toString(), Qt::ISODate);
        if (!record.date.isValid())
            continue;
        for (const QJsonValue& pv : obj.value(QLatin1String("punches")).toArray()) {
            const QJsonObject po = pv.toObject();
            const QTime time = QTime::fromString(po.value(QLatin1String("time")).toString(),
                                                 QStringLiteral("HH:mm:ss"));
            if (time.isValid())
                record.punches.append({typeFromKey(po.value(QLatin1String("type")).toString()), time});
        }
        for (const QJsonValue& tv : obj.value(QLatin1String("tasks")).toArray()) {
            const QJsonObject to = tv.toObject();
            record.tasks.append({to.value(QLatin1String("text")).toString(),
                                 to.value(QLatin1String("done")).toBool()});
        }
        m_days.insert(record.date, record);
    }
}

void Storage::save() const {
    QJsonArray days;
    for (const DayRecord& record : m_days) {
        if (record.punches.isEmpty() && record.tasks.isEmpty())
            continue;
        QJsonArray punches;
        for (const Punch& p : record.punches)
            punches.append(QJsonObject{{QStringLiteral("type"), typeKey(p.type)},
                                       {QStringLiteral("time"), p.time.toString(QStringLiteral("HH:mm:ss"))}});
        QJsonArray tasks;
        for (const Task& t : record.tasks)
            tasks.append(QJsonObject{{QStringLiteral("text"), t.text},
                                     {QStringLiteral("done"), t.done}});
        days.append(QJsonObject{{QStringLiteral("date"), record.date.toString(Qt::ISODate)},
                                {QStringLiteral("punches"), punches},
                                {QStringLiteral("tasks"), tasks}});
    }

    QDir().mkpath(QFileInfo(m_path).path());
    QFile file(m_path);
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(QJsonObject{{QStringLiteral("days"), days}}).toJson());
}
