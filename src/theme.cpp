#include "theme.h"

#include <QApplication>
#include <QFile>
#include <QSettings>

#include <cstring>
#include <vector>

namespace {

Theme::Mode s_mode = Theme::Mode::Light;

struct Token {
    const char* name;
    const char* light;
    const char* dark;
};

// A paleta inteira do app, lado a lado. Alfas em rgba() usam a forma
// percentual, que é a que o parser de QSS do Qt entende.
constexpr Token kTokens[] = {
    {"@window",            "#f5f5f4",                  "#1e1e23"},
    {"@sidebarBorder",     "#e5e3e0",                  "#26262c"},
    {"@sidebar",           "#f2f1ef",                  "#17171b"},
    {"@surfaceBorder",     "#eeece9",                  "#33333a"},
    {"@surfaceHover",      "#faf9f8",                  "#2e2e35"},
    {"@surface",           "#ffffff",                  "#28282e"},
    {"@hoverSolid",        "#f3f2f1",                  "#32323a"},
    {"@pressedSolid",      "#e8e6e3",                  "#3a3a43"},
    {"@hover",             "rgba(0, 0, 0, 6%)",        "rgba(255, 255, 255, 7%)"},
    {"@textBody",          "#3d3846",                  "#d5d2da"},
    {"@textSoft",          "#4a4550",                  "#b8b4c0"},
    {"@textMuted",         "#8b8690",                  "#97929f"},
    {"@text",              "#26232b",                  "#eceaf0"},
    {"@faint",             "#c9c5c1",                  "#55515b"},
    {"@track",             "#eceae8",                  "#33333a"},
    {"@separator",         "#f0eeec",                  "#313138"},
    {"@altRow",            "rgba(0, 0, 0, 2%)",        "rgba(255, 255, 255, 3%)"},
    {"@inputBorder",       "#d8d5d2",                  "#3f3f47"},
    {"@accentHover",       "#4a90e8",                  "#74acee"},
    {"@accentPressed",     "#1a5fb4",                  "#4a8ada"},
    {"@accentStrong",      "#1a5fb4",                  "#8db8f0"},
    {"@accentSoft",        "rgba(53, 132, 228, 14%)",  "rgba(98, 160, 234, 18%)"},
    {"@accent",            "#3584e4",                  "#62a0ea"},
    {"@successHover",      "#40cb8c",                  "#40cb8c"},
    {"@successPressed",    "#26a269",                  "#26a269"},
    {"@successText",       "#1c8656",                  "#5fdb9f"},
    {"@successSoft",       "#dff3e7",                  "rgba(46, 194, 126, 16%)"},
    {"@success",           "#2ec27e",                  "#2ec27e"},
    {"@warnText",          "#96650a",                  "#edc16d"},
    {"@warnSoft",          "#fdf0d0",                  "rgba(229, 165, 10, 15%)"},
    {"@dangerText",        "#c01c28",                  "#ff8b95"},
    {"@dangerSoftHover",   "#f4ccd0",                  "rgba(192, 28, 40, 36%)"},
    {"@dangerSoftPressed", "#eeb4ba",                  "rgba(192, 28, 40, 48%)"},
    {"@dangerSoft",        "#f9e0e2",                  "rgba(192, 28, 40, 24%)"},
    {"@timerOff",          "#a6a1ab",                  "#6b6773"},
    {"@scrollHover",       "#b5b1ad",                  "#56565f"},
    {"@scroll",            "#d0cdc9",                  "#45454d"},
    {"@tooltipBg",         "#26232b",                  "#0c0c0f"},
    {"@tooltipText",       "#ffffff",                  "#eceaf0"},
};

const char* tokenValue(const char* name) {
    for (const Token& token : kTokens)
        if (std::strcmp(token.name, name) == 0)
            return s_mode == Theme::Mode::Dark ? token.dark : token.light;
    return "#ff00ff";  // cor berrante para denunciar token faltando
}

QSettings settings() {
    return QSettings(QStringLiteral("tracker-horas"), QStringLiteral("tracker-horas"));
}

}  // namespace

namespace Theme {

Mode mode() { return s_mode; }
bool isDark() { return s_mode == Mode::Dark; }

Mode savedMode() {
    return settings().value(QStringLiteral("darkMode"), false).toBool() ? Mode::Dark
                                                                        : Mode::Light;
}

void apply(QApplication* app, Mode mode) {
    s_mode = mode;
    settings().setValue(QStringLiteral("darkMode"), mode == Mode::Dark);

    QFile file(QStringLiteral(":/style.qss"));
    if (!file.open(QIODevice::ReadOnly))
        return;
    QString qss = QString::fromUtf8(file.readAll());

    // Substitui nomes mais longos primeiro, senão "@accent" corromperia
    // "@accentHover" deixando um "Hover" solto no meio do QSS.
    std::vector<const Token*> ordered;
    for (const Token& token : kTokens)
        ordered.push_back(&token);
    std::sort(ordered.begin(), ordered.end(), [](const Token* a, const Token* b) {
        return std::strlen(a->name) > std::strlen(b->name);
    });
    for (const Token* token : ordered)
        qss.replace(QLatin1String(token->name),
                    QLatin1String(s_mode == Mode::Dark ? token->dark : token->light));

    // Reaplicar o stylesheet repolimenta todos os widgets vivos de uma vez.
    app->setStyleSheet(qss);
}

void toggle(QApplication* app) {
    apply(app, isDark() ? Mode::Light : Mode::Dark);
}

QColor accent() { return QColor::fromString(QLatin1String(tokenValue("@accent"))); }
QColor accentStrong() { return QColor::fromString(QLatin1String(tokenValue("@accentStrong"))); }
QColor iconMuted() { return QColor::fromString(QLatin1String(tokenValue("@textMuted"))); }
QColor weekendText() { return QColor::fromString(QLatin1String(tokenValue("@timerOff"))); }
QColor faintText() { return QColor::fromString(QLatin1String(tokenValue("@faint"))); }

QColor todayHighlight() {
    return isDark() ? QColor(0x2c, 0x36, 0x46) : QColor(0xef, 0xf5, 0xfd);
}

}  // namespace Theme
