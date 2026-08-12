#pragma once

#include <QWidget>

class Storage;
class QLabel;
class QProgressBar;
class QVBoxLayout;

// Página "Banco de horas": saldo acumulado de crédito/débito em relação à
// meta mensal (contrato por horas no mês), com o mês em andamento à parte e
// a lista dos meses já fechados. Meses sem ponto não geram débito; o mês
// atual só entra no saldo acumulado quando termina.
class BankPage : public QWidget {
public:
    explicit BankPage(Storage* storage, QWidget* parent = nullptr);

    void refresh();  // recalcula tudo (chamado ao entrar na página)

private:
    Storage* m_storage;
    QLabel* m_subtitle;
    QLabel* m_totalValue;
    QLabel* m_totalPill;
    QLabel* m_totalHint;
    QLabel* m_currentMonthLabel;
    QProgressBar* m_monthBar;
    QLabel* m_monthCaption;
    QVBoxLayout* m_list;
};
