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

}  // namespace AppSettings
