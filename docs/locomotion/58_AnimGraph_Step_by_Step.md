# 58 — AnimGraph de Locomoção: Passo a Passo no Editor · 🟢 P0 (M0/M1)

> **Como montar o `ABP_DDRPlayer` no editor**, clique a clique: variáveis lidas do CMC real, Blend Space, State Machine (Idle/Move/Jump/Fall/Land), e o **Slot que faz o combo do M1 funcionar**. Implementa a arquitetura do [08](08_Locomotion_Overview.md) com o código que **já existe** (`FDDRLocomotionState` via `GetLocomotionState()`).

> **Pré-req:** M0 compilado ([54](../systems/54_M0_Editor_Setup.md)) · `ABP_DDRPlayer` criado (parent `AnimInstance`, [54 §3.3](../systems/54_M0_Editor_Setup.md)) · clips do set ([22](../22_Animation_Inventory.md)): Idle, Run 8-way (ou só forward), Jump.

> 🧭 **Filosofia ([08 §2.1](08_Locomotion_Overview.md)):** o ABP **só lê e veste**. Toda decisão (gait, dash, falling) já vem pronta do CMC no `FDDRLocomotionState`. Se você se pegar calculando gameplay no ABP, pare.

---

## 0. O que vamos montar

```
[Event Graph]  lê CMC->GetLocomotionState() → vars do ABP (Speed, Direction, Gait, bIsFalling, bIsMoving, bIsDashing)
       ↓ alimenta
[AnimGraph]
   [State Machine "Locomotion"]
      Idle ⇄ Move(BS_Locomotion) → JumpStart → Fall → Land → (volta)
       ↓
   [Slot 'DefaultGroup.DefaultSlot']   ← SEM ISTO O COMBO DO M1 NÃO TOCA
       ↓
   [Output Pose]
```

O que fica de **fora** (de propósito): start/stop transitions e distance match ([10](10_Start_Stop_DistanceMatch.md), P1 — os ganchos ficam prontos, §6), turn-in-place (removido), foot IK ([14](14_Foot_Leg_IK.md), P2).

---

## 1. Variáveis + Event Graph (ler o snapshot do CMC)

### 1.1 Crie as variáveis no ABP

| Variável | Tipo | Vem de |
|---|---|---|
| `Speed` | Float | `LocomotionState.Speed` |
| `Direction` | Float | `LocomotionState.Direction` (-180..180) |
| `Gait` | `EDDRGait` | `LocomotionState.Gait` |
| `bIsMoving` | Bool | `LocomotionState.bIsMoving` (Speed > 10) |
| `bIsFalling` | Bool | `LocomotionState.bIsFalling` |
| `bIsDashing` | Bool | `LocomotionState.bIsDashing` |

### 1.2 Event Graph (MVP — game thread, ok p/ 1 player)

```
Event Blueprint Initialize Animation:
  Try Get Pawn Owner → Cast to DDRPlayerCharacter → promova a var "Player" (cache do cast)

Event Blueprint Update Animation:
  Player (válido?) → Get DDR Movement → Get Locomotion State
    → Break DDRLocomotionState
    → SET Speed / Direction / Gait / bIsMoving / bIsFalling / bIsDashing
```

> ⚠️ **Cast UMA vez** (no Initialize), não a cada frame. O `GetLocomotionState` é `BlueprintPure` — o nó aparece direto no BP.

> 🚀 **Upgrade P1 (quando houver hordas):** migrar a leitura pra **Property Access** / funções `BlueprintThreadSafe` (ou um `UDDRAnimInstance` C++ com `NativeThreadSafeUpdateAnimation`, [08 §2.4](08_Locomotion_Overview.md)). No MVP com 1 player, o Event Update resolve.

---

## 2. Blend Space — `BS_Locomotion` (Direction × Speed)

1. Content Browser → `Content/DarkDungeonRift/Animation/` → **Animation → Blend Space** (2D), skeleton do player → nome **`BS_Locomotion`**.
2. **Axis Settings:**

| Eixo | Nome | Min | Max | Grid |
|---|---|---|---|---|
| **Horizontal** | `Direction` | **-180** | **180** | 8 divisões (casa com 8-way) |
| **Vertical** | `Speed` | **0** | **750** | 3 divisões |

3. **Arraste os clips** pro grid:

| Speed \ Direction | -180 | -90 | 0 (fwd) | +90 | +180 |
|---|---|---|---|---|---|
| **0** | Idle | Idle | **Idle** | Idle | Idle |
| **500** | Run_Bwd | Run_Left | **Run_Fwd** | Run_Right | Run_Bwd |
| **750** | — | — | **Sprint** (clip "2 Run speed") | — | — |

> 🎯 **MVP mínimo:** se quiser ir mais rápido ainda, comece com **1D (só Speed)**: Idle(0) → Run_Fwd(500) → Sprint(750). Com **orient-to-movement** o personagem já gira pra direção do movimento — o eixo Direction só refina diagonais/transientes. O 8-way completo do seu set entra depois sem mudar o graph (mesmo Blend Space, mais samples).

4. **Weight Speed/Smoothing** (Axis): ~**4-6** de interpolação por eixo — suaviza trocas (e dá o "Gait Switch Transition de graça", [09 §6](09_Gaits.md)).

---

## 3. AnimGraph — State Machine "Locomotion"

No **AnimGraph**: clique direito → **State Machine** → nome `Locomotion` → ligue (por enquanto) direto no Output. Dentro dela:

### 3.1 Estados

| Estado | Conteúdo (dentro do estado) |
|---|---|
| **Idle** | clip `Idle` (loop) — *use o Idle normal; o combat-idle entra via tag depois ([08 §6](08_Locomotion_Overview.md))* |
| **Move** | **`BS_Locomotion`** com pinos: `Direction` ← var Direction, `Speed` ← var Speed |
| **JumpStart** | clip de impulso do Jump (não-loop) |
| **Fall** | clip/pose de queda (loop) — do seu "Jump 4-way", o trecho aéreo |
| **Land** | clip de pouso (não-loop) |

### 3.2 Transições (a tabela que importa)

| De → Para | Regra (condição exata) | Notas |
|---|---|---|
| **Idle → Move** | `bIsMoving` | responde JÁ ([20 §2](../feel/20_Game_Feel.md)) |
| **Move → Idle** | `NOT bIsMoving` | braking 2000 do CMC faz o resto |
| **Idle → JumpStart** e **Move → JumpStart** | `bIsFalling` | pulo OU queda de borda |
| **JumpStart → Fall** | **Automatic Rule** ✅ (Time Remaining ratio < 0.1) | o impulso termina → cai |
| **Fall → Land** | `NOT bIsFalling` | tocou o chão |
| **Land → Idle** | **Automatic Rule** ✅ | pouso curto termina |
| **Land → Move** | `bIsMoving` (prioridade > Land→Idle) | pousou correndo → não trava no Land |

> ⏱️ **Durations das transições:** 0.1-0.2s nas de chão; **JumpStart/Fall/Land curtas (0.05-0.1s)** — ar precisa responder. **Land → Move com prioridade** (Transition Priority menor = avaliada antes): pousar em movimento não pode prender o jogador na anim de pouso ([20 §4.2](../feel/20_Game_Feel.md)).

### 3.3 (Opcional M0+) Dash pose

Depois da State Machine, antes do Slot: **Blend Poses by Bool** (`bIsDashing`) → True = clip de Dodge (1 pose/clip do seu set); False = resultado da SM. *Sem isso o dash desliza no Run — funcional, feio, aceitável no M0.*

---

## 4. 🔑 Slot 'DefaultSlot' (o passo que o M1 EXIGE)

Sem este nó, **as montages do combo (`AM_Combo`) não tocam no jogo** — esse é o bug clássico "funciona no preview da montage, não no PIE".

1. Após a State Machine (e o blend do dash, se fez): clique direito → **Slot 'DefaultSlot'**.
2. Confirme o slot: **`DefaultGroup.DefaultSlot`** (o mesmo da `AM_Combo`, [57](../combat/57_M1_Combo_Editor_Setup.md)).
3. Ligue: `State Machine → (Blend dash) → Slot → Output Pose`.

> A ordem importa: **locomoção embaixo, combate por cima** ([08 §4](08_Locomotion_Overview.md)). A montage sobrescreve a pose quando toca; ao acabar, blenda de volta pro resultado da SM.

---

## 5. Ligar no player

`BP_DDRPlayer` → componente **Mesh** → **Anim Class = `ABP_DDRPlayer`**. (Se já estava, só recompile o ABP.)

---

## 6. Ganchos futuros (deixados prontos — NÃO implemente agora)

| Futuro | Onde encaixa neste graph | Doc |
|---|---|---|
| **Start/Stop + Distance Match (P1)** | viram **estados** entre Idle⇄Move: `Idle→Start→Move→Stop→Idle`. O gatilho do Stop **já existe**: `bWantsToStop` no snapshot do CMC | [10](10_Start_Stop_DistanceMatch.md) |
| **Air SM de combate (M2)** | gate por tag `State.Combat.InAir` (lida do ASC) — SM separada da queda normal | [16 §3.1](../combat/16_Aerial_Combos.md), [08 §6](08_Locomotion_Overview.md) |
| **Locomoção por arma** | este graph vira a **base lógica**; as poses migram pra **Linked Anim Layers** (`ALI_DDRLocomotion`) | [50 §2](../combat/50_Weapons_Arsenal.md) |
| **Idle de combate** | Blend by bool (tag `State.Combat.Attacking`/timer de combate) trocando Idle normal ↔ combat | [22](../22_Animation_Inventory.md) tem os 2 |

---

## 7. Teste (PIE)

| Teste | Esperado |
|---|---|
| Parado | Idle (sem tremer entre estados) |
| WASD | Move — blendspace interpola; diagonais ok |
| Shift | clip de sprint a 750 (eixo Speed alcança) |
| Soltar o stick | volta pra Idle rápido (braking 2000 + `NOT bIsMoving`) |
| Pulo | JumpStart → Fall → Land → volta certo (correndo → Move direto) |
| **Atacar (LMB)** | **`AM_Combo` toca por cima** (Slot ok) e volta pra locomoção no fim |
| Dash | pose de dodge (se §3.3) ou desliza (aceitável M0) |
| `ddr.LocomotionDebug 1` | Gait/Speed do HUD batem com a anim que toca |

---

## 8. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| **Montage não toca no PIE** (só no preview) | **sem nó Slot 'DefaultSlot'** no AnimGraph | §4 — o passo crítico |
| T-pose | Anim Class não setada / skeleton errado | §5; BS e clips no MESMO skeleton |
| Personagem desliza parado (toca Run) | transição usa Speed cru com threshold 0 | use `bIsMoving` (já tem histerese de 10 no CMC) |
| Blendspace não interpola | nomes/ranges dos eixos não batem com os pinos | §2 — `Direction` -180..180, `Speed` 0..750 |
| Direção invertida (esq/dir trocados) | sinal do `Direction` | confira o sample -90/+90 no grid (o CMC usa cross(forward,vel).Z) |
| Preso na anim de Land ao pousar correndo | falta `Land → Move` com prioridade | §3.2 |
| Anim "pisca" entre Idle e Move | durations 0 + histerese insuficiente | duration 0.15s; use `bIsMoving`, não Speed>0 |
| Pés deslizam no run | **esperado no MVP** (sem distance match/stride warp) | P1 — [10](10_Start_Stop_DistanceMatch.md)/[11](11_Warping.md); topdown esconde |
| Cast falha no Event Graph | preview/dummy usa o ABP | `Is Valid` antes; cast só no Initialize |

---

## 9. Próximo passo

→ Com o graph pronto e o Slot funcionando, o **M1 fecha o ciclo**: [57 — Combo no Editor](../combat/57_M1_Combo_Editor_Setup.md) · [55 — M1 Setup](../systems/55_M1_Editor_Setup.md). Polish de locomoção (start/stop): [10](10_Start_Stop_DistanceMatch.md) **depois** do combate gostoso.
