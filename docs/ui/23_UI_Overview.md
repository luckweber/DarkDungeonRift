# 23 — UI: Visão Geral & Arquitetura · 🟢 P0

> Arquitetura da interface do **Dark Dungeon Rift**: stack de widgets, modos de input, navegação (teclado **e gamepad**), e a regra de prioridade MVP. Leia antes dos docs de UI específicos (24-29).

> 🧭 **Princípio nº 1 (do [escopo MVP, 02 §2](../design/02_MVP_Scope.md)):** **UI funcional, não bonita.** No MVP a interface precisa *funcionar e ser legível*, não impressionar. Animação, tema, polish = P2. HUD que comunica e menus que navegam = P0/P1.

> 🎮 **Single-player** ([Index, decisões](../00_Index.md)): sem lobby, sem matchmaking, sem UI de rede. Simplifica tudo.

---

## 1. UMG vs CommonUI — a decisão

| Abordagem | Quando | Veredito DDR |
|---|---|---|
| **UMG puro** | Rápido, simples, controle total | 🟢 **MVP** — funcional já, menos setup |
| **CommonUI** (plugin) | Navegação gamepad robusta, input routing, estilo escalável, stack de camadas pronto | 🟡 **Migração P1** — quando a navegação por pad e o nº de telas crescerem |

**Decisão:** **UMG no MVP** (doc 04 marcou CommonUI como P2). **Mas** projete a navegação por gamepad desde já (ver §4) — ARPG joga muito no controle, e refazer navegação depois é caro. Se o projeto crescer, CommonUI vira a casa natural.

---

## 2. Stack de camadas (Z-order)

A UI vive em **camadas** sobrepostas. Uma camada superior bloqueia input/visão da inferior.

```
┌─ Camada 5: LOADING / TRANSIÇÃO ─┐  ← cobre tudo (doc 26)
├─ Camada 4: MODAL / POPUP ───────┤  ← confirmações ("abandonar run?")
├─ Camada 3: MENU / TELA CHEIA ───┤  ← main menu, settings, pause (docs 24/27/29)
├─ Camada 2: OVERLAY DE RUN ──────┤  ← seleção de boon, death screen (doc 29)
├─ Camada 1: HUD ─────────────────┤  ← combate (doc 28) — sempre embaixo
└─ Camada 0: MUNDO (jogo) ────────┘
```

> 🔑 Regra: quem está em cima **decide o input** (§3). HUD nunca rouba input do jogo; um menu sempre rouba.

---

## 3. Modos de input (crucial)

UE tem 3 modos — escolher errado = "o jogador não consegue clicar" ou "o personagem anda no menu".

| Modo | Quando | Mouse | Jogo recebe input? |
|---|---|---|---|
| **Game Only** | Jogando (HUD na tela) | capturado/escondido | ✅ Sim |
| **UI Only** | Menu de tela cheia (main menu, settings) | livre, visível | ❌ Não |
| **Game and UI** | Overlays leves (raro no MVP) | visível | ✅ Sim |

```
Abrir menu  → SetInputMode(UIOnly) + bShowMouseCursor=true  + (pause se aplicável)
Fechar menu → SetInputMode(GameOnly) + bShowMouseCursor=false + (unpause)
```

> ⚠️ **Erro nº 1 de UI em UE:** esquecer de trocar o input mode → o personagem dasha enquanto você clica em "Ajustes". Centralize isso num **UI Manager** (§5).

---

## 4. Navegação — teclado E gamepad (P0, não P1)

ARPG topdown joga lindo no controle. **Toda tela navegável por gamepad desde o MVP:**

- **Foco:** sempre ter um widget com foco inicial (`SetFocus` / `bIsFocusable`). Sem foco, o pad não navega.
- **D-pad / stick** move o foco; **A/Enter** confirma; **B/Esc** volta/fecha.
- **Não dependa de hover/click do mouse** pra nada essencial — tudo precisa ter caminho por foco.
- UMG: use `Navigation` (Up/Down/Left/Right) nos botões; ou CommonUI (resolve automático).

> 🎮 **Teste do controle:** desconecte o mouse e navegue o jogo inteiro (menu→run→pause→settings→sair) só no pad. Se travar em algum lugar, a navegação está incompleta.

---

## 5. UI Manager (centralize push/pop)

Crie um **`UDDRUIManager`** (UGameInstanceSubsystem ou no PlayerController) que centraliza:

```cpp
UDDRUIManager:
  PushScreen(WidgetClass, Layer)   // cria, adiciona à viewport, ajusta input mode/foco
  PopScreen()                      // remove topo, restaura input mode da camada de baixo
  ShowHUD() / HideHUD()
  ShowLoading() / HideLoading()
  (mantém uma pilha por camada)
```

Benefício: **um lugar só** decide input mode, pause, foco e Z-order. Sem isso, cada widget gerencia o seu e os bugs se multiplicam.

---

## 6. Como a UI lê o jogo (binding)

| Fonte | UI lê via | Exemplo |
|---|---|---|
| **GAS Attributes** ([05](../systems/05_GAS_Architecture.md)) | delegate `GetGameplayAttributeValueChangeDelegate` | HP/Stamina no HUD (doc 28) |
| **GAS Tags** | `RegisterGameplayTagEvent` | cooldown ativo, `State.Dead` |
| **Run state** ([03](../design/03_Core_Loop_Roguelike.md)) | RunManager (delegates) | sala X/Y, Essência, boons ativos |
| **Settings** | GameUserSettings / SaveGame ([27](27_Settings.md)) | volume, qualidade |

> 🔗 **Single-player simplifica:** sem replicação, os delegates do GAS disparam local e o HUD atualiza direto. Sem `RepNotify`, sem predição de UI.

---

## 7. Prioridade MVP (o que construir, em ordem)

| Tela | Prioridade | Doc |
|---|---|---|
| **HUD** (HP, abilities, boons) | 🟢 P0 | [28](28_HUD.md) |
| **Seleção de Boon** (entre salas) | 🟢 P0 | [29](29_Run_Flow_Menus.md) |
| **Main Menu** (funcional) | 🟡 P1 | [24](24_MainMenu.md) |
| **Pause + Death screen** | 🟡 P1 | [29](29_Run_Flow_Menus.md) |
| **Save/Load** (só meta) | 🟡 P1 | [25](25_Save_Load_Slots.md) |
| **Loading** (simples) | 🟡 P1 | [26](26_Loading_System.md) |
| **Settings** (mínimo) | 🟡 P1 | [27](27_Settings.md) |
| Tema/polish/animação | 🔵 P2 | — |

> 🎯 **A UI mais importante do MVP não é um menu — é o HUD + a seleção de boon.** São as que o jogador usa *durante* o fun. Menus de borda (main/settings) podem ser feios e funcionais.

---

## 8. Checklist

- [ ] UMG no MVP; navegação gamepad projetada desde já
- [ ] Stack de camadas definido (HUD < overlay < menu < modal < loading)
- [ ] `UDDRUIManager` centraliza push/pop + input mode + foco
- [ ] Toda tela tem foco inicial e caminho por gamepad
- [ ] HUD lê GAS por delegate; run state por RunManager
- [ ] Input mode trocado corretamente ao abrir/fechar menu
- [ ] Teste do controle (jogo inteiro sem mouse) passa

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Personagem age enquanto navego o menu | Input mode não trocado | §3 — UIOnly ao abrir |
| Gamepad não navega o menu | Sem foco inicial / sem Navigation | §4 — SetFocus + Navigation |
| Menu não recebe clique | Input GameOnly num menu | §3 |
| HUD não atualiza HP | Não inscrito no delegate do atributo | §6 |
| Telas "empilham" erradas | Z-order/camada manual | §5 — UI Manager |

---

## 10. Próximo passo

→ [24 — Main Menu](24_MainMenu.md) · [28 — HUD](28_HUD.md) (a mais importante) · [29 — Run-Flow Menus](29_Run_Flow_Menus.md) (seleção de boon).
