# 41 — Catálogo de Gameplay Tags · 🟢 P0

> **Referência única** de todas as `FGameplayTag` do Dark Dungeon Rift. GAS, AnimBP, IA, UI e data-driven leem estas tags — **não** strings soltas. Registre em `Config/DefaultGameplayTags.ini` ou `UDDRGameplayTags.h` ([04 §4](04_Project_Setup.md)).

> 🔑 **Convenção:** inglês, hierarquia por ponto, `PascalCase` no último segmento. Tags de **identidade** (`Ability.*`, `Eco.*`) ≠ tags de **estado** (`State.*`).

---

## 1. Mapa das famílias

```
Ability.*          → o que a entidade PODE fazer (identidade GAS)
State.*            → o que a entidade ESTÁ fazendo (transiente)
Cooldown.*         → janelas de recarga
Cost.*             → marcadores de custo (stamina, etc.)
Eco.*              → upgrades de run (Ecos)
GameplayCue.*      → juice (VFX/SFX/shake) — ver 21 §10
Meta.*             → progressão permanente (hub)
Event.*            → GameplayEvents (notify → ability)
```

---

## 2. `Ability.*` — identidade de habilidades

| Tag | Ability | Input ([39](39_Controls.md)) | Doc |
|---|---|---|---|
| `Ability.Attack` | pai (cancelamento amplo) | — | [19](../combat/19_Abilities_Deep.md) |
| `Ability.Attack.Light` | `GA_Attack_Light` | LMB / X | [19](../combat/19_Abilities_Deep.md) |
| `Ability.Attack.Launcher` | `GA_Launcher` | E / B | [16](../combat/16_Aerial_Combos.md) |
| `Ability.Attack.Air` | `GA_AirAttack` | LMB / X (requer InAir) | [16](../combat/16_Aerial_Combos.md) |
| `Ability.Attack.AirSlam` | `GA_AirSlam` | R / Y | [16](../combat/16_Aerial_Combos.md) |
| `Ability.Dash` | `GA_Dash` | Space / A | [19](../combat/19_Abilities_Deep.md) |
| `Ability.Dash.Strike` | `GA_Dash_Strike` (Eco Fúria) | — | [40](../design/40_Eco_Pool_Catalog.md) |
| `Ability.Interact` | interação mundo | F / RT | [39](39_Controls.md) |

---

## 3. `State.*` — estados transientes

### Combate

| Tag | Quem | Concedida por | Efeito |
|---|---|---|---|
| `State.Combat.Attacking` | player/inimigo | abilities de ataque (`ActivationOwnedTags`) | bloqueia outras ações |
| `State.Combat.InAir` | **player** | `GA_Launcher` / pulo | habilita `GA_AirAttack`, `GA_AirSlam` |
| `State.Combat.Airborne` | **alvo** | `GA_Launcher` / knockup | juggle; **IA pausa** ([30 §6](../enemies/30_AI_Overview.md)) |
| `State.Combat.Stagger` | alvo | poise ≤ 0 | vulnerável; pode lançar |
| `State.Combat.Stunned` | alvo | slam + Eco Vazio | não age |
| `State.Combat.HyperArmor` | inimigo | windup forte ([32](../enemies/32_Enemy_Combat_Behavior.md)) | ignora flinch |
| `State.Combat.LaunchImmune` | boss/elite | fase boss | não lança (temporário) |
| `State.Dead` | qualquer | HP ≤ 0 | bloqueia tudo |

### Movimento

| Tag | Concedida por | Efeito |
|---|---|---|
| `State.Movement.Dashing` | `GA_Dash` (GE i-frame) | imune a dano; pipeline [18](../combat/18_Combat_System_Deep.md) |
| `State.Movement.Sprinting` | hold sprint | gait sprint ([09](../locomotion/09_Gaits.md)) |
| `State.Movement.Jumping` | pulo | jump-cancel aéreo |

---

## 4. `Cooldown.*` e `Cost.*`

| Tag | Ability | Duração típica |
|---|---|---|
| `Cooldown.Dash` | `GA_Dash` | 0.5–0.8 s |
| `Cooldown.Launcher` | `GA_Launcher` | 1.5–2.5 s |
| `Cooldown.Ultimate` | (P2) | — |
| `Cost.Stamina` | custo genérico | — |
| `Cost.Stamina.Dash` | dash com stamina (P1) | — |

---

## 5. `Eco.*` — upgrades de run

### Famílias ([40](../design/40_Eco_Pool_Catalog.md))

| Tag | Família |
|---|---|
| `Eco.Family.Tempest` | Tempestade (aéreo) |
| `Eco.Family.Carrion` | Carniça (sustain) |
| `Eco.Family.Fury` | Fúria (dano/risco) |
| `Eco.Family.Void` | Vazio (controle) |

### IDs de Eco (exemplos — lista completa no [40](../design/40_Eco_Pool_Catalog.md))

```
Eco.Tempest.JugglePlus
Eco.Tempest.JumpReset
Eco.Tempest.SlamLightning
Eco.Tempest.PopBoost
Eco.Tempest.ChainLaunch
Eco.Carrion.AirHeal
Eco.Carrion.SlamHeal
Eco.Carrion.Lifesteal3
Eco.Carrion.AirKillDrop
Eco.Fury.LowHP
Eco.Fury.DashStrike
Eco.Fury.CritHitstop
Eco.Fury.GlassCannon
Eco.Fury.AttackUp
Eco.Void.SlamStun
Eco.Void.PoiseUp
Eco.Void.LauncherVacuum
Eco.Void.SlowWorld
```

### Sinergias

| Tag | Combinação |
|---|---|
| `Eco.Synergy.Hurricane` | JugglePlus + JumpReset |
| `Eco.Synergy.AerialLeech` | AirHeal + JugglePlus |
| `Eco.Synergy.ThunderFinale` | SlamLightning + SlamStun |
| `Eco.Synergy.FragileVampire` | LowHP + Lifesteal3 |

### Custo / tensão

| Tag | Uso |
|---|---|
| `Eco.Cost.Glass` | Glass Cannon (-HP) |
| `Eco.Active.*` | tag concedida enquanto Eco está ativo na run (opcional) |

---

## 6. `Meta.*` — progressão permanente

| Tag | Uso |
|---|---|
| `Meta.Upgrade.Health` | +vida inicial (hub) |
| `Meta.Upgrade.EcoSlot` | +1 opção de Eco |
| `Meta.Unlock.Family.Void` | destrava família Vazio |
| `Meta.Unlock.Eco.SlamLightning` | destrava Eco específico |

---

## 7. `Event.*` — GameplayEvents (notify → GAS)

| Tag | Disparado por | Consumido por |
|---|---|---|
| `Event.Combat.Hit` | `ANS_Hitbox` | pipeline dano + combo ([15](../combat/15_Combat_Overview.md)) |
| `Event.Combat.Launch` | notify launcher | `GA_Launcher` |
| `Event.Combat.SlamDown` | notify slam | `GA_AirSlam` |
| `Event.Combat.SlamLand` | impacto chão | AoE + hard land |
| `Event.Combat.PoiseBreak` | poise ≤ 0 | stagger |

---

## 8. `GameplayCue.*` — juice (resumo)

Catálogo completo em [21 §10](../feel/21_Juice_FX.md). Principais:

```
GameplayCue.Hit.Light
GameplayCue.Hit.Heavy
GameplayCue.Hit.Air
GameplayCue.Launch
GameplayCue.Slam
GameplayCue.Dodge.Perfect
GameplayCue.Land.Hard
GameplayCue.Proj.Fire
GameplayCue.Proj.Impact
GameplayCue.Death.Enemy
```

---

## 9. Slots GAS — onde cada tag vai

| Slot UE | Famílias permitidas | Erro comum |
|---|---|---|
| `AbilityTags` | `Ability.*` | colocar `State.*` aqui |
| `ActivationOwnedTags` | `State.*` (enquanto roda) | colocar `Cooldown.*` |
| `ActivationRequiredTags` | `State.*` necessárias | esquecer `InAir` no air attack |
| `ActivationBlockedTags` | `State.Dead`, `Cooldown.*` | — |
| `CancelAbilitiesWithTag` | `Ability.*` alheias | cancelar `State.*` (não funciona) |

> 📋 Fichas por ability: [19 §2–§3](../combat/19_Abilities_Deep.md).

---

## 9b. Console vars (`ddr.*`) — catálogo

Registro único dos **console vars de debug/tuning** (não são tags, mas são identificadores que apareciam espalhados):

| cvar | Uso | Doc |
|---|---|---|
| `ddr.LocomotionDebug` | HUD de Speed/Gait/Direction/estado | [08 §5](../locomotion/08_Locomotion_Overview.md) |
| `ddr.JumpDebug` | airtime / apex / landing | [13](../locomotion/13_Jump_Fall_Landing.md) |
| `ddr.CombatDebug` | estado / janelas de cancelamento / buffer | [15 §6](../combat/15_Combat_Overview.md) |
| `ddr.JuggleDebug` | altura / PopHeight / decay / hits do juggle | [16 §3](../combat/16_Aerial_Combos.md) |
| `ddr.HitStopScale` | escala de hit-stop (tuning **e acessibilidade**) | [21 §3](../feel/21_Juice_FX.md), [27 §2](../ui/27_Settings.md) |
| `ddr.ShakeScale` | escala de screen-shake (tuning + acessibilidade) | [21](../feel/21_Juice_FX.md), [27 §2](../ui/27_Settings.md) |
| `ddr.ShowLocalizationKeys` | mostra chave i18n em vez do texto | [38 §11](38_Localization.md) |

> 🔗 `ddr.HitStopScale` / `ddr.ShakeScale` são **expostos ao jogador** como opções de acessibilidade no [27 §2](../ui/27_Settings.md).

---

## 10. Registro no projeto

### Opção A — `DefaultGameplayTags.ini` (recomendado MVP)

```ini
[/Script/GameplayTags.GameplayTagsList]
+GameplayTagList=(Tag="State.Combat.InAir",DevComment="Player no ar")
+GameplayTagList=(Tag="Ability.Attack.Light",DevComment="Combo chao")
; ... todas as tags deste doc
```

### Opção B — Native tags (C++)

```cpp
// UDDRGameplayTags.h — UE 5.4 style
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Combat_InAir);
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_InAir, "State.Combat.InAir");
```

> Use **uma** fonte de verdade. DataAssets referenciam tags pelo editor (dropdown) — nunca digite string à mão.

---

## 11. Checklist

- [ ] Todas as tags deste doc em `DefaultGameplayTags.ini`
- [ ] Nenhuma string mágica `"State.Combat.InAir"` no código
- [ ] Abilities usam slots corretos ([19 §2](../combat/19_Abilities_Deep.md))
- [ ] AnimBP lê `State.*` (não `Ability.*`)
- [ ] BT inimigo pausa em `State.Combat.Airborne`
- [ ] Ecos usam `Eco.*` em `UDDREcoData`
- [ ] Sinergias checam `Eco.Synergy.*` + Eco ativos na run

---

## 12. Próximo passo

→ [05 — GAS](05_GAS_Architecture.md) · [44 — Data-Driven](44_Data_Driven.md) · [40 — Pool de Ecos](../design/40_Eco_Pool_Catalog.md).
