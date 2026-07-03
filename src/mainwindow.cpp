#include "mainwindow.h"

#include "pages.h"
#include "todaypage.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

// Fade-in da página recém-exibida: anima a propriedade "opacity" de um
// efeito temporário. DeleteWhenStopped faz o Qt liberar a animação sozinho.
static void fadeIn(QWidget* page) {
    auto* effect = new QGraphicsOpacityEffect(page);
    page->setGraphicsEffect(effect);
    auto* animation = new QPropertyAnimation(effect, "opacity", page);
    animation->setDuration(180);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(animation, &QPropertyAnimation::finished, page,
                     [page] { page->setGraphicsEffect(nullptr); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("Tracker Horas"));
    resize(1060, 700);
    setMinimumSize(920, 620);

    m_todayPage = new TodayPage(&m_storage);
    m_historyPage = new HistoryPage(&m_storage);
    m_reportsPage = new ReportsPage(&m_storage);

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_todayPage);
    m_stack->addWidget(m_historyPage);
    m_stack->addWidget(m_reportsPage);

    auto* sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(232);
    auto* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(16, 22, 16, 16);
    sideLayout->setSpacing(4);

    auto* appTitle = new QLabel(QStringLiteral("Tracker Horas"));
    appTitle->setObjectName("appTitle");
    auto* appSubtitle = new QLabel(QStringLiteral("controle de ponto e tarefas"));
    appSubtitle->setObjectName("appSubtitle");
    sideLayout->addWidget(appTitle);
    sideLayout->addWidget(appSubtitle);
    sideLayout->addSpacing(24);

    auto addNav = [this, sideLayout](const QString& text, int index) {
        auto* button = new QPushButton(text);
        button->setProperty("nav", true);
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, [this, index] {
            if (m_stack->currentIndex() == index)
                return;
            // Páginas de resumo são recalculadas ao entrar nelas.
            if (index == 1)
                m_historyPage->refresh();
            else if (index == 2)
                m_reportsPage->refresh();
            m_stack->setCurrentIndex(index);
            fadeIn(m_stack->currentWidget());
        });
        sideLayout->addWidget(button);
        return button;
    };
    addNav(QStringLiteral("Hoje"), 0)->setChecked(true);
    addNav(QStringLiteral("Histórico"), 1);
    addNav(QStringLiteral("Relatórios"), 2);

    sideLayout->addStretch();
    auto* footer = new QLabel(QStringLiteral("v0.1 · dados salvos localmente"));
    footer->setObjectName("appSubtitle");
    footer->setWordWrap(true);
    sideLayout->addWidget(footer);

    auto* central = new QWidget;
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(sidebar);
    layout->addWidget(m_stack, 1);
    setCentralWidget(central);
}
