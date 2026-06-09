# 08 — Visão Geral da Locomoção · 🟢 P0

> Arquitetura do sistema de movimento/animação e o mapa dos **11 recursos** (Turn In Place foi removido — §1). Cobre o pilar **"Clean Extendable Logic"** e a decisão **bespoke vs Motion Matching**. Leia este antes dos outros docs de locomoção (09-11, 13-14).

> 🧭 **Verdade topdown (repetida porque importa):** a câmera está longe e de cima. Metade dos 11 recursos tem **payoff visual baixo** aqui. Este doc te diz onde investir e onde *não* investir no MVP.

---

## 1. Os 11 recursos, priorizados para topdown

> ✂️ **Turn In Place foi REMOVIDO do escopo** (decisão do dev). Em topdown + orient-to-movement o personagem gira andando, nunca parado — TIP tinha payoff ~nulo. Confirmado pelos assets: a lista de animação ([22 — Inventário](../22_Animation_Inventory.md)) **não tem nenhum clip de turn-in-place**.

| # | Recurso | Doc | Prioridade | Payoff topdown |
|---|---|---|:---:|---|
| 1 | **Multiple Gaits** (walk/run/sprint) | [09](09_Gaits.md) | 🟢 P0 | Alto — velocidade é leitura de gameplay |
| 2 | **Sprinting & Crouching** | [09](09_Gaits.md) | 🟢/🔵 | Sprint alto; **crouch P2** (pouco uso em ARPG) |
| 3 | **Clean Extendable Logic** | este doc | 🟢 P0 | Arquitetura — sempre vale |
| 4 | **Jumping** | [13](13_Jump_Fall_Landing.md) | 🟢 P0 | Alto — **base do combo aéreo** |
| 5 | **Gait-Selected Jumping** | [13](13_Jump_Fall_Landing.md) | 🔵 P2 | Baixo — variar anim de pulo por gait quase não aparece |
| 6 | **Height-Based Fall to Landing** | [13](13_Jump_Fall_Landing.md) | 🟡 P1 | Médio — leitura de pouso do slam aéreo |
| 7 | **Start & Stop Transitions** | [10](10_Start_Stop_DistanceMatch.md) | 🟡 P1 | Médio — responsividade > pose perfeita |
| 8 | **Distance Match Stops** | [10](10_Start_Stop_DistanceMatch.md) | 🟡 P1 | Baixo-médio — foot-sliding some na distância |
| 9 | **Gait Switch Transition States** | [09](09_Gaits.md) | 🔵 P2 | Baixo — transição walk↔run quase invisível |
| 10 | **Stride Warping** | [11](11_Warping.md) | 🟡/🔵 | Baixo — anti-skating que a câmera esconde |
| 11 | **Foot & Leg IK** | [14](14_Foot_Leg_IK.md) | 🔵 P2 | Baixo — só importa em rampas/escadas |
| — | ~~Turn In Place~~ | *removido* | ❌ cortado | Topdown + orient-to-movement elimina; sem assets |

**Tradução prática:** pro MVP, faça bem **Gaits (sprint)** e **Jump/Fall/Landing**. O resto (start/stop, distance match, warping, foot IK) é **polish incremental** que você liga *depois* do combate fechado. Não inverta essa ordem.

---

## 2. "Clean Extendable Logic" — a arquitetura

O recurso #3 é o mais importante porque sustenta todos os outros. Princípios:

### 2.1 Separação de responsabilidades

```
┌─ GAMEPLAY (autoridade) ────────────────────────────────┐
│  UDDRCharacterMovementComponent                         │
│   • velocidade, gaits, sprint, dash, pulo, gravidade    │
│   • É a VERDADE. Decide o que o personagem faz.         │
└──────────────────────────┬──────────────────────────────┘
                           │ (lê estado, NÃO comanda)
┌─ ANIMAÇÃO (cosmético) ───┴──────────────────────────────┐
│  UDDRAnimInstance (+ AnimGraph)                         │
│   • lê Velocity/Accel/IsFalling/Gait/Tags               │
│   • escolhe poses. NÃO decide gameplay.                 │
└─────────────────────────────────────────────────────────┘
```

> 🔑 **Regra de ouro:** *gameplay decide, animação reage.* O CMC nunca pergunta "que animação tá tocando?". O AnimBP nunca move o personagem (exceto root motion controlado). Quebrar isso = bugs de sincronização infinitos. (Lição transferida do DungeonForged.)

### 2.2 Estado em um lugar só

O `AnimInstance` calcula **uma vez por frame** (em `NativeThreadSafeUpdateAnimation`) um struct de estado e o AnimGraph só **lê**:

```cpp
// DDRLocomotionTypes.h — o "contrato" de estado
USTRUCT(BlueprintType)
struct FDDRLocomotionState
{
    float Speed;            // tamanho da velocidade horizontal
    FVector Velocity;
    FVector Acceleration;
    float Direction;        // -180..180 relativo ao forward (p/ 8-way / blendspace)
    bool bIsMoving;
    bool bIsFalling;
    EDDRGait Gait;          // Walk / Run / Sprint
    bool bWantsToStop;      // accel ~0 mas velocidade > 0 → distance match stop
};
```

### 2.3 Enums data-driven, não booleans soltos

```cpp
UENUM(BlueprintType)
enum class EDDRGait : uint8 { Walk, Run, Sprint };

UENUM(BlueprintType)
enum class EDDRMovementMode : uint8 { Grounded, InAir, Combat };
```

> Adicionar um gait novo (ex.: "Exhausted") = 1 enum + 1 entrada de dados, **não** reescrever o AnimGraph. Isso é o "extendable".

### 2.4 Thread-safe update

Use `NativeThreadSafeUpdateAnimation` (roda no worker thread) para o cálculo de estado. Mantém o frame barato com muitos inimigos (importante num jogo de hordas).

---

## 3. Bespoke vs Motion Matching — a decisão

Os 11 recursos podem ser implementados de **dois jeitos**:

| Abordagem | Como | Prós | Contras |
|---|---|---|---|
| **A — Bespoke (state machine + distance match + warping)** | AnimBP com state machines, blendspaces 8-way, curvas de distance matching, pose warping | Domínio total, poucos plugins, você **já conhece** (DungeonForged) | Mais clips à mão; escala mal p/ N armas |
| **B — Motion Matching (GASP / PoseSearch)** | Pose Search Database escolhe a pose por trajetória | Feel AAA "de fábrica", menos lógica à mão, transições emergem | Curva de aprendizado, precisa set de anims marcado, plugin `PoseSearch` |

**Decisão MVP: Abordagem A (bespoke).** Motivos:
1. Você já domina o padrão (transferível do DungeonForged).
2. Em **topdown**, o ganho de fidelidade do MM é justamente o que a câmera esconde.
3. MVP = combate. Não gastar a curva de MM agora.

> 🧪 **Spike opcional (P2):** depois do MVP, habilite `PoseSearch` e teste MM **num ABP separado**, sem quebrar o atual. O stack (`MotionTrajectory`+`Chooser`+`AnimationWarping`+`AnimationLocomotionLibrary`) é quase o GASP inteiro — só falta `PoseSearch`. Mas isso é exploração, não MVP.

---

## 4. Estrutura do AnimGraph (bespoke)

```
[State Machine: Locomotion]
   ├─ Idle ──(Distance Match opcional)
   ├─ Start ──(start transition, doc 10)
   ├─ Loop ──(blendspace 8-way ou direcional por Gait, doc 09)
   └─ Stop ──(distance match stop, doc 10)
        │
[State Machine: Air]  ← entra quando bIsFalling / State.Combat.InAir
   ├─ JumpStart → Loop(Apex) → Fall → Land  (doc 13)
        │
   ↓ (saída comum)
[Stride Warping]  (doc 11, P1)
[Foot IK / Control Rig]  (doc 14, P2)
[Slot 'DefaultSlot']  ← montages de combate entram aqui (doc 15/16)
[Output Pose]
```

> O **Slot 'DefaultSlot'** é onde o combate (montages GAS) sobrescreve a locomoção. Locomoção embaixo, combate em cima. Ver [doc 15](../combat/15_Combat_Overview.md).

---

## 5. Debug — instrumente desde o início

Adote console vars de debug **agora** (prática AAA, barata):

```
ddr.LocomotionDebug 1   // HUD: Speed, Gait, Direction, bIsFalling, state atual
ddr.JumpDebug 1         // airtime, apex, landing prediction
```

> Mostre na tela: velocidade, gait atual, direção, estado da state machine. Economiza horas de "por que ele não troca de anim?". (Transferido do DungeonForged, que tem `df.LocomotionDebug` etc.)

---

## 6. Conexão com GAS (combate ↔ locomoção)

O AnimInstance **lê tags do ASC** pra decidir poses sem acoplar código:

```cpp
// no NativeThreadSafeUpdateAnimation:
bInAir   = ASC && ASC->HasMatchingGameplayTag(Tag_State_Combat_InAir);
bAttacking = ASC && ASC->HasMatchingGameplayTag(Tag_State_Combat_Attacking);
```

- `State.Combat.InAir` (do launcher) → AnimGraph usa state machine aérea de **combate** (não a de queda normal).
- `State.Movement.Dashing` → pode tocar pose de dash.

> Isto é o que liga o [doc 05 (GAS)](../systems/05_GAS_Architecture.md) ao [doc 16 (aéreo)](../combat/16_Aerial_Combos.md). Locomoção e combate conversam por **tags**, não por referências diretas.

---

## 7. Ordem de implementação da locomoção

```
1. CMC: gaits + sprint + pulo (doc 09, 13)        ← M0
2. AnimInstance + struct de estado (este doc §2)   ← M0
3. AnimGraph: Idle/Loop por gait (doc 09)          ← M0
4. AnimGraph: Air SM (doc 13)                      ← M0/M2
   ─── MVP jogável aqui ───
5. Start/Stop + distance match (doc 10)            ← P1, pós-combate
6. Stride warping (doc 11)                         ← P1
7. Height-based landing (doc 13)                   ← P1
8. Foot IK (doc 14)                                ← P2, se sobrar tempo
```

---

## 8. Checklist (arquitetura)

- [ ] `UDDRCharacterMovementComponent` é autoridade de movimento
- [ ] `UDDRAnimInstance` só **lê** estado, não comanda gameplay
- [ ] `FDDRLocomotionState` centraliza o estado (calculado 1×/frame)
- [ ] Gaits/modos como **enums**, não booleans soltos
- [ ] Cálculo em `NativeThreadSafeUpdateAnimation` (thread-safe)
- [ ] AnimGraph com SM Locomotion + SM Air + Slot DefaultSlot p/ combate
- [ ] Console debug (`ddr.LocomotionDebug`) desde o início
- [ ] AnimInstance lê tags do ASC (ponte com GAS)
- [ ] Decisão bespoke registrada (MM é spike P2)

---

## 9. Próximo passo

→ [09 — Gaits](09_Gaits.md): walk/run/sprint/crouch + transições de gait.
