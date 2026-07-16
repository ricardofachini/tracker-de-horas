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
    int adjustSeconds = 0;          // correção manual, somada ao cronômetro

    bool isRunning() const;  // há um intervalo aberto?

    // Segundos dedicados à tarefa: cronômetro + ajuste manual. Se ela está
    // em andamento, `now` fecha o intervalo aberto (passe QTime() para
    // ignorá-lo).
    int spentSeconds(const QTime& now = QTime()) const;
};

// Tudo que aconteceu em um dia de trabalho.
struct DayRecord {
    QDate date;
    QList<Punch> punches;
    QList<Task> tasks;

    enum class Status { Off, Working, OnBreak, Done };

    Status status() const;

    // Expediente ainda aberto (trabalhando ou em pausa, sem saída final)?
    bool hasOpenShift() const;

    // Segundos trabalhados. Se o expediente ainda está aberto, `now`
    // fecha o intervalo em andamento (passe QTime() para ignorá-lo).
    int workedSeconds(const QTime& now = QTime()) const;

    // Segundos em pausa, com a mesma convenção de `now`.
    int breakSeconds(const QTime& now = QTime()) const;

    QTime firstIn() const;  // primeira entrada do dia (QTime() se não houver)
    QTime lastOut() const;  // última saída do dia (QTime() se não houver)

    int runningTaskIndex() const;  // tarefa em andamento (-1 se nenhuma)

    // Fecha em `now` o intervalo da tarefa em andamento, se houver.
    // Retorna true se alguma tarefa estava correndo.
    bool pauseRunningTask(const QTime& now);

    // Reordena os registros por horário — necessário após editar um ponto,
    // pois status() e workedSeconds() dependem da ordem cronológica.
    void sortPunches();
};

QString punchLabel(PunchType type);

// "6h 42min" ou, com withSeconds, "06:42:15".
QString formatDuration(int seconds, bool withSeconds = false);

// "8h", "7h30" ou "45min" — para rótulos curtos (meta, valores do gráfico).
QString formatDurationCompact(int seconds);
