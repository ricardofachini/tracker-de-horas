#pragma once

#include "model.h"
#include <QMap>

// Persistência simples em JSON (~/.local/share/tracker-horas/data.json).
class Storage {
public:
    Storage();

    // Retorna o registro do dia, criando um vazio se não existir.
    DayRecord& day(const QDate& date);

    // Consulta somente-leitura: nullptr se o dia não tem registro.
    const DayRecord* find(const QDate& date) const;

    // Todos os dias registrados, do mais recente ao mais antigo.
    QList<DayRecord> allDays() const;

    // Turno que cruza a meia-noite: fecha o expediente aberto de `date` com
    // Saída 23:59:59 e reabre o dia seguinte às 00:00:00 no mesmo estado
    // (trabalhando ou em pausa). A tarefa em andamento é pausada, como em
    // qualquer saída. Retorna false se o dia não tinha expediente aberto.
    bool bridgeMidnight(const QDate& date);

    void save() const;

private:
    void load();

    QString m_path;
    QMap<QDate, DayRecord> m_days;
};
