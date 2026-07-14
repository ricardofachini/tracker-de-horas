#pragma once

#include <QWidget>

class Storage;
class QLabel;
class QVBoxLayout;

// Página "Banco de horas": saldo acumulado de crédito/débito em relação à
// meta diária, com resumo (hoje, semana, mês) e a lista dia a dia agrupada
// por mês. Dias sem ponto não geram débito; o dia em andamento só entra no
// saldo quando o expediente é encerrado.
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
    QLabel* m_todayValue;
    QLabel* m_todayCaption;
    QLabel* m_weekValue;
    QLabel* m_monthValue;
    QVBoxLayout* m_list;
};
