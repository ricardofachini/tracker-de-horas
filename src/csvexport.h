#pragma once

#include <QDate>
#include <QString>

class Storage;
class QWidget;

// Exportação da folha de ponto em CSV: separador ';' e UTF-8 com BOM,
// para abrir direto no LibreOffice/Excel em português.

// Conteúdo do CSV de um mês: uma linha por dia + linha de total.
QString monthCsv(const Storage& storage, const QDate& month);

// Pergunta onde salvar e grava o CSV do mês. true = arquivo gravado.
bool exportMonthCsv(QWidget* parent, const Storage& storage, const QDate& month);
