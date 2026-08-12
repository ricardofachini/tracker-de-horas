#pragma once

// Preferências do usuário (QSettings, o mesmo arquivo do tema).
namespace AppSettings {

// Meta de jornada diária, em segundos. Usada pela barra de progresso da
// página Hoje e pela linha de meta do gráfico semanal.
constexpr int kDefaultJourneySeconds = 8 * 3600;
constexpr int kMinJourneySeconds = 15 * 60;          // 00:15
constexpr int kMaxJourneySeconds = 24 * 3600 - 60;   // 23:59

int journeySeconds();
void setJourneySeconds(int seconds);  // limitado a [min, max]

// Meta de horas mensais, em segundos. É a base do banco de horas e do
// "Saldo do mês" da Folha — o contrato do usuário é por horas no mês.
constexpr int kDefaultMonthlyGoalSeconds = 176 * 3600;  // 22 × 8h
constexpr int kMinMonthlyGoalSeconds = 1 * 3600;        // 01:00
constexpr int kMaxMonthlyGoalSeconds = 744 * 3600;      // 31 × 24h

int monthlyGoalSeconds();
void setMonthlyGoalSeconds(int seconds);  // limitado a [min, max]

}  // namespace AppSettings
