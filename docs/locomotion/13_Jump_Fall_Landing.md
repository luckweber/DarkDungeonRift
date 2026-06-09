# 13 — Jumping / Fall / Landing · 🟢 P0 (jump/fall) · 🟡 P1 (landing)

> Cobre **Jumping** (#4), **Gait-Selected Jumping** (#5) e **Height-Based Fall to Landing** (#6). O sistema vertical — pulo, queda e pouso.

> 🧭 **Por que P0 aqui (diferente do resto da locomoção):** o combo aéreo é o **pilar do jogo** (doc 01). A mecânica vertical — controlar altura, tempo de ar, pouso — é a **fundação técnica** do [doc 16 (combos aéreos)](../combat/16_Aerial_Combos.md). Investir aqui é investir no pilar.

---

## 1. Dois tipos de "ar" no jogo (não confundir)

| Tipo | Origem | Quem controla | Doc |
|---|---|---|---|
| **Pulo de traversal** | Jogador aperta Jump | CMC (física de pulo) | **este doc** |
| **Juggle / lançamento** | Launcher de combate | GAS + RootMotion/Launch | [16](../combat/16_Aerial_Combos.md) (usa a base daqui) |

Este doc cobre a **base física e de animação do ar**, que ambos usam. O combate aéreo (juggle) é construído **em cima** disto.

---

## 2. Pulo no CMC — parâmetros

```cpp
// UDDRCharacterMovementComponent / Character
JumpZVelocity = 600.f;        // força inicial do pulo
GravityScale  = 1.75f;        // gravidade "de jogo" (>1 = cai mais rápido, sente melhor)
AirControl    = 0.4f;         // quanto dá pra dirigir no ar (0-1)
// queda mais pesada que subida (game feel clássico):
// aplique GravityScale maior na descida (ex.: ×1.25) ou via custom gravity
```

| Parâmetro | Valor inicial | Efeito |
|---|---|---|
| `JumpZVelocity` | 600-750 | Altura do pulo |
| `GravityScale` | 1.5-2.0 | Peso da queda. **>1 sente muito melhor** que gravidade real |
| `AirControl` | 0.3-0.5 | Manobra no ar |
| Gravidade na descida | ×1.2-1.5 | Apex flutuante + queda rápida = "feel AAA" |

---

## 3. Game feel do pulo (barato, alto retorno)

Mesmo em topdown, um pulo "bom" vem destes truques (todos no CMC):

| Técnica | O que faz | Prioridade |
|---|---|---|
| **Coyote time** | Aceita pulo até ~0.1s depois de sair da borda | 🟡 P1 |
| **Jump buffer** | Aceita pulo apertado ~0.1s antes de pousar | 🟡 P1 |
| **Apex hang** | Gravidade reduzida no topo do arco | 🟡 P1 |
| **Variable jump height** | Soltar cedo = pulo mais baixo (apex-cut) | 🟡 P1 |
| **Gravidade assimétrica** | Sobe leve, cai pesado | 🟢 P0 (1 linha, grande efeito) |

> 🪜 Estes mesmos truques são o que faz o **juggle aéreo** sentir bom depois. Construa-os no CMC agora.

---

## 4. State machine de ar (AnimGraph)

```
[State Machine: Air]   ← entra quando bIsFalling
   JumpStart ──▶ Loop(Rising) ──(vZ < 0)──▶ Apex/Fall(Loop) ──(prox. do chão)──▶ Land
                                                                                   │
   ◀────────────────────────── (aterrissou: !bIsFalling) ──────────────────────────┘
```

| Estado | Quando | Anim |
|---|---|---|
| **JumpStart** | acabou de sair do chão (vZ > 0) | impulso de pulo |
| **Loop Rising** | subindo | pose de subida |
| **Fall Loop** | vZ < 0 (caindo) | pose de queda |
| **Land** | tocou o chão | pouso (ver §6 — height-based) |

**Inputs do struct de estado (doc 08):** `bIsFalling`, `Velocity.Z`, `TimeInAir`, `PredictedLandingDistance`.

---

## 5. Gait-Selected Jumping (#5 · 🔵 P2)

A ideia: a anim de pulo muda conforme o gait de onde você pulou (pular parado ≠ pular correndo ≠ sprint jump).

**Realidade topdown:** variar a *animação* do pulo por gait é **quase invisível de cima** → P2. **MAS** a *física* já varia naturalmente: pular em sprint carrega o momentum horizontal (CMC preserva velocidade), então o pulo já "vai mais longe" sem trabalho de anim.

**Recomendação:**
- **Física:** o momentum de sprint já se propaga ao pulo de graça. ✅ (isso é o que importa pro gameplay)
- **Animação por gait:** pule (P2). Uma anim de pulo genérica serve no MVP.

> Ou seja: você ganha o **efeito jogável** do "gait-selected jump" (pulo de corrida vai mais longe) sem a animação dedicada. Bom o suficiente.

---

## 6. Height-Based Fall to Landing (#6 · 🟡 P1)

O pouso muda conforme **de que altura/quão rápido** você caiu: queda curta = pouso leve; queda alta = pouso pesado (agacha, poeira, shake).

**Por que é P1 (e não P2) aqui:** o **slam aéreo** (finisher do combo, doc 16) precisa de um pouso **com peso** pra leitura — o impacto comunica o fim do combo. Então o "hard landing" serve diretamente ao pilar.

### Como decidir o tipo de pouso

```
TimeInAir (ou velocidade Z no impacto, ou distância de queda prevista)
  < 0.4s  → Soft land  (sem interrupção, segue correndo)
  0.4-1s  → Medium land (leve agacha)
  > 1s    → Hard land  (agacha forte + poeira + screen shake + breve recovery)
  Slam    → sempre Hard land + dano em área (doc 16)
```

| Sinal | Fonte | Uso |
|---|---|---|
| `TimeInAir` | acumulado enquanto `bIsFalling` | classifica o pouso |
| `Velocity.Z` no toque | CMC no `OnLanded`/`Landed` | alternativa mais física |
| `PredictedLandingDistance` | line trace pra baixo enquanto cai | antecipa anim de pouso (pré-land) |

### Predictive landing (polish P1)

Enquanto cai, faça um trace pra baixo → sabe a distância até o chão → **antecipa** a transição pra anim de "preparar pouso" antes de tocar. Suaviza o pouso (sem "pop" no contato).

### No GAS

Hard land dispara `GameplayCue.Land.Hard` (poeira + shake + som). O slam (doc 16) reusa isso. Conecta locomoção ↔ feedback via [GAS](../systems/05_GAS_Architecture.md).

---

## 7. Anti-slide no pouso

Ao pousar correndo, o personagem não pode "patinar". Retenha um pouco da velocidade horizontal e aplique braking forte no toque:

```cpp
// no Landed():
// reter parte do momentum (anti-stop seco) mas frear o slide:
BrakingDecelerationWalking aplicado; opcional "landing retain" (0.4) por X frames
```

> O DungeonForged usa "retain 0.4 + landing brake 4096" pra isso. Tune pro seu feel.

---

## 8. Checklist

- [ ] CMC: `JumpZVelocity`, `GravityScale`>1, `AirControl` tunados
- [ ] Gravidade assimétrica (cai mais pesado) — P0
- [ ] Coyote/buffer/apex-hang/variable-height — P1
- [ ] State machine de Air (JumpStart→Rising→Fall→Land)
- [ ] Momentum de sprint se propaga ao pulo (gait-jump "de graça")
- [ ] Anim de pulo única no MVP (gait-selected = P2)
- [ ] **Height-based landing** (soft/medium/hard por TimeInAir) — P1
- [ ] Hard land → `GameplayCue.Land.Hard` (shake/poeira)
- [ ] Predictive landing (trace) — P1 polish
- [ ] Anti-slide no pouso

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Pulo "flutua"/lento | `GravityScale` baixo | Suba p/ 1.5-2.0 |
| Pulo sente "leve demais" no topo | Sem apex hang nem gravidade assimétrica | §3 |
| Pousa e desliza | Sem braking/retain no Landed | §7 |
| Pouso sempre igual | Não classifica por TimeInAir | §6 |
| "Pop" ao tocar o chão | Sem predictive landing | §6 trace + pré-land |
| Pulo em sprint não vai mais longe | CMC zerando velocidade no pulo | Preserve momentum horizontal |

---

## 10. Próximo passo

→ [14 — Foot & Leg IK](14_Foot_Leg_IK.md) (P2), ou pule direto pro combate: [15 — Combat Overview](../combat/15_Combat_Overview.md).
