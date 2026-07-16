#pragma once

#include "icons.h"
#include "model.h"

#include <QDialog>

#include <functional>

class Storage;
class QLabel;
class QListWidget;
class QToolButton;

// Diálogo modal para corrigir (ou criar) um registro de ponto: tipo + horário.
// Pode ser chamado quantas vezes for preciso. Retorna true se o usuário
// salvou; `punch` sai atualizado.
bool editPunchDialog(QWidget* parent, Punch& punch,
                     const QString& title = QStringLiteral("Editar registro"));

// Confirmação estilizada para remoções. true = usuário confirmou.
bool confirmRemoveDialog(QWidget* parent, const QString& question, const QString& detail);

// Aviso estilizado com um único botão de OK.
void infoDialog(QWidget* parent, const QString& title, const QString& detail);

// Turno que atravessou a meia-noite com o app fechado (ou o PC suspenso):
// `openDay` ficou com o expediente aberto. Pergunta se o usuário continuou
// trabalhando (ponte para o dia seguinte, com ou sem saída na madrugada) ou
// se prefere corrigir os registros. Retorna true se os dados mudaram.
bool resolveOpenShiftDialog(Storage* storage, const QDate& openDay, QWidget* parent);

// Botão de ação compacto usado nas linhas de lista (editar, remover, play...).
QToolButton* makeRowActionButton(ActionGlyph glyph, const QColor& normal,
                                 const QColor& active, const QString& tooltip);

// Linha "HH:mm · Tipo" com botões de editar e remover à direita.
QWidget* makePunchRow(const Punch& punch,
                      const std::function<void()>& onEdit,
                      const std::function<void()>& onRemove);

// Edição dos registros de um dia inteiro — aberto pela Folha com clique
// duplo, para corrigir também dias passados.
class DayPunchesDialog : public QDialog {
public:
    DayPunchesDialog(Storage* storage, const QDate& date, QWidget* parent = nullptr);

    bool changed() const { return m_changed; }

private:
    void repopulate();
    void addPunch();

    Storage* m_storage;
    QDate m_date;
    bool m_changed = false;
    QListWidget* m_list;
    QLabel* m_empty;
    QLabel* m_totalLabel;
};
