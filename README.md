# Tracker Horas

App desktop Linux (Wayland nativo) de controle de ponto e tarefas diárias,
escrito em C++20 com Qt 6 Widgets e visual inspirado no Adwaita/GNOME.

## Funcionalidades

- **Hoje** — relógio de ponto ao vivo: registrar entrada, pausa, retorno e
  saída, com contador de horas trabalhadas atualizado a cada segundo, barra
  de progresso da jornada de 8h, além da lista de tarefas do dia (adicionar,
  concluir, limpar concluídas).
- **Folha** — folha de ponto mensal em formato de planilha: entrada, saída,
  pausas, horas trabalhadas e tarefas por dia, com navegação entre meses.
- **Histórico** — lista dos dias registrados com total de horas e tarefas.
- **Relatórios** — resumo de horas de hoje, da semana, do mês e média diária
  (gráficos e exportação planejados).

Os dados são salvos em JSON em `~/.local/share/tracker-horas/data.json`.

## Compilar e executar

Requisitos: CMake ≥ 3.19, compilador C++20 e Qt 6 (`qt6-base-dev` no Ubuntu).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/tracker-horas
```

O app roda nativamente em Wayland (`QT_QPA_PLATFORM=wayland;xcb` por padrão,
com fallback para X11).

## Estrutura

```
src/
  main.cpp        ponto de entrada, estilo global
  model.{h,cpp}   tipos de domínio: Punch, Task, DayRecord
  storage.{h,cpp} persistência em JSON
  theme.*         paleta clara/escura e geração do QSS a partir de tokens
  widgets.h       helpers de UI (cards, labels com papel, estado dinâmico)
  card.*          card com sombra e hover animado
  icons.h         ícones desenhados com QPainter
  todaypage.*     página principal (ponto + tarefas)
  timesheetmodel.* modelo da folha mensal (model/view)
  timesheetpage.*  página Folha (QTableView)
  pages.*         Histórico e Relatórios
  mainwindow.*    janela com sidebar de navegação
assets/
  style.qss       folha de estilo com tokens (@cor), estilo Adwaita
```

## Roadmap

- [x] **Folha de ponto mensal** — visualização tipo planilha do mês:
      uma linha por dia com entrada, saída, pausas, horas trabalhadas e
      tarefas, com navegação entre meses e total mensal (página "Folha",
      `QAbstractTableModel` + `QTableView`).
- [ ] Editar/remover registros de ponto já feitos.
- [ ] Gráficos por semana e metas de jornada nos Relatórios.
- [ ] Exportação de dados (CSV).
- [x] Tema escuro com alternância por botão na sidebar (preferência salva;
      seguir o tema do sistema automaticamente fica para depois).
- [ ] Tratar turnos que cruzam a meia-noite.
