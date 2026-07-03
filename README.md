# Tracker Horas

App desktop Linux (Wayland nativo) de controle de ponto e tarefas diárias,
escrito em C++20 com Qt 6 Widgets e visual inspirado no Adwaita/GNOME.

## Funcionalidades

- **Hoje** — relógio de ponto ao vivo: registrar entrada, pausa, retorno e
  saída, com contador de horas trabalhadas atualizado a cada segundo, além da
  lista de tarefas do dia (adicionar, concluir, limpar concluídas).
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
  widgets.h       helpers de UI (cards, labels com papel, estado dinâmico)
  todaypage.*     página principal (ponto + tarefas)
  pages.*         Histórico e Relatórios
  mainwindow.*    janela com sidebar de navegação
assets/
  style.qss       tema claro estilo Adwaita
```

## Limitações conhecidas (esqueleto)

- Turnos que cruzam a meia-noite não são tratados.
- Não é possível editar/remover um registro de ponto já feito.
- Tema escuro ainda não implementado.
