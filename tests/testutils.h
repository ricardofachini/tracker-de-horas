#pragma once

#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QTimer>
#include <QtTest>

#include <functional>

// Helpers compartilhados pelos testes de UI (páginas e diálogos).

// Botão com o texto exato, procurado nos descendentes de `root`.
inline QPushButton* findButton(QWidget* root, const QString& text) {
    for (QPushButton* button : root->findChildren<QPushButton*>())
        if (button->text() == text)
            return button;
    return nullptr;
}

// Aciona o botão com o texto dado. Usa click() em vez de eventos de mouse:
// dispensa show() e geometria calculada, que não existem no modo offscreen.
inline bool clickButton(QWidget* root, const QString& text) {
    QPushButton* button = findButton(root, text);
    if (!button)
        return false;
    button->click();
    return true;
}

// Agenda `action` para rodar no próximo diálogo modal aberto por exec().
// O timer dispara já dentro do event loop do diálogo; `action` deve fechá-lo
// (accept/reject/clique em botão), senão o exec() não retorna. Se nenhum
// diálogo aparecer depois de algumas tentativas, o teste falha.
inline void onNextModal(QObject* context, std::function<void(QDialog*)> action,
                        int attemptsLeft = 100) {
    QTimer::singleShot(5, context, [context, action = std::move(action), attemptsLeft] {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) {
            if (attemptsLeft > 0)
                onNextModal(context, std::move(action), attemptsLeft - 1);
            else
                QTest::qFail("nenhum diálogo modal apareceu", __FILE__, __LINE__);
            return;
        }
        action(dialog);
    });
}
