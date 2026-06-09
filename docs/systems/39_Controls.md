# 39 — Controles (Teclado & Gamepad) · 🟢 P0

> **Mapa canônico** de teclas e botões do Dark Dungeon Rift: o que o jogador aperta em combate, menus e UI. Complementa a implementação técnica do [07 — Input](07_Input.md) (Enhanced Input, GAS, buffer). **Modelo A** (orient-to-movement, [06 §3](06_Camera_TopDown.md)) no MVP.

> 🎯 **Regra:** um binding por verbo de gameplay. O HUD ([28](../ui/28_HUD.md)) e os tutoriais mostram **estes símbolos** — não invente labels diferentes por tela.

---

## 1. Visão rápida — combate (MVP)

### Teclado + mouse

```
        [W]
    [A] [S] [D]          MOUSE
      MOVIMENTO          LMB ── Ataque (combo)
                         (mira: personagem gira p/ movimento — Modelo A)

    Space ── Dash / esquiva (única esquiva do MVP)
    V     ── Pulo (jump-cancel no ar)
    Shift ── Sprint (segurar)
    E     ── Launcher (uppercut → combo aéreo)
    R     ── Slam (finisher aéreo)
    F     ── Interagir (porta, loot)
    Esc   ── Pause
```

### Gamepad (layout Xbox · equivalente PlayStation)

```
    [LB]  [LT]     [RT]  [RB]
      │              │     └── Sprint (segurar)
      │              └──────── (reservado P2)
      └────────────────────── Pulo (jump-cancel)

         [Y/△]              [B/○]
          Slam              Launcher
         [X/□]              [A/✕]
         Ataque              Dash

    [D-pad]                    [LS]──── Move
                               [RS]──── (livre no Modelo A)

    [View/Share]  [Menu/Options]  → Pause
```

| Verbo | Teclado | Xbox | PlayStation | Input Action | Ability / sistema |
|---|---|---|---|---|---|
| **Mover** | WASD | LS | LS | `IA_Move` | CMC |
| **Ataque / combo** | **LMB** | **X** | **□** | `IA_Attack` | `GA_Attack_Light` / `GA_AirAttack` (gate por `InAir`) |
| **Dash** | **Space** | **A** | **✕** | `IA_Dash` | `GA_Dash` |
| **Pulo** | **V** | **LB** | **L1** | `IA_Jump` | CMC + jump-cancel aéreo ([19](../combat/19_Abilities_Deep.md)) |
| **Sprint** | **Shift** (hold) | **RB** | **R1** | `IA_Sprint` | Gait ([09](../locomotion/09_Gaits.md)) |
| **Launcher** | **E** | **B** | **○** | `IA_Launcher` | `GA_Launcher` |
| **Slam** | **R** | **Y** | **△** | `IA_AirSlam` | `GA_AirSlam` |
| **Interagir** | **F** | **X** (hold) ou **RT** | **□** hold / **R2** | `IA_Interact` | Porta, pickup |
| **Pause** | **Esc** | **Menu** | **Options** | `IA_Pause` | Menu pause ([29](../ui/29_Run_Flow_Menus.md)) |

> ⚠️ **Q** aparece no HUD ([28](../ui/28_HUD.md)) como slot de habilidade futura — **sem binding no MVP** (reservado pra ultimate / Eco que concede ability, P2).

---

## 2. Por que estes bindings?

| Decisão | Motivo |
|---|---|
| **LMB / X = ataque** | Muscle memory de ARPG/hack'n'slash; botão mais acessível no pad |
| **Space / A = dash** | Verbo de sobrevivência nº1 ([20 §4](../feel/20_Game_Feel.md)); precisa ser o mais confortável |
| **E / B = launcher** | Próximo ao ataque no teclado; no pad, face button secundária (não compete com dash) |
| **R / Y = slam** | Finisher "no topo" do teclado / botão superior no pad — gesto de "fechar" o combo |
| **V / LB = pulo** | Separado do dash; pulo é jump-cancel aéreo, não esquiva ([19](../combat/19_Abilities_Deep.md)) |
| **Shift / RB = sprint** | Hold padrão; configurável pra toggle em [Settings](../ui/27_Settings.md) |
| **Sem Roll no MVP** | Uma esquiva só (dash); Roll cortado ([19](../combat/19_Abilities_Deep.md)) |

> 🎮 **Modelo A (MVP):** sem mira no mouse/stick direito — o personagem **gira pra onde anda**. Stick direito fica livre (ou desligado). **Modelo B** (cursor/twin-stick, P2) reativa `IA_Look` no RS / posição do mouse ([06 §3](06_Camera_TopDown.md)).

---

## 3. Fluxo de combate (o que apertar, em ordem)

```
[Chão]
  LMB LMB LMB        → combo de 3-4 golpes
  (janela) + E       → launcher (ou E direto se aberto)
  Space              → dash-cancel a qualquer momento (escape)

[Ar]
  LMB LMB            → air combo (mesmo botão — GAS escolhe por tag InAir)
  (janela) + V       → jump-cancel (re-sobe, estende juggle)
  R                  → slam (finisher — clímax do pilar)

[Entre salas]
  F (se loot)        → interagir
  (UI eco-select)    → escolher 1 Eco — ver §4
```

> 🔗 **Mesmo botão, abilities diferentes:** `IA_Attack` ativa `GA_Attack_Light` no chão e `GA_AirAttack` no ar — o GAS resolve por `ActivationRequiredTags` ([19 §3](../combat/19_Abilities_Deep.md)). O jogador **não** troca de botão ao subir.

---

## 4. Controles de UI & menus

Válidos em **todas** as telas ([23 §4](../ui/23_UI_Overview.md)). Toda tela navegável **sem mouse**.

### Navegação geral (menus, pause, settings, eco-select)

| Ação | Teclado | Gamepad |
|---|---|---|
| **Confirmar / selecionar** | Enter | **A / ✕** |
| **Voltar / fechar** | Esc / Backspace | **B / ○** |
| **Mover foco** | Setas / Tab | **D-pad** ou **LS** (navegação UMG) |
| **Aba anterior / próxima** | Q / E (settings) | **LB / RB** |
| **Slider − / +** | ← / → | **LT / RT** (ou D-pad) |

### Por tela

| Tela | Controles principais | Doc |
|---|---|---|
| **Main Menu** | Enter = Nova Run / Continuar; Esc = Sair | [24](../ui/24_MainMenu.md) |
| **Pause** | Esc = continuar; navegação = Continuar / Ajustes / Abandonar | [29 §2](../ui/29_Run_Flow_Menus.md) |
| **Seleção de Eco** | ←→ escolhe card; Enter confirma (sem "pular") | [29 §3](../ui/29_Run_Flow_Menus.md) |
| **Settings** | LB/RB troca aba; sliders ao vivo | [27](../ui/27_Settings.md) |
| **Morte / Hub** | Enter = Continuar; compra upgrade no hub | [29 §4-5](../ui/29_Run_Flow_Menus.md) |

> 🎮 **Teste obrigatório:** desconecte o mouse e complete menu → run → pause → eco-select → settings só no pad ([23 §4](../ui/23_UI_Overview.md)).

---

## 5. Tabela técnica — Input Action → binding default

Para implementar no `IMC_Default` ([07 §2](07_Input.md)):

| Input Action | Tipo | Teclas default (KB+M) | Gamepad default | Trigger | Contexto |
|---|---|---|---|---|---|
| `IA_Move` | Axis2D | WASD | LS | Ongoing | Sempre (gameplay) |
| `IA_Look` | Axis2D | — (Modelo A) | — | — | Desligado no MVP |
| `IA_Attack` | Digital | LMB | X / □ | Started | Gameplay |
| `IA_Dash` | Digital | Space | A / ✕ | Started | Gameplay |
| `IA_Jump` | Digital | V | LB / L1 | Started | Gameplay |
| `IA_Sprint` | Digital | LShift | RB / R1 | Started + Completed | Hold (toggle em settings) |
| `IA_Launcher` | Digital | E | B / ○ | Started | Gameplay |
| `IA_AirSlam` | Digital | R | Y / △ | Started | Gameplay |
| `IA_Interact` | Digital | F | RT / R2 | Started | Gameplay + perto de interativo |
| `IA_Pause` | Digital | Esc | Menu / Options | Started | Gameplay |
| `IA_UIConfirm` | Digital | Enter | A / ✕ | Started | UI Only |
| `IA_UICancel` | Digital | Esc | B / ○ | Started | UI Only |
| `IA_UINavigate` | Axis2D | Setas | D-pad / LS | Ongoing | UI Only |

### Input Mapping Contexts (camadas)

```
IMC_Gameplay     → Move, Attack, Dash, Jump, Sprint, Launcher, AirSlam, Interact, Pause
IMC_UI           → Confirm, Cancel, Navigate (+ bloqueia gameplay)
IMC_GameplayOnly → ativo durante combate; removido em menus fullscreen
```

> Ao abrir menu: **remove** `IMC_Gameplay`, **adiciona** `IMC_UI` ([23 §3](../ui/23_UI_Overview.md)). Fechar = inverso.

---

## 6. Símbolos no HUD ([28](../ui/28_HUD.md))

O rodapé-direita mostra ícones com cooldown — use os **mesmos** bindings deste doc:

```
┌─ HABILIDADES ───────┐
│ [Dash●] [LMB] [E◔] │   ← Dash · Ataque · Launcher
│  (Q)     [R]        │   ← Q = vazio MVP · Slam
└─────────────────────┘
```

| Slot HUD | Binding | Estado no MVP |
|---|---|---|
| Dash | Space / A | 🟢 ativo |
| Ataque | LMB / X | 🟢 ativo |
| Launcher | E / B | 🟢 ativo |
| Q | — | 🔵 reservado (P2) |
| Slam | R / Y | 🟢 ativo |

Ícones: use **Enhanced Input** glyph assets ou sprites Xbox/PlayStation conforme o dispositivo detectado ([38 §8](../systems/38_Localization.md) — ícones são culture-invariant).

---

## 7. Rebind & acessibilidade

| Item | Prioridade | Doc |
|---|---|---|
| Sprint hold ↔ toggle | 🟡 P1 | [27](../ui/27_Settings.md) |
| Inverter eixos / deadzone | 🟡 P1 | [27](../ui/27_Settings.md) |
| **Rebind completo** | 🔵 P2 | [27 §7](../ui/27_Settings.md) |
| Vibração on/off | 🟡 P1 | [27](../ui/27_Settings.md) |
| Mostrar prompt de tecla (glyph) | 🟡 P1 | este doc + HUD |

> No MVP os bindings são **fixos** — documente-os em tutorial/loading ([26](../ui/26_Loading_System.md)). Rebind só quando a tela de Settings suportar ([27](../ui/27_Settings.md)).

---

## 8. Prioridade MVP

| Item | Prioridade |
|---|---|
| Bindings de combate (§1) no `IMC_Default` | 🟢 P0 |
| Gamepad equivalente (mesma tabela) | 🟢 P0 |
| UI confirm/cancel/navigate (§4) | 🟢 P0 |
| Troca IMC_Gameplay ↔ IMC_UI em menus | 🟢 P0 |
| HUD mostra símbolos corretos (§6) | 🟢 P0 |
| Sprint toggle em settings | 🟡 P1 |
| Slot Q / ultimate | 🔵 P2 |
| Rebind completo | 🔵 P2 |
| Modelo B (mira RS/mouse) | 🔵 P2 |

---

## 9. Checklist

- [ ] `IMC_Default` com todos os bindings de §5
- [ ] Gamepad testado (combate completo: combo → launch → air → slam)
- [ ] Teclado testado (mesmo fluxo)
- [ ] `IA_Attack` roteia chão/ar via GAS (não `if` no input)
- [ ] Dash = `Started` (não `Triggered` repetido)
- [ ] Sprint = hold + `Completed` ao soltar (ou toggle via settings)
- [ ] Menus navegáveis sem mouse ([23 §4](../ui/23_UI_Overview.md))
- [ ] HUD usa símbolos de §6
- [ ] Pause não deixa personagem agir ([23 §3](../ui/23_UI_Overview.md))

---

## 10. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Ataque não sai no ar | Input chama ability errada | §3 — mesmo `IA_Attack`, gate por `InAir` no GAS |
| Dash dispara em loop | `Triggered` em vez de `Started` | [07 §8](07_Input.md) |
| Sprint trava ligado | Falta bind `Completed` | [07 §8](07_Input.md) |
| Personagem age no menu | `IMC_Gameplay` ainda ativo | §5 — trocar contexto |
| Símbolo errado no HUD | Label hardcoded | §6 — ler binding do Enhanced Input |
| "Pra cima" do pad errado | Move não usa yaw da câmera | [07 §3](07_Input.md) |

---

## 11. Próximo passo

→ Implementar no [07 — Input](07_Input.md) · Exibir no [28 — HUD](../ui/28_HUD.md) · Toggle sprint em [27 — Settings](../ui/27_Settings.md).
