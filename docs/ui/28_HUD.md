# 28 — HUD de Combate + Diagrama de Posição · 🟢 P0

> O HUD é a UI **mais importante do MVP** — é a que o jogador olha *durante o fun*. Cobre o que mostrar, **onde** (diagrama), e como ler do GAS. Pré-req: [23 — UI Overview](23_UI_Overview.md), [05 — GAS](../systems/05_GAS_Architecture.md), [39 — Controles](../systems/39_Controls.md) (símbolos de tecla/botão).

> 🧭 **Regra de ouro topdown:** o personagem fica no **centro** da tela e o **combo aéreo sobe acima dele**. Então o **centro e a faixa central-superior são SAGRADOS** — o HUD vive nas **bordas e cantos**. HUD no meio = tapa o combate = quebra o Pilar 4 (leitura, [01 §2](../design/01_Game_Vision.md)).

---

## 1. Diagrama de posição (layout do HUD)

```
┌──────────────────────────────────────────────────────────────────┐
│ SALA 3/5   🟣 124          ┌── BOSS: Guardião ──┐                  │ ← TOPO
│ ⏱ 04:32                    │ ████████████░░░░░░ │                  │   (run info ·
│                            └────────────────────┘                  │    boss bar)
│                                                                    │
│                                                                    │
│                                                                    │
│                  ▓▓▓  ZONA SAGRADA (NÃO PÔR HUD)  ▓▓▓              │ ← CENTRO
│                  ▓   personagem + combate + combo  ▓              │   (manter
│                  ▓        aéreo sobe AQUI ↑         ▓              │    LIMPO)
│                                                                    │
│                                                                    │
│  ┌─ ECOS ATIVOS ─┐                                  ┌─ COMBO ─┐   │ ← MEIO-BAIXO
│  │ 🔥 💧 ⚡ 🩸     │                                  │  x14    │   │
│  └────────────────┘                                  │ A-RANK! │   │
│                                                       └─────────┘   │
│ ┌─ VITAIS ──────────┐                      ┌─ HABILIDADES ───────┐ │ ← RODAPÉ
│ │ ❤️ ███████████░░░ │                      │ [Dash●] [LMB] [E◔] │ │   (vitais ·
│ │ ⚡ ██████░░        │                      │  Q◑     R(ult)██   │ │    abilities)
│ └───────────────────┘                      └─────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘
```

| Região | Conteúdo | Por quê ali |
|---|---|---|
| **Topo-esquerda** | Sala X/Y · Essência/ouro da run · tempo | Info de contexto, glance ocasional |
| **Topo-centro** | Barra de **boss** (só quando há boss) | Visível mas não no caminho do combate |
| **Centro** | **NADA** (zona sagrada) | É onde o jogador olha e o combo aéreo sobe |
| **Meio-baixo-esquerda** | **ECOS ATIVOS** (ícones da build) | Lembra a build sem roubar o centro |
| **Meio-baixo-direita** | **Contador de combo / rank** | O pilar! Feedback de estilo (ver §3) |
| **Rodapé-esquerda** | **HP + Stamina** | Padrão ARPG; canto = glance rápido |
| **Rodapé-direita** | **Abilities + cooldowns** | Mão direita "vê" os cooldowns ali |

> 🎮 Em **gamepad** o jogador não clica abilities (usa botões), mas os **ícones de cooldown** no rodapé-direita continuam essenciais pra saber *quando* o dash/ultimate voltam.

---

## 2. O que cada elemento mostra (e de onde lê)

| Elemento | Lê de (GAS/sistema) | Prioridade |
|---|---|---|
| **HP** (barra) | `Health`/`MaxHealth` ([05 §3](../systems/05_GAS_Architecture.md)) via delegate | 🟢 P0 |
| **Stamina** (se houver) | `Stamina`/`MaxStamina` | 🟡 P1 (stamina é opcional, [09 §3](../locomotion/09_Gaits.md)) |
| **Cooldowns** (dash, ultimate) | tag `Cooldown.*` + tempo restante do GE | 🟢 P0 |
| **ECOS ATIVOS** | lista de `GE` Infinite de run ([03b](../design/03b_Reward_System.md)) | 🟢 P0 (a build!) |
| **Combo / rank** | contador do combo ([18 §4](../combat/18_Combat_System_Deep.md)) | 🟡 P1 (mas é o pilar — ver §3) |
| **Run info** (sala, Essência) | RunManager ([03](../design/03_Core_Loop_Roguelike.md)) | 🟡 P1 |
| **Boss bar** | Health do boss (só no boss) | 🟡 P1 |
| **HP de inimigo** | barra flutuante sobre o alvo OU nada | 🔵 P2 (em topdown denso polui — ver §4) |
| **Minimapa** | RunManager / layout da sala | 🔵 P2 (arena fechada quase não precisa) |

---

## 3. Contador de combo / rank — o feedback do pilar 🥊

O jogo é hack'n'slash de combo aéreo → o HUD deve **recompensar o estilo** (escola DMC/Bayonetta):

```
┌─ COMBO ─┐
│  x14    │   ← nº de hits sem tomar dano / sem parar
│ A-RANK! │   ← rank por variedade+combo (D→C→B→A→S→SS)
└─────────┘   barra de "deterioração" que cai se você parar
```

- **Contador de hits** sobe a cada acerto encadeado; zera ao tomar hit (conecta com **player flinch**, [18 §5.6](../combat/18_Combat_System_Deep.md)) ou ao parar.
- **Rank de estilo** (P1/P2): premia *variedade* (não repetir o mesmo golpe) + combo aéreo + perfect-dodge. É o que faz o jogador *querer* combar com estilo.
- 🔗 Conecta com [03b](../design/03b_Reward_System.md): um Eco pode dar "+Essência por rank alto" → estilo vira recurso.

> 🎯 No MVP, **um contador de hits simples** (x14) já dá o feedback essencial. O rank S/SS é polish P1-P2, mas é barato e muito alinhado ao pilar.

---

## 4. Leitura em topdown (cuidados específicos)

| Decisão | Recomendação |
|---|---|
| **HP de inimigo** | Em horda densa, barra sobre cada inimigo **polui**. Prefira **flash/cor** (quase morto = pisca) ou só barra no **alvo mais próximo**. Boss = barra dedicada no topo. |
| **Damage numbers** | Só crit/slam ([21 §5](../feel/21_Juice_FX.md)) — não em todo hit, senão vira ruído. |
| **Tamanho/contraste** | HUD legível a -55°/950 de distância: ícones grandes, alto contraste, fundo escuro do jogo ajuda. |
| **Não animar demais** | HUD que pulsa/treme muito compete com o combate pela atenção. Movimento no HUD = só quando muda (dano, cooldown pronto). |

> ⚠️ O **combo aéreo sobe acima do personagem** ([16 §6](../combat/16_Aerial_Combos.md)). Por isso a faixa **central-superior fica livre** — nada de HUD lá, ou tapa o juggle.

---

## 5. Implementação (UMG + GAS)

```
WBP_HUD (camada 1, [23 §2]) — sempre na tela durante o jogo
  • OnInit: inscreve nos delegates:
      ASC->GetGameplayAttributeValueChangeDelegate(Health) → AtualizaBarraHP
      ASC->RegisterGameplayTagEvent(Cooldown.Dash) → liga/desliga ícone
      RunManager->OnEcoAdded / OnRoomChanged → atualiza Ecos/sala
  • SÓ lê e exibe — nunca decide gameplay ([08 §2.1](../locomotion/08_Locomotion_Overview.md))
```

> 🔗 **Single-player:** os delegates disparam local, sem replicação. HP cai → delegate → barra atualiza no mesmo frame. Simples.

---

## 6. MVP vs polish

| Item | Prioridade |
|---|---|
| HP + cooldowns (dash/ultimate) | 🟢 P0 |
| ECOS ATIVOS (ícones) | 🟢 P0 |
| Contador de combo (x N) | 🟡 P1 |
| Run info (sala/Essência) + boss bar | 🟡 P1 |
| Rank de estilo (S/SS) | 🟡/🔵 |
| Minimapa, HP por inimigo, animações de HUD | 🔵 P2 |

---

## 7. Checklist

- [ ] **Centro/central-superior livres** (zona sagrada — combo aéreo)
- [ ] HP (rodapé-esq) + abilities/cooldowns (rodapé-dir)
- [ ] ECOS ATIVOS visíveis (a build)
- [ ] Contador de combo (rodapé-dir/meio)
- [ ] HUD lê GAS por delegate (não faz polling no Tick)
- [ ] Legível a -55°/950 (contraste, tamanho)
- [ ] Damage numbers só crit/slam ([21](../feel/21_Juice_FX.md))
- [ ] Boss bar só quando há boss

---

## 8. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| HUD tapa o combate | Elemento no centro | §1 — só bordas/cantos |
| Combo aéreo "some" atrás do HUD | HUD na faixa central-superior | §1/§4 — manter livre |
| HP não atualiza | Sem inscrição no delegate | §5 |
| HUD "pesado"/baixo FPS | Polling no Tick em vez de delegate | §5 — event-driven |
| Tela poluída na horda | Barra de HP em cada inimigo | §4 — flash/cor ou só no alvo |

---

## 9. Próximo passo

→ [29 — Run-Flow Menus](29_Run_Flow_Menus.md): pause, **Seleção de Eco** e telas de morte/vitória.
