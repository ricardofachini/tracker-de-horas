#pragma once

#include <QDate>
#include <QList>
#include <QString>
#include <QTime>

// Um registro de ponto: entrada, pausa, retorno ou saída.
enum class PunchType { In, BreakStart, BreakEnd, Out };

struct Punch {
    PunchType type;
    QTime time;
};

struct Task {
    QString text;
    bool done = false;
};

// Tudo que aconteceu em um dia de trabalho.
struct DayRecord {
    QDate date;
    QList<Punch> punches;
    QList<Task> tasks;

    enum class Status { Off, Working, OnBreak, Done };

    Status status() const;

    // Segundos trabalhados. Se o expediente ainda está aberto, `now`
    // fecha o intervalo em andamento (passe QTime() para ignorá-lo).
    int workedSeconds(const QTime& now = QTime()) const;
};

QString punchLabel(PunchType type);

// "6h 42min" ou, com withSeconds, "06:42:15".
QString formatDuration(int seconds, bool withSeconds = false);
