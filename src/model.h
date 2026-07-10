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

// Uma sessão de dedicação a uma tarefa. `end` inválido = em andamento.
struct TaskInterval {
    QTime start;
    QTime end;
};

struct Task {
    QString text;
    bool done = false;
    QList<TaskInterval> intervals;  // sessões de trabalho nesta tarefa

    bool isRunning() const;  // há um intervalo aberto?

    // Segundos dedicados à tarefa. Se ela está em andamento, `now` fecha
    // o intervalo aberto (passe QTime() para ignorá-lo).
    int spentSeconds(const QTime& now = QTime()) const;
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

    // Segundos em pausa, com a mesma convenção de `now`.
    int breakSeconds(const QTime& now = QTime()) const;

    QTime firstIn() const;  // primeira entrada do dia (QTime() se não houver)
    QTime lastOut() const;  // última saída do dia (QTime() se não houver)

    int runningTaskIndex() const;  // tarefa em andamento (-1 se nenhuma)

    // Reordena os registros por horário — necessário após editar um ponto,
    // pois status() e workedSeconds() dependem da ordem cronológica.
    void sortPunches();
};

QString punchLabel(PunchType type);

// "6h 42min" ou, com withSeconds, "06:42:15".
QString formatDuration(int seconds, bool withSeconds = false);
