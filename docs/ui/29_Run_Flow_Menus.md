# 29 — Menus do Fluxo de Run (Pause · Boon · Morte · Hub) · 🟢 P0 (boon) / 🟡 P1

> As telas que aparecem **durante e entre** as runs: pause, **seleção de boon** (a UI mais importante do loop), morte/vitória, e o hub de meta-progressão. Pré-req: [23 — UI Overview](23_UI_Overview.md), [03 — Core Loop](../design/03_Core_Loop_Roguelike.md), [03b — Boons](../design/03b_Reward_System.md).

---

## 1. Mapa do fluxo

```
[Gameplay+HUD] ──(ESC)──→ [Pause]
      │
   (limpa sala)
      ↓
 [Seleção de Boon] ──escolhe──→ próxima sala
      │
   (morre/vence)
      ↓
 [Morte / Vitória] ──→ [Hub] ──(gasta Essência)──→ [Nova Run]
```

---

## 2. Pause · 🟡 P1

```
┌──────────────────────┐
│       PAUSA          │
│  ▶ Continuar         │  ← foco; fecha e despausa
│    Ajustes           │  → Settings (27), jogo segue pausado
│    Abandonar Run     │  → modal: "perde o progresso da run?"
│    Sair pro Menu     │  → modal → Main Menu (24)
└──────────────────────┘
```

- `SetPause(true)` + input **UIOnly** ([23 §3](23_UI_Overview.md)).
- **Abandonar Run:** encerra a run (converte desempenho em Essência, [03 §5](../design/03_Core_Loop_Roguelike.md)) → Hub.
- Camada menu (3). Despausar restaura input GameOnly.

> ⚠️ Roguelike: pausar **não** é save. Se fechar o jogo no pause sem run-resume ([25 §4](25_Save_Load_Slots.md)), a run se perde — deixe isso claro no "Sair".

---

## 3. 🌟 Seleção de Boon · 🟢 P0 (a UI do coração do jogo)

Aparece ao **limpar uma sala** ([03 §3-4](../design/03_Core_Loop_Roguelike.md)). É **onde a build acontece** → é UI **gameplay-crítica**, não menu de borda. Referência: Boons do Hades, Deuses do Death Must Die.

```
┌──────────────────────────────────────────────────────────────┐
│              ESCOLHA UMA BÊNÇÃO                               │
│  ┌────────────┐   ┌────────────┐   ┌────────────┐           │
│  │ 🔥 LENDÁRIO │   │ 🩸 RARO     │   │ ⚡ COMUM    │           │
│  │ Slam vira  │   │ Combo aéreo │   │ +15% dano  │           │
│  │ AoE de fogo│   │ cura 5 HP   │   │            │           │
│  │            │   │             │   │            │           │
│  │ ~ Tempestade│   │ ~ Carniça   │   │ ~ Fúria     │  ← família│
│  │ ⚡ sinergia! │   │             │   │             │           │
│  └────────────┘   └────────────┘   └────────────┘           │
│   [A] Escolher   (sem "pular" no MVP — sempre pega 1)        │
└──────────────────────────────────────────────────────────────┘
```

| Elemento do card | Lê de | Por quê |
|---|---|---|
| **Nome + descrição** | [03b](../design/03b_Reward_System.md) (pool de boons) | o que o boon faz |
| **Cor da borda = raridade** | tier ([03b §5](../design/03b_Reward_System.md)) | cobiça (Comum/Raro/Lendário) |
| **Família** (ícone/tag) | família temática ([03b §3](../design/03b_Reward_System.md)) | identidade de build |
| **Dica de sinergia** | checa boons já pegos ([03b §4](../design/03b_Reward_System.md)) | "isto combina com o que você tem" → decisão melhor |
| **"transforma o combo"** | regra de ouro ([03b §1](../design/03b_Reward_System.md)) | destaca boons que mudam *como* você joga |

> 🎰 **A leitura desta tela é decisão, não menu.** Mostre raridade (cor), família e **sinergia** — é o que torna "1 de 3" uma escolha empolgante e não um clique. Navegável por gamepad (foco no card do meio).

> 🟢 **P0 no MVP:** sem o boon-select, não há build → não há roguelike. Pode ser **feio**, mas tem que **comunicar** nome/raridade/sinergia.

---

## 4. Morte / Vitória · 🟡 P1

```
┌──────────────────────────────────┐
│         VOCÊ MORREU              │   (ou: RIFT FECHADA — vitória)
│                                  │
│  Salas limpas:        4          │
│  Inimigos:            87         │
│  Melhor combo:        x22 (A)    │
│  Essência ganha:    🟣 +56       │  ← o progresso! (Pilar 3)
│                                  │
│  ▶ Continuar (→ Hub)             │
│    Sair pro Menu                 │
└──────────────────────────────────┘
```

- Mostra o **resumo da run** + **Essência ganha** — a morte tem que *dar progresso* ([Pilar 3, 01 §2](../design/01_Game_Vision.md)).
- "Continuar" → Hub (gastar Essência) → nova run. O loop fecha aqui.
- Vitória usa a **mesma tela**, texto/tom diferente.

> 🎯 A frase "**você ficou X mais forte**" (ou a Essência ganha) é o que faz o jogador apertar "de novo". É o anzol do meta-loop ([03 §5](../design/03_Core_Loop_Roguelike.md)).

---

## 5. Hub (gastar Essência) · 🟡 P1

O espaço entre runs. Pode ser uma **tela** (simples) ou um **nível** (com NPC/altar) — no MVP, uma tela já serve.

```
┌──────────────────────────────────────────────┐
│  SANTUÁRIO            🟣 Essência: 340         │
│  ┌──────────────┐ ┌──────────────┐            │
│  │ +5% vida ini │ │ Desbloq. boon │            │
│  │   🟣 80       │ │ família Vazio │            │
│  │  [Comprar]   │ │   🟣 150      │            │
│  └──────────────┘ └──────────────┘            │
│                                               │
│           ▶  INICIAR NOVA RUN                  │
└──────────────────────────────────────────────┘
```

- Compra upgrades **permanentes** ([03 §5](../design/03_Core_Loop_Roguelike.md), [03b §7](../design/03b_Reward_System.md)) — que **destravam escolhas/mecânicas**, não só +%.
- Comprar = autosave ([25 §3](25_Save_Load_Slots.md)).
- "Iniciar Nova Run" → loading → gameplay.

> 🔗 [03b §7](../design/03b_Reward_System.md): meta-upgrade bom **destrava** (nova família de boon, +1 slot de escolha, habilidade inicial), não só engorda um número.

---

## 6. Prioridade MVP

| Tela | Prioridade |
|---|---|
| **Seleção de Boon** | 🟢 **P0** (sem ela não há loop) |
| Morte/Vitória (resumo + Essência) | 🟡 P1 |
| Hub (gastar Essência) | 🟡 P1 |
| Pause | 🟡 P1 |
| Animações/cards bonitos/SFX | 🔵 P2 |

> 🎯 Ordem real: **boon-select** (M4, com o loop) → **morte+hub** (M5, fecha o meta) → pause (qualquer hora). Pause é o mais fácil; boon-select é o mais importante.

---

## 7. Checklist

- [ ] **Boon-select** mostra nome + raridade (cor) + família + **sinergia** ([03b](../design/03b_Reward_System.md))
- [ ] Boon-select navegável por gamepad (foco inicial)
- [ ] Morte/Vitória mostra resumo + **Essência ganha** (o anzol)
- [ ] Hub gasta Essência → autosave ([25](25_Save_Load_Slots.md))
- [ ] Pause: SetPause + UIOnly; Abandonar/Sair com modal
- [ ] Telas reusam input mode/foco via `UDDRUIManager` ([23 §5](23_UI_Overview.md))

---

## 8. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Boon-select sente "menu", não decisão | Sem raridade/sinergia visível | §3 — cor + dica de sinergia |
| Jogo roda durante o pause | `SetPause` não chamado / input errado | §2 |
| Morte não dá sensação de progresso | Não mostra Essência ganha | §4 |
| Boons não aparecem na tela | UI não lê o pool/RunManager | §3 — bind ao [03b](../design/03b_Reward_System.md)/RunManager |
| Essência some | Compra não fez autosave | §5 / [25 §3](25_Save_Load_Slots.md) |

---

## 9. Próximo passo

UI completa. → Volte ao [Index](../00_Index.md) ou ao [Roadmap (17)](../17_Implementation_Roadmap.md). A UI segue o roadmap: **HUD + boon-select** com o combate/loop (M1-M4); menus de borda (main/settings/pause) no M4-M5.
