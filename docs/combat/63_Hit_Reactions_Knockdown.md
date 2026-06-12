# 63 — Reações: Hits 4-Way, Knockdown Animado & Get-Up · 🟢 P0 (M3)

> **O sistema de reações** — o que o corpo faz ao TOMAR golpe: flinch direcional (chão/ar, leve/pesado), a **queda animada do slam** (substitui o ragdoll como caminho canônico), deitado → **get-up**, e o mapping de **block/parry** (P1). Vale para **player E inimigo** — tudo vive na base `ADDRCharacterBase`; o BP só assigna montages. É a metade do juice do pilar: *"um inimigo sem boas reações é um saco de pancada sem peso"* ([31 §5](../enemies/31_Enemy_Archetypes.md)).

> ✅ **STATUS C++: implementado** (`PlayHitReaction`, knockdown animado de 5 seções, get-up, respiro, integração no pipeline de dano). **Você** cria 4 montages no editor (§4) e assigna nos BPs.

> **Docs:** [16 §3.1 SM do alvo](16_Aerial_Combos.md) · [18 §5 poise/stagger](18_Combat_System_Deep.md) · [32 comportamento](../enemies/32_Enemy_Combat_Behavior.md) · [56 defensivo](56_Defensive_Combat.md) · [60 §4 slam](../systems/60_M2_Editor_Setup.md) · [61 M3 setup](../systems/61_M3_Editor_Setup.md)

---

## 0. Checklist rápido

- [x] C++ compilado (base: `PlayHitReaction` + knockdown + getup)
- [ ] `AM_Hit_Combat` (leve, 4 seções F/B/L/R) — §4.1
- [ ] `AM_Hit_Large_Combat` (pesado, 4 seções) — §4.1
- [ ] `AM_Hit_Combat_Air` (no ar, 4 seções) — §4.1
- [ ] `AM_Knockdown` (5 seções: queda do slam → deitado → getup) — §4.2
- [ ] Assign nas 4 montages no `BP_TrainingDummy` / `BP_Enemy_*` (e no `BP_DDRPlayer` quando tomar dano)
- [ ] PIE: cada hit flincha na direção certa; slam = queda animada + deita + levanta (§6)

---

## 1. Mapa clip → uso (o inventário)

| Pasta / clips | Uso | Quem toca |
|---|---|---|
| `02_Hit_Combat` (4 direções) | **Flinch leve** no chão | `PlayHitReaction(light)` — todo hit comum |
| `06_Hit_Large_Combat` (4 dir.) | **Flinch pesado** no chão | `PlayHitReaction(heavy)` — launch/slam/`bHeavyHitReaction` no hitbox |
| `04_Hit_Combat_Air` (4 dir.) | **Flinch no AR** | cada pop do juggle — *vende o combo hit a hit* |
| `08_Hit_Combat_Air_Large_To_Floor` Start/Loop/End (4 dir.) | **Queda do slam** (knockdown aéreo→chão) | seções `Fall_Start/Fall_Loop/Fall_End` do `AM_Knockdown` |
| `11_Knock_Down_Combat_Loop_Seq` | **Deitado no chão** | seção `Ground` do `AM_Knockdown` |
| `11_Knock_Down_Combat_End_Seq` | **Levantar do knockdown** (autorado p/ emendar com o `Loop` do set 11) | seção `GetUp` — **opção A** (ver §4.2) |
| `09_Get_Up/Get_Up_Combat_Seq` | **Levantar genérico** | seção `GetUp` — **opção B** · também o getup de outros estados futuros |
| `11_Knock_Down_Combat_Start_Seq` | knockdown iniciado NO CHÃO (guard-break `Block_Hit_Break`, golpe pesado sem launch) | 🟡 P1 — seção `Ground_Start` reservada no mesmo `AM_Knockdown` |
| `11_Knock_Down_Combat_Death_Seq` | **morte caído** (juggle/slam letal) | 🟡 P1 — seção `Death`; pipeline de morte (ragdoll blend no fim) |
| `Block_Start/Loop/End/Hit/Hit_2/Hit_Break` | bloqueio + guard break | 🟡 P1 — §5 |
| `Parry_L_Seq` / `Parry_R_Seq` | parry ofensivo | 🟡 P1 — §5 |

---

## 2. Arquitetura — por que assim (a decisão AAA)

**Queda do slam: ANIMADA, não ragdoll.** O padrão AAA (DMC/Bayonetta/GoW) é knockdown **autorado**: pose dramática legível, impacto no frame certo, identidade. Ragdoll puro é físico mas sem leitura — vira "boneco mole" aleatório. A hierarquia implementada:

```
EndAirborne(slam) →
  1º  AM_Knockdown assignada?  → QUEDA ANIMADA (canônico)
  2º  bRagdollOnSlammed + PA?  → ragdoll no pouso (fallback físico)
  3º  nada                      → queda guiada de cápsula (pose congelada)
```

> 💀 **Ragdoll fica reservado pra MORTE** (P1): morte no ar/derrubado = ragdoll (ou `Knock_Down_Death` com blend físico no fim). Vivo = animado.

**O fluxo do knockdown animado** (mesma máquina de seções do slam do player — `Montage_SetNextSection` no C++, seções SEM link no editor):

```
slam conecta → EndAirborne(slam)
   ├─ cápsula: queda GUIADA (sweep até o chão — já existia; dirige o MOVIMENTO)
   └─ montage: Fall_Start → Fall_Loop (self-link; dura o que a queda durar)  ← POSE
            pouso ↓
        Fall_End (impacto) → Ground (deitado, self-link, KnockdownGroundSeconds=1.0s)
            ↓ timer
        GetUp (Get_Up_Combat) → montage acaba → FinishKnockdown
```

**Garantias do C++ durante o knockdown:**
- Tag **`State.Combat.Stagger`** do início ao fim do getup → o **BT da IA fica pausado** (decorator do [61 §4.1](../systems/61_M3_Editor_Setup.md) lê Airborne OU Stagger) — o inimigo não "pensa" caído.
- **Inatingível** enquanto caído (`CanHitActor` ignora `IsKnockedDown()`/`IsRagdolled()`) — janela de respiro do knockdown ([16 §8](16_Aerial_Combos.md)).
- Montage interrompida/morte no caminho → `FinishKnockdown` limpa tudo (sem tag presa).
- Logs: `[KNOCKDOWN] queda ANIMADA ON · POUSO → Fall_End → Ground · GetUp · LEVANTOU (IA retoma)`.

**Flinch 4-way (`PlayHitReaction`)** — chamado pelo pipeline de dano em TODO hit:
- Direção = ângulo **atacante→vítima** relativo ao facing da vítima → seção `F`/`B`/`L`/`R` (mesma matemática 4 setores do dodge/jump).
- Severidade: `heavy` se o hitbox tem `bLaunchTargets`/`bSlamDownTargets`/**`bHeavyHitReaction`** (campo novo no `ANS_DDRHitbox`); senão `light`. **No ar** usa sempre o flinch aéreo.
- Não sobrepõe estados maiores (knockdown/ragdoll/morte/queda do slam).

---

## 3. Regras player × inimigo (assimetria deliberada)

| Regra | Inimigo | Player |
|---|---|---|
| Flinch leve interrompe o próprio ataque? | **SIM** (trash interrompível é design, [32 §3](../enemies/32_Enemy_Combat_Behavior.md)) — hyperarmor protege os fortes (P1) | **NÃO** (`bLightHitReactionWhileAttacking=false` no ctor) — light vira só hit-stop/cue; **heavy** interrompe |
| Knockdown | slam do player derruba (queda animada + getup) | latente e pronto (golpe pesado de elite, M4) — mesma máquina |
| Get-up | `Get_Up_Combat` após `KnockdownGroundSeconds` | idem |
| Flinch no ar | cada pop do juggle (o juice do pilar) | n/a no MVP (player não é juggleado) |

> ⚙️ O knob é da **base** (`bLightHitReactionWhileAttacking`) — qualquer BP pode inverter. O elite com hyperarmor (P1) vai **suprimir o flinch** via tag `State.Combat.HyperArmor` checada antes do `PlayHitReaction` (gancho documentado, [32 §3](../enemies/32_Enemy_Combat_Behavior.md)).

---

## 4. Setup no editor (as 4 montages)

> ⚠️ Regras de sempre: **Montage Sections → Clear** (links são do C++) · clips de reação com **`EnableRootMotion = OFF`** (a cápsula é dirigida pelo código; RM no flinch desliza a cápsula) · Slot `DefaultSlot`.

### 4.1 Flinches — `AM_Hit_Combat` · `AM_Hit_Large_Combat` · `AM_Hit_Combat_Air`

1. Crie a montage com os **4 clips** do set (ex.: `Hit_Combat_F/B/L/R`).
2. Crie **4 seções nomeadas EXATAS**: `F`, `B`, `L`, `R` — uma por clip. **Clear nos links!**
3. Repita para os 3 sets (02 leve, 06 pesado, 04 aéreo).

### 4.2 Knockdown — `AM_Knockdown` (5 seções)

| Seção (nome EXATO) | Clip |
|---|---|
| `Fall_Start` | `Hit_Combat_Air_Large_To_Floor_Start_Seq` |
| `Fall_Loop` | `Hit_Combat_Air_Large_To_Floor_Loop_Seq` |
| `Fall_End` | `Hit_Combat_Air_Large_To_Floor_End_Seq` |
| `Ground` | `Knock_Down_Combat_Loop_Seq` |
| `GetUp` | `Get_Up_Combat_Seq` |

**Clear nos links** — o C++ encadeia (`Fall_Start→Fall_Loop` self-link na queda; pouso pula pra `Fall_End→Ground` self-link; timer → `GetUp`; `GetUp` sem next = montage acaba = fim do knockdown). MVP usa os clips **F** (frontal); queda 4-way é P1 (os clips existem).

### 4.3 Assign nos BPs (categoria `DDR|Reactions`)

| Campo | Asset | Em quem |
|---|---|---|
| Hit Reaction Montage | `AM_Hit_Combat` | dummy, inimigos M3, player |
| Hit Reaction Heavy Montage | `AM_Hit_Large_Combat` | idem |
| Air Hit Reaction Montage | `AM_Hit_Combat_Air` | dummy/inimigos (quem é juggleado) |
| Knockdown Montage | `AM_Knockdown` | dummy/inimigos (player: M4) |
| Knockdown Ground Seconds | 1.0 (dummy) · 0.8 (grunt) | tune por arquétipo |

> 🧷 **Requisito:** o personagem precisa de **AnimInstance** (ABP com `DefaultSlot`). Sem ABP, `Montage_Play` falha e o código **cai no fallback ragdoll** sozinho (log `[KNOCKDOWN] sem AnimInstance`).

### 4.4 Hitbox: marcando golpe pesado

No `ANS_DDRHitbox` de golpes fortes (sem launch/slam): **`bHeavyHitReaction = ✅`** → flinch `06_Hit_Large` no alvo. Launch/slam já contam como heavy automaticamente.

---

## 5. 🛡️ Block & Parry — mapping (P1, design no [56](56_Defensive_Combat.md))

Os clips já cobrem o sistema defensivo inteiro; quando o 56 for implementado:

| Clip | Uso futuro |
|---|---|
| `Block_Start` → `Block_Loop` (hold) → `Block_End` | `GA_Block` (segurar RMB): entrar → loop → soltar |
| `Block_Hit` / `Block_Hit_2` | chip reaction (alterna) — dano reduzido no GE |
| `Block_Hit_Break` | **guard break** (stamina/poise do bloqueio zera) → emenda no `11_Knock_Down_Start` (knockdown de chão) |
| `Parry_L` / `Parry_R` | `GA_Parry`: janela perfeita → riposte; L/R pelo lado do golpe (mesma matemática do `ComputeHitReactionSection`) |

> A função de direção 4-way da base (**`ComputeHitReactionSection`**) já serve o parry L/R de graça.

---

## 6. Teste PIE

| Teste | Esperado |
|---|---|
| LMB no dummy por trás/lados | flinch na **direção certa** (B/L/R) — log `[HIT-REACT]` |
| Golpe com `bHeavyHitReaction` | flinch pesado (06) |
| Juggle (E → LMB×n) | dummy **flincha no ar a cada pop** (04) — o juggle ganha vida |
| **R (slam)** | queda **animada** (Start→Loop), impacto no chão (Fall_End), **deita** (Ground ~1s), **levanta** (GetUp) — sem ragdoll, sem pose congelada |
| Bater no dummy caído | **nada** (inatingível — respiro) até levantar |
| Inimigo M3 caído | IA **parada** (Stagger) até o fim do getup — log `LEVANTOU (IA retoma)` |
| Player tomando hit leve no meio do combo | combo **não** corta (só cue/hit-stop); hit pesado corta |
| Sem `AM_Knockdown` assignada | fallback ragdoll (com PA) — nada quebra |

---

## 7. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Slam ainda vira ragdoll | `Knockdown Montage` não assignada no BP | §4.3 |
| `[KNOCKDOWN] sem AnimInstance` | mesh sem ABP | assigne o ABP (Anim Class) na mesh do BP |
| Flinch sempre frontal | seções não se chamam `F/B/L/R` exatas | §4.1 |
| Dummy levanta "do nada" no meio da queda | seções do knockdown com **links** na montage | **Clear** — o C++ encadeia |
| Cápsula desliza no flinch | RM ON nos clips de hit | §4 — RM OFF |
| Inimigo ataca deitado | decorator do BT sem `State.Combat.Stagger` | [61 §4.1](../systems/61_M3_Editor_Setup.md) |
| Player perde o combo a cada arranhão | `bLightHitReactionWhileAttacking` true no player | ctor já seta false; confira override no BP |
| Dummy eterno no chão | `Knockdown Ground Seconds` = 0 + sem GetUp na montage | §4.2/§4.3 |

---

## 8. Próximo passo

→ **Hyperarmor** (suprime flinch nos golpes comprometidos do elite, [32 §3](../enemies/32_Enemy_Combat_Behavior.md)) · **morte com ragdoll/`Knock_Down_Death`** · **block/parry** ([56](56_Defensive_Combat.md), mapping §5) · queda 4-way (clips existem).
