# 56 — Combate Defensivo (Perfect-Dodge & Parry) · 🟢 P0 (dash) / 🟡 P1 (parry)

> **A camada defensiva do hack'n'slash:** esquiva com i-frames, perfect-dodge (counterplay barato) e parry ofensivo (parry→stagger→launch). Consolida o que estava espalhado em [19](19_Abilities_Deep.md), [20](../feel/20_Game_Feel.md), [21](../feel/21_Juice_FX.md) e a [Review de Combate](../design/Design_Review_Combat_2026-06.md). Pré-req: [18 §3/§5.6](18_Combat_System_Deep.md) · [07 Input](../systems/07_Input.md) · [39 Controles](../systems/39_Controls.md).

> 🔑 **Uma esquiva só no MVP:** `Dodge` → `GA_Dash`. **Roll cortado** — volta pós-MVP com assinatura distinta (longo, sem i-frame frontal).

---

## 1. Os três verbos defensivos

| Verbo | Input | Milestone | Função |
|---|---|:---:|---|
| **Dash / esquiva** | `IA_Dash` | 🟢 M1 | escape + i-frames generosos |
| **Perfect-dodge** | sub-janela do dash | 🟢 M1 | timing recompensado → witch-time |
| **Parry** | `IA_Parry` (P1) | 🟡 pós-core | counter ofensivo → stagger → launch |

```
Defesa em camadas:
  [Dash generoso]     → perdão ("confio no botão")
  [Perfect sub-janela]→ skill ("li o telegrafe e revidei")
  [Parry]             → mastery ("transformei defesa em combo")
```

> 🎯 A ofensiva (combo aéreo) é o pilar — mas **defesa ruim** faz o jogador mash dash sem ler telegrafe. Perfect-dodge e parry convertem *leitura* em *recompensa*.

---

## 2. ⚡ Perfect-dodge (M1 — P0)

**O que é:** se o player dasha e um hit inimigo cruza a **sub-janela perfect** (~6 primeiros frames / ~0.1s), dispara witch-time + cue — **sem sistema novo**, um `if` no pipeline de dano ([18 §3](18_Combat_System_Deep.md)).

### Implementação

```
OnIncomingDamage(HitInfo):
  if Target.HasTag(State.Movement.Dashing):
      if Target.HasTag(State.Combat.PerfectDodgeWindow):   // GE nos ~6 frames iniciais
          → ApplyGameplayCue(GameplayCue.Dodge.Perfect)
          → SetGlobalTimeDilation(0.3) + CustomTimeDilation(player, 1/0.3)
          → Cancelar dano (ou reduzir a 0)
          return BLOCKED
      else:
          → BLOCKED (i-frame generoso normal)
```

| Knob | Valor inicial | Onde tunar |
|---|---|---|
| Sub-janela perfect | ~6 frames (~0.1s) | `GE_Dash_PerfectWindow` Duration |
| Witch-time | 0.3× global, ~0.5s | `GA_Dash` / Eco SlowWorld |
| Hit-stop pré-slow | 8-10 frames | [21 §3](../feel/21_Juice_FX.md) |

> 🥷 **ROI máximo:** reusa `GA_Dash`, cue já existe em [21](../feel/21_Juice_FX.md), Eco `SlowWorld` estende duração ([40](../design/40_Eco_Pool_Catalog.md)). Entra no **M1** junto com hit-stop. Implementação C++ (direção, anti-parede, o que *não* fazer): [19 §3 — GA_Dash](19_Abilities_Deep.md).

### Tags

| Tag | Dono |
|---|---|
| `State.Movement.Dashing` | `GE_Dash_IFrames` (~0.2-0.3s) |
| `State.Combat.PerfectDodgeWindow` | `GE_Dash_PerfectWindow` (~0.1s, front-loaded) |
| `GameplayCue.Dodge.Perfect` | [21](../feel/21_Juice_FX.md) |

---

## 3. 🛡️ Parry ofensivo (P1 — pós-core)

**O que é:** janela curta antes do impacto — se o hit inimigo cruza o parry **de frente**, o inimigo entra em **stagger** (poise zerado) → abre launch/combo. **Ofensivo**, não passivo (Review de Combate — Rha+Kael).

> ⚠️ **Não é MVP-core** — entra **depois** do loop básico jogável (M4+). Clip existe no set ([22 §5](../22_Animation_Inventory.md)); sistema é barato (janela + tag + reusa poise).

### `GA_Parry` — ficha

| Campo | Valor |
|---|---|
| **Input** | `IA_Parry` (Started) — ver [39](../systems/39_Controls.md) |
| **AbilityTag** | `Ability.Parry` |
| **Owned** | `State.Combat.Parrying` |
| **Block** | `Cooldown.Parry` (~1.0-1.5s) |
| **Montage** | `AM_Parry` (1 clip do set) |
| **Instancing** | `InstancedPerActor` |

**Fluxo:**

```
ActivateAbility:
  1. CommitAbility (cooldown)
  2. PlayMontage AM_Parry
  3. Aplica GE_ParryWindow (~8-12 frames active parry)
       → tag State.Combat.ParryActive
  4. OnMontageEnded → EndAbility

OnIncomingDamage (pipeline 18 — ALVO = player):
  if Attacker.HasTag(State.Combat.HyperArmor): return NORMAL  // não parry hyperarmor
  if Defender.HasTag(State.Combat.ParryActive):
      if HitFromFront(Defender, HitDirection):   // cone ~120° frontal
          → ApplyGameplayCue(GameplayCue.Parry.Success)
          → Attacker: Poise = 0 → Stagger (18 §5.2)
          → Attacker: PlayMontage AM_ParryCounter (opcional no inimigo)
          → BLOCK damage to player
          → (opcional) auto-chain GA_Launcher se input bufferizado
          return BLOCKED
```

| Resultado | Juice |
|---|---|
| Parry success | freeze 8-10f + `GameplayCue.Parry.Success` + flash ([21](../feel/21_Juice_FX.md)) |
| Parry whiff | recovery punível (~20 frames) — risco |
| Hit during whiff | dano normal + player flinch ([18 §5.6](18_Combat_System_Deep.md)) |

> 🥊 **Parry → launch** é o fantasy: leu o telegrafe, puniu, **iniciou o pilar**. Hyperarmor continua não-parryável — ensina respeito ao windup forte ([32 §3](../enemies/32_Enemy_Combat_Behavior.md)).

---

## 4. Tensão generoso × perfect (resolvida)

| Camada | Duração | Efeito |
|---|---|---|
| **I-frame generoso** | ~0.2-0.3s | perdão — dash inteiro protege |
| **Sub-janela perfect** | ~0.1s (início) | timing → witch-time |

Sem sub-janela, i-frame generoso **mata** o perfect-dodge (nunca precisa timing). Com as duas camadas, ambos coexistem ([20 §4](../feel/20_Game_Feel.md)).

---

## 5. Dodge-offset · 🟡 P1 (opcional)

Problema ([18 §5.6](18_Combat_System_Deep.md)): defender **sempre** quebra combo → jogador nunca defende.

**Solução:** janela `State.Combat.DodgeOffset` no meio do combo onde dash **não** cancela o grafo de combo — preserva expressão contra hyperarmor.

```
if HasTag(DodgeOffset) && InputDash:
    GA_Dash activate WITHOUT CancelAbilitiesWithTag(Ability.Attack)
    → combo node preserved
```

> Implementar **depois** do perfect-dodge. É polish de skill ceiling, não bloqueante.

---

## 6. Player flinch (quando NÃO defende)

| Hit type | Efeito no player |
|---|---|
| Light | micro-flinch, combo continua (M1) |
| Heavy / stagger | interrompe combo — pune ganância |
| Durante hyperarmor próprio (slam) | imune — exceção telegrafada |

Clips: set Hit_* ([22](../22_Animation_Inventory.md)). Sem flinch, defesa não tem contrapartida.

---

## 7. Input & controles

| Ação | Teclado | Gamepad | Doc |
|---|---|---|---|
| Dash | `Space` / Shift | `B` / Circle | [39](../systems/39_Controls.md) |
| Parry | `F` (P1) | `LB` (P1) | adicionar ao 39 quando implementar |

> Parry **não** entra no buffer de combo ([07 §5](../systems/07_Input.md)) — é input instantâneo (Started only).

---

## 8. Prioridade

| Item | Quando |
|---|---|
| `GA_Dash` + i-frames generosos | 🟢 M1 |
| Perfect-dodge sub-janela + cue | 🟢 M1 |
| Player flinch leve/pesado | 🟡 M2-M3 |
| `GA_Parry` ofensivo | 🟡 pós-M4 (primeiro item pós-core) |
| Dodge-offset | 🟡 P1 polish |
| Roll (segunda esquiva) | 🔵 P2 |

---

## 9. Checklist

- [ ] `GA_Dash` concede `State.Movement.Dashing` via GE ([19 §3](19_Abilities_Deep.md))
- [ ] Sub-janela `State.Combat.PerfectDodgeWindow` nos ~6 frames iniciais
- [ ] Pipeline de dano checa tags antes de aplicar GE ([18 §3](18_Combat_System_Deep.md))
- [ ] `GameplayCue.Dodge.Perfect` + witch-time ([21](../feel/21_Juice_FX.md))
- [ ] Roll **não** implementado no MVP
- [ ] (P1) `GA_Parry` + stagger no atacante + cue success
- [ ] (P1) Parry **não** funciona vs hyperarmor
- [ ] (P1) `IA_Parry` no [07](../systems/07_Input.md) e [39](../systems/39_Controls.md)
- [ ] Player flinch calibrado por peso do hit

---

## 10. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Dash spam sem ler | Sem recovery/cooldown | [19](19_Abilities_Deep.md) cooldown ~0.6s |
| Perfect nunca dispara | Janela = dash inteiro | §2 — sub-janela só no início |
| Parry trivializa boss | Parry ignora hyperarmor | §3 — block vs HyperArmor |
| Combo quebra demais | Flinch pesado em todo hit | §6 — calibrar por tipo |
| Defesa inútil | Sem recompensa (perfect/parry) | §2-3 |

---

## 11. Próximo passo

→ [19 — Abilities](19_Abilities_Deep.md) (GA_Dash) · [18 — Combate Profundo](18_Combat_System_Deep.md) (pipeline) · [32 — Inimigo](../enemies/32_Enemy_Combat_Behavior.md) (telegrafe a ler) · [20 — Game Feel](../feel/20_Game_Feel.md).
