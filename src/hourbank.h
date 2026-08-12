#pragma once

#include "model.h"

// Banco de horas: crédito/débito de cada mês em relação à meta mensal
// (o contrato é por horas no mês). Regras: só meses com registro de ponto
// contam (mês vazio não gera débito) e o saldo do mês é trabalhado − meta
// (negativo quando faltou hora).
namespace HourBank {

// O dia entra no banco? (tem ao menos um registro de ponto) — usado para
// decidir se um mês teve atividade.
bool dayCounts(const DayRecord& day);

// Saldo do mês em segundos: trabalhadas no mês − meta mensal. Quem soma as
// horas trabalhadas do mês é o chamador (varre os dias do mês).
int monthBalance(int workedSeconds, int monthlyGoalSeconds);

// "+1h 05min", "−45min" ou "0min" — o negativo usa o menos tipográfico
// (U+2212), da mesma largura do "+", para os valores alinharem.
QString formatBalance(int seconds);

}  // namespace HourBank
