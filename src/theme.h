#pragma once

#include <QColor>

class QApplication;

// Sistema de temas: o style.qss usa tokens (@surface, @text, ...) e este
// módulo os substitui pelas cores do modo ativo antes de aplicar o QSS.
// Trocar de tema = gerar o QSS de novo e reaplicar no QApplication.
namespace Theme {

enum class Mode { Light, Dark };

Mode mode();
bool isDark();
Mode savedMode();  // último modo escolhido (QSettings)

void apply(QApplication* app, Mode mode);
void toggle(QApplication* app);

// Cores que o lado C++ precisa (ícones, sombras e papéis do model/view).
QColor accent();
QColor accentStrong();
QColor iconMuted();
QColor todayHighlight();
QColor weekendText();
QColor faintText();
QColor successText();
QColor warnText();
QColor dangerText();

}  // namespace Theme
