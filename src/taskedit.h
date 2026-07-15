#pragma once

#include "model.h"

class QWidget;

// Diálogo modal para lançar ou corrigir manualmente o tempo dedicado a uma
// tarefa. O valor digitado vira o total da tarefa naquele momento; a
// diferença para o cronômetro fica guardada em Task::adjustSeconds, e o
// cronômetro segue contando normalmente a partir daí. Retorna true se o
// usuário salvou um valor diferente do exibido.
bool editTaskTimeDialog(QWidget* parent, Task& task);

// Diálogo mostrado ao registrar a entrada: oferece escolher uma tarefa
// pendente (ou criar uma nova) que começa a correr às `start`. Preempção
// como no play das linhas: a tarefa em andamento, se houver, é pausada.
// Retorna true se alguma tarefa foi iniciada; `day` sai atualizado.
bool askTaskOnPunchInDialog(QWidget* parent, DayRecord& day, const QTime& start);
