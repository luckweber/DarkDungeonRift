# 24 — Main Menu · 🟡 P1

> A tela de entrada do jogo. Funcional e legível — **não precisa ser bonita no MVP** ([02 §2](../design/02_MVP_Scope.md)). Pré-req: [23 — UI Overview](23_UI_Overview.md).

> 🧭 **Main Menu ≠ Hub.** O **Main Menu** é a tela de boot (fora da run). O **Hub** é o espaço *dentro do jogo*, entre runs, onde se gasta Essência (meta-progressão, [03 §5](../design/03_Core_Loop_Roguelike.md)). Este doc é o Main Menu; o Hub está em [29 §5](29_Run_Flow_Menus.md).

---

## 1. Layout (diagrama)

```
┌──────────────────────────────────────────────┐
│                                              │
│            DARK  DUNGEON  RIFT               │  ← título/logo (placeholder no MVP)
│                                              │
│                                              │
│            ▶  NOVA RUN                        │  ← foco inicial
│               CONTINUAR        (se há save)  │
│               AJUSTES                         │
│               SAIR                            │
│                                              │
│                                              │
│  v0.1.0                          [⚙ idioma]  │  ← rodapé: versão, atalho
└──────────────────────────────────────────────┘
```

> Fundo: imagem estática escura + leve parallax/partícula (P2). No MVP, cor sólida + título em texto já serve.

---

## 2. Botões & ações

| Botão | Ação | Condição |
|---|---|---|
| **Nova Run** | Começa uma run nova → loading → gameplay | sempre |
| **Continuar** | Carrega o save (meta) e vai pro Hub/run | só se existe save ([25](25_Save_Load_Slots.md)) |
| **Ajustes** | Abre [Settings (27)](27_Settings.md) (camada menu) | sempre |
| **Sair** | Confirma (modal) → fecha o jogo | sempre |

> 🎮 **Foco inicial** em "Nova Run" (ou "Continuar" se houver save). Navegável 100% por gamepad ([23 §4](23_UI_Overview.md)).

---

## 3. Fluxo

```
[Main Menu]
   ├─ Nova Run ──→ (seleção de personagem? — P2) ──→ [Loading (26)] ──→ [Gameplay/Hub]
   ├─ Continuar ─→ carrega save (25) ──────────────→ [Hub (29 §5)]
   ├─ Ajustes ───→ [Settings (27)] ──(voltar)──→ [Main Menu]
   └─ Sair ──────→ [Modal confirmar] ──→ Quit
```

- **Seleção de personagem/classe:** o MVP tem **1 player** ([02 §2](../design/02_MVP_Scope.md)) → **pule**. Reserve o gancho pra pós-MVP (múltiplas classes estilo Hades/Death Must Die).
- **Nova Run com save existente:** confirmar se sobrescreve a run em andamento (se houver run-resume, [25 §4](25_Save_Load_Slots.md)).

---

## 4. Implementação (UMG)

```
WBP_MainMenu (UserWidget) — Camada 3 (menu)
  • carregado no nível de menu (mapa "L_MainMenu" leve) ou como widget sobre um nível vazio
  • OnConstruct: SetInputMode(UIOnly) + foco no 1º botão + checa save → habilita "Continuar"
  • cada botão → função no UDDRUIManager (23 §5)
```

> 💡 **Mapa de menu dedicado** (`L_MainMenu`): um nível minúsculo só com câmera + luz, pra carregar rápido. "Nova Run" faz `OpenLevel` (via loading, doc 26) pro nível de jogo/hub.

---

## 5. MVP vs polish

| Item | Prioridade |
|---|---|
| 4 botões funcionais + navegação pad | 🟢 P0 (se for fazer o menu) |
| Checa save → "Continuar" condicional | 🟡 P1 |
| Modal de confirmação de "Sair" | 🟡 P1 |
| Logo/arte, fundo animado, transições, SFX de UI | 🔵 P2 |
| Seleção de personagem/classe | 🔵 P2 (1 player no MVP) |

> 🎯 No MVP-cedo, dá até pra **pular o main menu** e cair direto na run (boot → gameplay) pra testar o combate. O menu entra no M4-M5 quando o loop fecha.

---

## 6. Checklist

- [ ] `WBP_MainMenu` com Nova Run / Continuar / Ajustes / Sair
- [ ] Input mode UIOnly + foco inicial + navegação gamepad
- [ ] "Continuar" só habilita se há save ([25](25_Save_Load_Slots.md))
- [ ] "Nova Run" → loading ([26](26_Loading_System.md)) → jogo
- [ ] "Sair" com modal de confirmação
- [ ] (opcional) mapa `L_MainMenu` leve

---

## 7. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| "Continuar" sempre ativo/inativo | Não checa `DoesSaveGameExist` | [25 §3](25_Save_Load_Slots.md) |
| Gamepad não navega | Sem foco/Navigation | [23 §4](23_UI_Overview.md) |
| Trava ao iniciar run | OpenLevel síncrono sem loading | [26](26_Loading_System.md) |

---

## 8. Próximo passo

→ [25 — Save/Load & Slots](25_Save_Load_Slots.md) (o que "Continuar" carrega) · [27 — Settings](27_Settings.md).
