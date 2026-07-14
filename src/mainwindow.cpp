#include "mainwindow.h"

#include "bankpage.h"
#include "icons.h"
#include "pages.h"
#include "punchedit.h"
#include "theme.h"
#include "timesheetpage.h"
#include "todaypage.h"

#include <QApplication>

#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

// Entrada da página nova: um deslize curto para cima, animando "pos".
// De propósito NÃO usa QGraphicsOpacityEffect: efeito gráfico no pai +
// sombras nos cards filhos = efeitos aninhados, que o Qt Widgets não
// suporta direito e renderiza com glitches.
static void slideIn(QWidget* page) {
    const QPoint end = page->pos();  // posição final, definida pelo stack
    auto* animation = new QPropertyAnimation(page, "pos", page);
    animation->setDuration(180);
    animation->setStartValue(end + QPoint(0, 16));
    animation->setEndValue(end);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("Tracker Horas"));
    resize(1060, 700);
    setMinimumSize(920, 620);

    m_todayPage = new TodayPage(&m_storage);
    m_sheetPage = new TimesheetPage(&m_storage);
    m_bankPage = new BankPage(&m_storage);
    m_historyPage = new HistoryPage(&m_storage);
    m_reportsPage = new ReportsPage(&m_storage);

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_todayPage);
    m_stack->addWidget(m_sheetPage);
    m_stack->addWidget(m_bankPage);
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

    auto addNav = [this, sideLayout](const QString& text, NavGlyph glyph, int index) {
        auto* button = new QPushButton(text);
        button->setProperty("nav", true);
        button->setCheckable(true);
        button->setAutoExclusive(true);
        button->setCursor(Qt::PointingHandCursor);
        // O QIcon tem estado Off/On: o Qt troca a cor sozinho ao marcar o botão.
        button->setIconSize(QSize(20, 20));
        m_navButtons.append({button, glyph});
        connect(button, &QPushButton::clicked, this, [this, index] {
            if (m_stack->currentIndex() == index)
                return;
            // Páginas de resumo são recalculadas ao entrar nelas.
            if (index == 1)
                m_sheetPage->refresh();
            else if (index == 2)
                m_bankPage->refresh();
            else if (index == 3)
                m_historyPage->refresh();
            else if (index == 4)
                m_reportsPage->refresh();
            m_stack->setCurrentIndex(index);
            slideIn(m_stack->currentWidget());
        });
        sideLayout->addWidget(button);
        return button;
    };
    addNav(QStringLiteral("Hoje"), NavGlyph::Today, 0)->setChecked(true);
    addNav(QStringLiteral("Folha"), NavGlyph::Sheet, 1);
    addNav(QStringLiteral("Banco de horas"), NavGlyph::Bank, 2);
    addNav(QStringLiteral("Histórico"), NavGlyph::History, 3);
    addNav(QStringLiteral("Relatórios"), NavGlyph::Reports, 4);

    sideLayout->addStretch();

    m_themeButton = new QPushButton;
    m_themeButton->setProperty("nav", true);
    m_themeButton->setCursor(Qt::PointingHandCursor);
    m_themeButton->setIconSize(QSize(20, 20));
    connect(m_themeButton, &QPushButton::clicked, this, [this] {
        Theme::toggle(qApp);       // regenera o QSS e repolimenta tudo
        refreshThemeIcons();       // ícones são pixmaps: precisam ser recriados
        m_todayPage->refresh();    // idem para os ícones das linhas de lista
    });
    sideLayout->addWidget(m_themeButton);
    sideLayout->addSpacing(6);

    auto* footer = new QLabel(QStringLiteral("v0.1 · dados salvos localmente"));
    footer->setObjectName("appSubtitle");
    footer->setWordWrap(true);
    sideLayout->addWidget(footer);

    refreshThemeIcons();

    // Se o dia de hoje foi corrigido pela Folha, a página Hoje precisa repintar.
    connect(m_sheetPage, &TimesheetPage::dayEdited, this, [this](const QDate& date) {
        if (date == QDate::currentDate())
            m_todayPage->refresh();
    });

    auto* central = new QWidget;
    auto* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(sidebar);
    layout->addWidget(m_stack, 1);
    setCentralWidget(central);

    // Turno que atravessou a meia-noite com o app fechado: se ontem ficou
    // com expediente aberto, perguntar como resolver assim que a janela abrir.
    QTimer::singleShot(0, this, [this] {
        const QDate yesterday = QDate::currentDate().addDays(-1);
        const DayRecord* day = m_storage.find(yesterday);
        if (!day)
            return;
        const DayRecord::Status status = day->status();
        if (status != DayRecord::Status::Working && status != DayRecord::Status::OnBreak)
            return;
        if (resolveOpenShiftDialog(&m_storage, yesterday, this))
            m_todayPage->refresh();
    });
}

void MainWindow::refreshThemeIcons() {
    for (const auto& [button, glyph] : m_navButtons)
        button->setIcon(navIcon(glyph));
    m_themeButton->setIcon(themeToggleIcon());
    m_themeButton->setText(Theme::isDark() ? QStringLiteral("Modo claro")
                                           : QStringLiteral("Modo escuro"));
}
