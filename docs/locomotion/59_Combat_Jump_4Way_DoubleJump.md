# 59 — Jump de Combate 4-Way + Double Jump · 🟢 P0 (M2 polish)

> **O que montar no editor + extensão C++ mínima** para o pulo direcional de combate e o segundo pulo no ar, usando o set `Sword_Animations/.../05_Jump/`. Complementa [13 — Jump/Fall/Landing](13_Jump_Fall_Landing.md) (física CMC) e [58 — AnimGraph](58_AnimGraph_Step_by_Step.md) (SM base). Integra com queda de slam via `Jump_Combat_*` ([58 §1.3](58_AnimGraph_Step_by_Step.md)).

> **Pré-req:** M0/M1 compilado · `ABP_DDRPlayer` com SM `Main States` (Locomotion → To Falling → Jump → Fall Loop → To Land → Land) · `IA_Jump` ligado ao `Character::Jump` ([54 §2](../systems/54_M0_Editor_Setup.md)).

---

## 0. Visão geral — o que você está montando

| Mecânica | Física (CMC) | Animação (ABP) |
|---|---|---|
| **1º pulo** | `JumpZVelocity` (700) + momentum horizontal | `Jump_Combat_*` **Start** (direção) → **Loop** (neutro) → **End** (direção) |
| **2º pulo (double)** | 2º impulso Z (ex.: 550) **só no ar** | `Double_Jump_Combat_*` (one-shot, mesma regra 4-way) |
| **Queda de borda** | gravidade normal | entra direto em **Fall Loop** (sem Start) |
| **Slam pós-End** | gravidade (`State.Combat.SlamFall`) | **Loop/End** de combate ([58 §1.3](58_AnimGraph_Step_by_Step.md)) |

> 🔑 **Regra de ouro ([08 §2.1](08_Locomotion_Overview.md)):** o CMC decide *quantos* pulos e *quando*; o ABP só escolhe *qual clip* com base no snapshot (`JumpDirection`, `bIsDoubleJump`, `bIsFalling`, `Velocity.Z`).

---

## 1. Inventário de assets (seu set)

Pasta canônica:

```
Content/Sword_Animations/Animations/Sequence2/05_Jump/
├── 01_Jump/                          ← pulo traversal (opcional no MVP)
├── 02_Jump_Combat/
│   ├── 01_Jump_Combat_0/             ← NEUTRO: Start + Loop + End
│   ├── 02_Jump_Combat_F_0/           ← FRENTE: Start + End
│   ├── 03_Jump_Combat_B_180/         ← TRÁS: Start + End
│   ├── 04_Jump_Combat_L_90/          ← ESQUERDA: Start + End
│   └── 05_Jump_Combat_R_90/          ← DIREITA: Start + End
├── 03_Double_Jump/
└── 04_Double_Jump_Combat/
    ├── Double_Jump_Combat_0
    ├── Double_Jump_Combat_F_0
    ├── Double_Jump_Combat_B_180
    ├── Double_Jump_Combat_L_90
    └── Double_Jump_Combat_R_90
```

### 1.1 Convenção de nomes (mapeamento ABP)

| Pasta | Sequences típicas | Seções / uso |
|---|---|---|
| `01_Jump_Combat_0` | `Jump_Combat_Start_0_Seq`, `Jump_Combat_Loop_*_Seq`, `Jump_Combat_End_0_Seq` | **Loop compartilhado** de queda/subida neutra |
| `02…05 Jump_Combat_*` | `Jump_Combat_Start_F_0`, `Jump_Combat_End_F_0` (etc.) | **Start** no takeoff · **End** no pouso |
| `04_Double_Jump_Combat` | `Double_Jump_Combat_F_0` (etc.) | one-shot no 2º pulo (sem Loop/End dedicados) |

> ⚠️ Os direcionais **não têm Loop próprio** — durante o ar (entre Start e pouso) use o **Loop neutro** de `01_Jump_Combat_0`. O **End** no pouso usa o clip da **direção guardada no pulo** (F/B/L/R ou neutro).

---

## 2. Direção do pulo (4-way)

Mesma filosofia do dodge ([59 dodge §0](../combat/59_Directional_Dodge.md)): direção **relativa ao facing** do personagem, não ao mundo.

### 2.1 Setores (4 quadrantes + neutro)

No momento do **Started** do `IA_Jump`, capture o vetor de input de movimento (WASD) no plano XY. Se magnitude < deadzone → **Neutro (`0`)**.

```
Ângulo = Atan2(Cross(Forward, InputDir).Z, Dot(Forward, InputDir))   // graus -180..180

  Neutro     : |Input| < deadzone (ex. 0.15)
  F (0°)     : -45° .. +45°
  R (+90°)   : +45° .. +135°
  B (±180°)  : > +135° ou < -135°
  L (-90°)   : -135° .. -45°
```

| Enum sugerido | Clip Start | Clip End (pouso) | Loop no ar |
|---|---|---|---|
| `Neutral` | `Jump_Combat_Start_0` | `Jump_Combat_End_0` | `Jump_Combat_Loop` (01) |
| `Forward` | `Jump_Combat_Start_F_0` | `Jump_Combat_End_F_0` | Loop neutro |
| `Back` | `Jump_Combat_Start_B_180` | `Jump_Combat_End_B_180` | Loop neutro |
| `Left` | `Jump_Combat_Start_L_90` | `Jump_Combat_End_L_90` | Loop neutro |
| `Right` | `Jump_Combat_Start_R_90` | `Jump_Combat_End_R_90` | Loop neutro |

> 🎯 **Pulo parado + WASD:** se o jogador segura W ao pular, cai em **Forward** mesmo parado — o input manda, não a velocidade (evita sempre neutro).

### 2.2 Double jump — mesma regra de direção

No **2º** `Started` do Jump (já `IsFalling()`), recalcule a direção **no frame do 2º pulo** e toque o clip `Double_Jump_Combat_<dir>`. Não precisa de End dedicado: após o one-shot, volta pro **Fall Loop** neutro até o pouso (End da direção **do 1º pulo** ou recalcule no land — recomendado: **guardar `LandDirection` = direção do último pulo que ainda não pousou**).

---

## 3. Extensão C++ ✅ (implementado)

O snapshot [`FDDRLocomotionState`](../../Source/DarkDungeonRift/Animation/DDRLocomotionTypes.h) expõe:

| Campo | Tipo | Uso AnimBP |
|---|---|---|
| `JumpDirection` | `EDDRJumpDirection` | qual clip Start / Double (4-way) |
| `LandDirection` | `EDDRJumpDirection` | qual clip End no pouso |
| `bIsDoubleJump` | bool | **pulso 1 frame** — SÓ na **transição** Fall Loop → Jump |
| `bJumpFromAir` | bool | **persistente** — **conteúdo** do estado Jump (Blend by Bool: true → `Double_Jump_Combat_*`) |
| `bIsAirCombat` | bool | flutuar no juggle (hold aéreo, Flying) — entra/permanece no **Fall Loop** (§4.2) |
| `AirJumpIndex` | int32 | 0 / 1 / 2 (debug) |

> ⚠️ **`bIsDoubleJump` (pulso) ≠ `bJumpFromAir` (persistente) — não confunda:** o pulso vive **1 frame** (acorda a transição e morre). O conteúdo do estado Jump avalia **1 frame depois** que a transição completa — aí o pulso já é `false` e o Blend by Bool **sempre cairia no Start normal**. Por isso o conteúdo usa o flag **persistente** `bJumpFromAir`, que só zera no pouso. (Era exatamente o bug "double jump sempre toca anim normal".)

`UDDRCharacterMovementComponent::TryCombatJump` (chamado por `ADDRPlayerCharacter::OnJumpPressed`):

```
OnJumpPressed():
  if (input travado || IsAirHorizontalInputLocked) return       // launcher/juggle/pin
  if (tag State.Combat.SlamFall) return                         // mergulho do slam: pulo NÃO cancela o -3500
  if (CMC->IsFalling()):
      if (AirJumpCount >= MaxAirJumps) return
      if (AirJumpCount == 0) AirJumpCount = 1                   // queda de BORDA consome o slot do chão
      AirJumpCount++                                            //   (borda = 1 pulo aéreo, igual pós-pulo)
      CaptureJumpDirection → JumpDirection, LandDirection
      bDoubleJumpJustTriggered = true
      Velocity.Z = max(Vel.Z,0) + DoubleJumpZVelocity
  else:
      if (!CanJump() || AirJumpCount > 0) return
      AirJumpCount = 1
      CaptureJumpDirection → JumpDirection, LandDirection
      Character::Jump()

OnMovementModeChanged (Falling OU Flying → Ground):              // Flying: pin do slam solta direto
  ResetAirJumpState()  // AirJumpCount; LandDirection mantém-se p/ anim Land
```

| Parâmetro | Default | Onde |
|---|---|---|
| `MaxAirJumps` | **2** | CMC `DDR\|Jump` |
| `JumpZVelocity` | **700** ✅ | CMC |
| `DoubleJumpZVelocity` | **550** | CMC `DDR\|Jump` |
| `JumpDirectionInputDeadzone` | **0.15** | CMC `DDR\|Jump` |

Bloqueios: `bBlockLocomotionInput` · `Combat->IsAirHorizontalInputLocked()` (launcher/juggle/pin).

Log: `[JUMP] ground dir=…` · `[JUMP] double dir=…` · `ddr.LocomotionDebug 1` mostra Jump/Air no HUD.

### 3.1 Enum (referência)

```cpp
UENUM(BlueprintType)
enum class EDDRJumpDirection : uint8
{
    Neutral, Forward, Back, Left, Right
};
```

### 3.2 ~~Lógica no `ADDRPlayerCharacter`~~ — feito

Substituído bind `ACharacter::Jump` por `OnJumpPressed` → `TryCombatJump`.

### 3.3 Expor no AnimBP

No Event Graph do `ABP_DDRPlayer`, além do [58 §1.2](58_AnimGraph_Step_by_Step.md):

```
Break DDRLocomotionState → JumpDirection, LandDirection, bIsDoubleJump, AirJumpIndex
```

---

## 4. State Machine `Main States` (AnimGraph)

Alinha com o graph que você já tem (screenshot): **Locomotion · To Falling · Jump · Fall Loop · To Land · Land**.

```
                    ┌─────────────┐
         ┌─────────│  Locomotion │◀────────────────────────┐
         │         └──────┬──────┘                         │
         │                │ bIsFalling (queda borda)        │
         │                ▼                                │
         │         ┌─────────────┐                         │
         │         │ To Falling  │ (blend curto opcional)  │
         │         └──────┬──────┘                         │
         │                │                                │
    NOT  │    bIsFalling + acabou de sair do chão           │
  Falling│                ▼                                │
         │         ┌─────────────┐   bIsDoubleJump         │
         │         │    Jump     │◀── (transição interna   │
         │         │ Start/Double│    ou sub-estado)       │
         │         └──────┬──────┘                         │
         │                │ Automatic (Start terminou)     │
         │                ▼                                │
         │         ┌─────────────┐                         │
         │         │  Fall Loop  │◀── bIsCombatFalling?     │
         │         │ Loop neutro │    Jump_Combat_Loop      │
         │         └──────┬──────┘                         │
         │                │ trace / NOT bIsFalling soon     │
         │                ▼                                │
         │         ┌─────────────┐                         │
         │         │   To Land   │                         │
         │         └──────┬──────┘                         │
         │                ▼                                │
         │         ┌─────────────┐                         │
         └────────▶│    Land     │── Automatic ────────────┘
                   │ End por dir │
                   └─────────────┘
```

### 4.1 Estado **Jump** (conteúdo) — passo a passo de nós

Dentro do estado `Jump` (duplo-clique nele):

1. Clique-direito no grafo → **Blend Poses by Enum** → escolha `EDDRJumpDirection`. No nó, clique-direito no corpo → **Add Pin** até ter os 5 valores (Neutral/Forward/Back/Left/Right).
2. Arraste do Asset Browser um **Sequence Player** para cada pino: `Jump_Combat_Start_0_Seq` (Neutral), `Start_F_0` (Forward), `Start_B_180` (Back), `Start_L_90` (Left), `Start_R_90` (Right). ⚠️ Em **todos**: `Loop = OFF`.
3. **Active Enum Value** ← variável `JumpDirection`.
4. Crie um **segundo** Blend Poses by Enum igual, com os 5 `Double_Jump_Combat_*` (Loop OFF), Active Enum Value ← `JumpDirection`.
5. Una os dois com **Blend Poses by Bool**: `True` = blend dos Double · `False` = blend dos Start · **Active Value** ← **`bJumpFromAir`** → **Output Animation Pose**.

> 🐞 **SE o double sempre toca a anim normal:** o Active Value do Blend by Bool está ligado em **`bIsDoubleJump`** (pulso) em vez de **`bJumpFromAir`** (persistente). Troque a variável — é a causa nº 1. O pulso morre antes do estado avaliar (ver §3 ⚠️). A **transição** `Fall Loop → Jump` continua usando `bIsDoubleJump`; só o **conteúdo** usa `bJumpFromAir`.

> 🔁 Re-entrar no estado (Fall Loop → Jump no double) **reinicia** os sequence players automaticamente — o one-shot do double toca do frame 0.

Mapa de referência:

| Condição | Clip |
|---|---|
| `bJumpFromAir == true` | `Double_Jump_Combat_<JumpDirection>` (4-way + neutro) |
| `bJumpFromAir == false`, `Neutral` | `Jump_Combat_Start_0` |
| `… false`, `Forward` | `Jump_Combat_Start_F_0` |
| … | … |

**Transição Jump → Fall Loop:** Automatic Rule, *Time Remaining* < 0.05 (Start curto).

**Double jump no ar:** quando `bIsDoubleJump` sobe a true **já em Fall Loop**, pode:
- **Opção A (simples):** transição `Fall Loop → Jump` com condição `bIsDoubleJump` (re-toca Start/Double one-shot).
- **Opção B:** sub-estado `DoubleJump` dentro de Fall Loop (só o one-shot, depois volta ao Loop).

### 4.2 Estado **Fall Loop**

Conteúdo: um Sequence Player com `Jump_Combat_Loop_0_Seq` (**Loop ON**). Subida e descida compartilham o Loop neutro; opcional P1: Rising vs Falling por `Velocity.Z > 0`.

> 🪂 **CRÍTICO — flutuar no juggle:** durante o hold aéreo o player está em **`MOVE_Flying`**, então **`bIsFalling = false`** — sem tratamento, ele fica **parado sem anim no ar** (o sintoma que você viu). A entrada do Fall Loop precisa do flag `bIsAirCombat`:

| Transição (entra/permanece no Fall Loop) | Condição |
|---|---|
| Locomotion / To Falling → Fall Loop | `bIsFalling` **OU** `bIsAirCombat` |
| Jump → Fall Loop | Automatic (Start terminou) |
| Fall Loop → Land | `NOT bIsFalling` **AND** `NOT bIsAirCombat` |

> Assim: launcher sobe → montage acaba → `bIsAirCombat=true` (flutua no Loop) → você ataca (montage por slot sobrepõe) → para de atacar → após `PlayerAirHoldSeconds` o C++ solta `Falling` (`bIsAirCombat→false`, `bIsFalling→true`, segue no Loop) → pousa → Land. **Nunca** parado sem anim.

### 4.3 Estado **Land**

**Blend by `LandDirection`** (enum guardado no pulo):

| LandDirection | Clip |
|---|---|
| `Neutral` | `Jump_Combat_End_0` |
| `Forward` | `Jump_Combat_End_F_0` |
| `Back` | `Jump_Combat_End_B_180` |
| `Left` | `Jump_Combat_End_L_90` |
| `Right` | `Jump_Combat_End_R_90` |

**Transição Land → Locomotion:** Automatic + `Land → Move` se `bIsMoving` ([58 §3.2](58_AnimGraph_Step_by_Step.md)).

### 4.4 Transições críticas

| De → Para | Condição |
|---|---|
| Locomotion → Jump | `bIsFalling` **e** `Velocity.Z > 0` (takeoff — não queda de borda) |
| Locomotion → To Falling → Fall Loop | `bIsFalling` **e** `Velocity.Z <= 0` (borda) |
| Fall Loop → Land | `NOT bIsFalling` |
| Fall Loop → Jump | `bIsDoubleJump` (2º pulo) |

---

## 5. Montages (opcional)

Para **root motion** nos Starts direcionais, crie montages por direção ou uma `AM_Jump_Combat` com seções:

| Seção | Sequence |
|---|---|
| `Start_N` | Start_0 |
| `Start_F` | Start_F_0 |
| … | … |

No MVP **sem root motion no pulo**, use **sequences direto no ABP** (como no seu workflow atual). Montage só se quiser RM autorado no Start.

---

## 6. Integração com combate aéreo

| Situação | Pulo normal? | Anim |
|---|---|---|
| Juggle (`State.Combat.Airborne` no **alvo**) | alvo: não · player: sim se não locked | player usa esta SM |
| Launcher / pin (`LockAirHorizontalInput`) | **bloqueado** | — |
| Slam descendente (`State.Combat.SlamFall`) | bloqueado | Fall Loop/End combate ([58 §1.3](58_AnimGraph_Step_by_Step.md)) |
| Jump-cancel AirAttack (P1, [19 §4](../combat/19_Abilities_Deep.md)) | 2º impulso via GAS, não via CMC double jump | pode reutilizar `Double_Jump_Combat_*` |

---

## 7. Checklist editor

- [ ] Sequences importadas em `05_Jump/` com nomes acima
- [x] Enum `EDDRJumpDirection` + snapshot no C++ ✅
- [x] `TryCombatJump` + `OnJumpPressed` no player ✅
- [x] `MaxAirJumps = 2`, `DoubleJumpZVelocity` tunável no CMC ✅
- [ ] ABP: vars `JumpDirection`, `LandDirection`, `bIsDoubleJump`
- [ ] SM `Main States`: Jump (Start/Double) → Fall Loop → Land (End 4-way)
- [ ] Ramo `bIsCombatFalling` no Fall Loop ([58 §1.3](58_AnimGraph_Step_by_Step.md))
- [ ] `Land → Move` com prioridade se correndo
- [ ] Slot `DefaultSlot` **depois** da SM ([58 §4](58_AnimGraph_Step_by_Step.md))
- [ ] PIE: neutro / F / B / L / R · double jump · queda de borda · slam fall

---

## 8. Teste (PIE)

| Teste | Esperado |
|---|---|
| Space parado | Start_0 → Loop → End_0 |
| W + Space | Start_F → Loop → End_F |
| S + Space | Start_B → … |
| A / D + Space | Start_L / Start_R |
| Space no ar (1×) | Double_Jump_Combat_* → Loop → End (dir do 2º ou 1º pulo) |
| 3× Space | 3º ignorado |
| Sair de plataforma | Fall Loop direto (sem Start) |
| Slam pós-End | Loop/End combate, não End genérico de traversal |
| Launcher ativo | Jump não responde |

Console: `ddr.LocomotionDebug 1` — confira `Fall=1` e gait durante o ar.

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Sempre anim neutra | direção não capturada no Jump | §3.2 — input no Started, não Velocity |
| **Double jump sempre toca anim NORMAL** | Blend by Bool ligado em `bIsDoubleJump` (pulso de 1 frame, morre antes do estado avaliar) | §4.1 — ligar em **`bJumpFromAir`** (persistente). Recompile (campo novo no snapshot) |
| End errado no pouso | `LandDirection` sobrescrito no 2º pulo | guardar End do 1º pulo ou recalcular só no land |
| Double jump não sai | bind ainda em `ACharacter::Jump` cru | handler custom + `AirJumpCount` |
| 2º pulo no chão | falta check `IsFalling()` | §3.2 |
| Atravessa loop Start→End | transição automática longa demais | Automatic Rule < 0.05 no Start |
| Preso no Fall | falta `NOT bIsFalling` → Land | §4.4 |
| **Player PARADO sem anim no ar** (juggle/launcher) | Flying → `bIsFalling=false`; Fall Loop não entra | §4.2 — condição do Fall Loop = `bIsFalling OR bIsAirCombat` (recompile) |
| Player não pousa (preso no Loop após juggle) | falta `NOT bIsAirCombat` na saída pro Land | §4.2 — `Fall Loop → Land = NOT bIsFalling AND NOT bIsAirCombat` |
| Pulo durante juggle move player | lock aéreo off | `IsAirHorizontalInputLocked()` |
| Slam usa End traversal | `bIsCombatFalling` false | tag `State.Combat.SlamFall` + [60 §8](../systems/60_M2_Editor_Setup.md) |
| **Space no MERGULHO do slam subia o player** (anulava o -3500) | `TryCombatJump` não checava `SlamFall` | ✅ corrigido — pulo bloqueado pela tag (recompile) |
| **Queda de borda dava 2 pulos aéreos** (pulo normal dá 1) | borda não consumia o slot do chão | ✅ corrigido — 1º pulo aéreo da borda conta como os dois (recompile) |
| **Pulo "morto" no chão depois de um slam** | pin soltava `Flying→Walking` sem resetar `AirJumpCount` | ✅ corrigido — reset também vindo de Flying (recompile) |

---

## 10. Próximo passo

→ Polish P1: coyote/buffer ([13 §3](13_Jump_Fall_Landing.md)) · height-based land ([13 §6](13_Jump_Fall_Landing.md)) · `GA_Jump` GAS ([19](../combat/19_Abilities_Deep.md)) para jump-cancel aéreo.
