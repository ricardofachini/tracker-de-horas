#pragma once

#include <QDate>
#include <QWidget>

class Storage;
class TimesheetModel;
class QLabel;
class QPushButton;

// Página "Folha": planilha do mês com entrada, saída e horas por dia.
// Clique duplo em um dia abre o diálogo de correção dos registros.
class TimesheetPage : public QWidget {
    Q_OBJECT

public:
    explicit TimesheetPage(Storage* storage, QWidget* parent = nullptr);

    void refresh();  // re-lê o mês exibido (chamado ao entrar na página)

signals:
    void dayEdited(const QDate& date);  // registros alterados pelo diálogo

private:
    void setMonth(const QDate& firstDay);

    TimesheetModel* m_model;
    QLabel* m_monthLabel;
    QLabel* m_totalLabel;
    QPushButton* m_nextButton;
};
