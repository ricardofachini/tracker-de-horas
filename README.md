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
  (seg–dom) com linha da meta, navegação entre semanas e detalhes no hover,
  desenhado com QPainter no estilo do app.
- **Meta de jornada configurável** — o campo "Meta de jornada diária" em
  Relatórios (padrão 8h) alimenta a barra de progresso da página Hoje e a
  linha de meta do gráfico semanal.
- **Turnos que cruzam a meia-noite** — com o app aberto, a virada do dia
  fecha o expediente às 23:59:59 e o reabre às 00:00:00 do dia seguinte no
  mesmo estado (trabalhando ou em pausa), sem perder horas. Se o app estava
  fechado (ou o PC suspenso), ao abrir ele pergunta se você seguiu
  trabalhando, até que horas foi, ou se prefere corrigir os registros.

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

## Testes

Testes unitários (Qt Test) cobrem o modelo de domínio, a persistência, a
ponte de meia-noite, a meta configurável e a exportação CSV:

```bash
ctest --test-dir build --output-on-failure
```

## Estrutura

```
src/
  main.cpp        ponto de entrada, estilo global
  model.{h,cpp}   tipos de domínio: Punch, Task, DayRecord
  storage.{h,cpp} persistência em JSON + ponte de meia-noite
  appsettings.*   preferências (meta de jornada diária)
  theme.*         paleta clara/escura e geração do QSS a partir de tokens
  widgets.h       helpers de UI (cards, labels com papel, estado dinâmico)
  card.*          card com sombra e hover animado
  icons.h         ícones desenhados com QPainter
  csvexport.*     exportação da folha mensal em CSV
  punchedit.*     edição de registros: diálogos e linhas com editar/remover
                  + diálogo de turno aberto na véspera
  todaypage.*     página principal (ponto + tarefas)
  timesheetmodel.* modelo da folha mensal (model/view)
  timesheetpage.*  página Folha (QTableView)
  weekchart.*     gráfico de barras semanal desenhado com QPainter
  pages.*         Histórico e Relatórios
  mainwindow.*    janela com sidebar de navegação
assets/
  style.qss       folha de estilo com tokens (@cor), estilo Adwaita
tests/
  tst_model.cpp        domínio: status, horas, pausas, formatação
  tst_storage.cpp      persistência JSON e ponte de meia-noite
  tst_appsettings.cpp  meta de jornada (padrão, limites)
  tst_csvexport.cpp    conteúdo do CSV mensal (aspas, totais)
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
- [x] Metas de jornada configuráveis (campo "Meta de jornada diária" na
      página Relatórios; padrão 8h).
- [x] Exportação de dados (CSV) — botão na Folha exporta o mês exibido
      (outros formatos ficam para depois).
- [x] Tema escuro com alternância por botão na sidebar (preferência salva;
      seguir o tema do sistema automaticamente fica para depois).
- [x] Tratar turnos que cruzam a meia-noite (fechamento automático às
      23:59:59 + reabertura 00:00:00 na virada do dia; diálogo de resolução
      quando o app estava fechado).
- [ ] Testes de UI (as páginas e diálogos ainda não têm testes automatizados;
      o domínio, a persistência e o CSV têm — veja `tests/`).
