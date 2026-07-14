#pragma once

#include "model.h"

// Banco de horas: crédito/débito de cada dia em relação à meta diária.
// Regras: só dias com registro de ponto contam (folga não gera débito) e
// o saldo do dia é trabalhado − meta (negativo quando faltou hora).
namespace HourBank {

// O dia entra no banco? (tem ao menos um registro de ponto)
bool dayCounts(const DayRecord& day);

// Saldo do dia em segundos. `now` fecha o expediente aberto de hoje,
// como em DayRecord::workedSeconds (passe QTime() para ignorá-lo).
int dayBalance(const DayRecord& day, int goalSeconds, const QTime& now = QTime());

// "+1h 05min", "−45min" ou "0min" — o negativo usa o menos tipográfico
// (U+2212), da mesma largura do "+", para os valores alinharem.
QString formatBalance(int seconds);

}  // namespace HourBank
