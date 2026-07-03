#pragma once

#include "model.h"
#include <QMap>

// Persistência simples em JSON (~/.local/share/tracker-horas/data.json).
class Storage {
public:
    Storage();

    // Retorna o registro do dia, criando um vazio se não existir.
    DayRecord& day(const QDate& date);

    // Todos os dias registrados, do mais recente ao mais antigo.
    QList<DayRecord> allDays() const;

    void save() const;

private:
    void load();

    QString m_path;
    QMap<QDate, DayRecord> m_days;
};
