# Tracker Horas

App desktop Linux (Wayland nativo) de controle de ponto e tarefas diárias,
escrito em C++20 com Qt 6 Widgets e visual inspirado no Adwaita/GNOME.

## Funcionalidades

- **Hoje** — relógio de ponto ao vivo: registrar entrada, pausa, retorno e
  saída, com contador de horas trabalhadas atualizado a cada segundo, barra
  de progresso da jornada de 8h, além da lista de tarefas do dia (adicionar,
  concluir, limpar concluídas). Cada registro de ponto pode ser editado
  (tipo e horário) ou removido quantas vezes for preciso, direto na lista.
- **Tarefas com cronômetro preemptivo** — cada tarefa tem play/pause com a
  hora atual do PC. Dar play em outra tarefa pausa automaticamente a que
  estava em andamento (comece a 2, volte para a 1, termine a 1, retome a 2);
  o tempo dedicado a cada uma fica somado na própria linha. Pausar ou
  encerrar o expediente também pausa a tarefa em andamento.
- **Folha** — folha de ponto mensal em formato de planilha: entrada, saída,
  pausas, horas trabalhadas e tarefas por dia, com navegação entre meses.
  Clique duplo em um dia abre o editor de registros, para corrigir, remover
  ou adicionar pontos também de dias passados. O botão "Exportar CSV" salva
  o mês exibido como planilha (separador `;`, UTF-8 com BOM, abre direto no
  LibreOffice/Excel), incluindo as tarefas e o tempo dedicado a cada uma.
- **Histórico** — lista dos dias registrados com total de horas e tarefas.
- **Relatórios** — resumo de horas de hoje, da semana, do mês e média diária,
  além do gráfico de barras "Horas por semana": horas trabalhadas por dia
  (seg–dom) com linha da meta de 8h, navegação entre semanas e detalhes no
  hover, desenhado com QPainter no estilo do app.

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
  csvexport.*     exportação da folha mensal em CSV
  punchedit.*     edição de registros: diálogos e linhas com editar/remover
  todaypage.*     página principal (ponto + tarefas)
  timesheetmodel.* modelo da folha mensal (model/view)
  timesheetpage.*  página Folha (QTableView)
  weekchart.*     gráfico de barras semanal desenhado com QPainter
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
- [x] Editar/remover registros de ponto já feitos (na página Hoje e pela
      Folha com clique duplo, incluindo dias passados).
- [x] Cronômetro por tarefa com preempção: play/pause em cada tarefa, só
      uma corre por vez e trocar de tarefa pausa a anterior.
- [x] Gráficos por semana nos Relatórios (barras por dia com linha de meta,
      navegação ‹ › entre semanas e tooltip por dia).
- [ ] Metas de jornada configuráveis (a meta de 8h ainda é fixa).
- [x] Exportação de dados (CSV) — botão na Folha exporta o mês exibido
      (outros formatos ficam para depois).
- [x] Tema escuro com alternância por botão na sidebar (preferência salva;
      seguir o tema do sistema automaticamente fica para depois).
- [ ] Tratar turnos que cruzam a meia-noite.
