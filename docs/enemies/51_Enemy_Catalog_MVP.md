# 51 — Catálogo de Inimigos MVP (conteúdo) · 🟢 P0 (M3-M4)

> **O roster completo do MVP:** 3 inimigos comuns + 1 mini-boss, com stats, abilities GAS, frame data e encontros. Materializa o [31 — Arquétipos](31_Enemy_Archetypes.md) em conteúdo jogável — espelho do [40 — Catálogo de Ecos](../design/40_Eco_Pool_Catalog.md). Cada linha vira um **`UDDREnemyData`** + abilities referenciadas ([44 — Data-Driven](../systems/44_Data_Driven.md)).

> 🔑 **Princípio:** inimigo = **canvas do pilar**. Stats e poise calibrados pra ensinar combo de chão → launch → aéreo ([18 §5](../combat/18_Combat_System_Deep.md), [16](../combat/16_Aerial_Combos.md)).

---

## 1. Resumo do roster

| ID | Nome (UI) | Papel | Role | Sala típica | Poise | Launch |
|---|---|---|---|---|:---:|---|
| `Enemy.Grunt.Melee` | **Sombra Rastejante** | trash melee | Grunt | 1-4 | ~20 | fácil (1-2 hits) |
| `Enemy.Ranged.Archer` | **Olho da Fenda** | pressão ranged | Ranged | 2-4 | ~40 | médio (fechar distância) |
| `Enemy.Elite.Brute` | **Sentinela Quebrada** | ensina poise | Elite | 3-4 | ~120 | difícil (combo longo) |
| `Enemy.Boss.Guardian` | **Guardião da Camada** | finale da run | Boss | Boss | ~200* | só em janela ([34](34_MiniBoss.md)) |

> *Boss: poise alto + `Faction.Boss.NoLaunch` fora da janela de vulnerabilidade — ver [34 §4](34_MiniBoss.md).

### Distribuição por milestone

| Milestone | Inimigos | Encontros |
|---|---|---|
| **M3** | Grunt + Atirador | 1 arena, 1-2 ondas |
| **M4** | + Brutamontes | 4 salas embaralhadas + boss |
| **P1 polish** | tuning de poise/telegrafe | escalada por profundidade ([33 §4](33_Spawning_Encounters.md)) |

---

## 2. Tabela mestra — stats (ponto de partida)

Valores assumem player com `AttackPower=100`, golpe light ~10 dano / 20 poise damage ([18 §1](../combat/18_Combat_System_Deep.md)). **Tune no M3** — exponha tudo no DataAsset.

| Stat | Grunt | Atirador | Brutamontes | Guardião (boss) |
|---|---:|---:|---:|---:|
| **Health** | 80 | 60 | 220 | 800 |
| **PoiseMax** | 20 | 40 | 120 | 200 |
| **PoiseRegen/s** | 25 | 20 | 15 | 10 |
| **PoiseRegenDelay** | 1.0s | 1.2s | 1.5s | 2.0s |
| **MoveSpeed** | 420 | 380 | 280 | 320 |
| **Mass** (launch cap) | 1.0 | 0.9 | 2.5 | 3.0 |
| **Armor** | 0 | 0 | 20 | 40 |
| **AttackPower** | 100 | 100 | 120 | 130 |
| **Essência drop** | 2 | 3 | 8 | 50 (fim run) |

> 🎯 **Grunt morre em ~8 lights; Atirador em ~6** — trash morre rápido, ranged é frágil se alcançado. Brutamontes exige ~6 hits de poise antes do stagger (120/20).

---

## 3. Assets data-driven

```
Content/Data/Enemies/
├── DA_Enemy_Grunt_Melee.uasset       → UDDREnemyData
├── DA_Enemy_Ranged_Archer.uasset
├── DA_Enemy_Elite_Brute.uasset
├── DA_Enemy_Boss_Guardian.uasset
├── DA_Projectile_ArcherBolt.uasset   → UDDRProjectileData ([37](../combat/37_Projectiles.md))
└── Encounters/
    ├── DA_Encounter_Arena_Crypt.uasset
    ├── DA_Encounter_Arena_Rift.uasset
    └── ... (parear com [52 — Arenas](../world/52_Arena_Level_Design.md))
```

```cpp
// UDDREnemyData — campos MVP (extends 31 §1)
UCLASS()
class UDDREnemyData : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag EnemyId;           // Enemy.Grunt.Melee
    UPROPERTY(EditDefaultsOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly) EEnemyRole Role;
    UPROPERTY(EditDefaultsOnly) float Health, PoiseMax, MoveSpeed, Mass, Armor;
    UPROPERTY(EditDefaultsOnly) float PoiseRegenPerSec, PoiseRegenDelay;
    UPROPERTY(EditDefaultsOnly) int32 EssenceDrop = 2;
    UPROPERTY(EditDefaultsOnly) TArray<TSubclassOf<UGameplayAbility>> Abilities;
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UBehaviorTree> BehaviorTree;
    UPROPERTY(EditDefaultsOnly) FDDRTelegraphConfig Telegraph;
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<USkeletalMesh> Mesh;
};
```

> 🗂️ **Novo inimigo = novo DataAsset**, não nova classe. Variedade pós-MVP = combinar eixos do [31 §3](31_Enemy_Archetypes.md).

---

## 4. Fichas por inimigo

### 🗡️ Sombra Rastejante — `Enemy.Grunt.Melee`

| Campo | Valor |
|---|---|
| **Papel** | Cobaia do pilar — poise baixo, lança em 1-2 hits |
| **BT** | `BT_Grunt_Melee` — persegue → ataca → recua → reposiciona |
| **Abilities** | `GA_Enemy_Melee_Slash` |
| **Telegrafe** | decal cone curto, vermelho, ~0.45s windup |
| **Token** | compete pelo token de ataque ([33 §5](33_Spawning_Encounters.md)) |

**`GA_Enemy_Melee_Slash` — frame data:**

| Fase | Frames | Notas |
|---|---:|---|
| Windup | 8 (~0.13s) | decal cone 120° · 150cm |
| Active | 4 | sweep melee · 12 dano · 25 poise |
| Recovery | 18 | janela do player — **punir aqui** |

> Sem hyperarmor. Quebra poise em 1 hit → stagger → launcher funciona imediatamente.

---

### 🏹 Olho da Fenda — `Enemy.Ranged.Archer`

| Campo | Valor |
|---|---|
| **Papel** | Força fechar distância antes de combar |
| **BT** | `BT_Ranged_Archer` — mantém 600-900cm → telegrafe → atira → recua |
| **Abilities** | `GA_Enemy_Ranged_Bolt` |
| **Projétil** | `DA_Projectile_ArcherBolt` ([37](../combat/37_Projectiles.md)) |
| **Telegrafe** | **linha vermelha** no chão (trajetória) · ~0.55s |

**`GA_Enemy_Ranged_Bolt` — frame data:**

| Fase | Frames | Notas |
|---|---:|---|
| Windup | 10 | linha decal player→mira · tell sonoro |
| Active | 2 | spawn `ADDProjectileBase` |
| Recovery | 24 | vulnerável longo — recompensa quem dasha e fecha |

**`DA_Projectile_ArcherBolt`:**

| Campo | Valor |
|---|---:|
| Speed | 2200 |
| BaseDamage | 14 |
| PoiseDamage | 15 |
| TelegraphDuration | 0.55s |
| HitRadius | 14 |

> 🚩 **Linha no chão é obrigatória** — sem ela o ranged é injusto em topdown ([32 §1](32_Enemy_Combat_Behavior.md)).

---

### 🛡️ Sentinela Quebrada — `Enemy.Elite.Brute`

| Campo | Valor |
|---|---|
| **Papel** | Ensina **quebrar guarda** antes de lançar |
| **BT** | `BT_Elite_Brute` — lento · investida telegrafada · hyperarmor no smash |
| **Abilities** | `GA_Enemy_Elite_Smash` (hyperarmor) · `GA_Enemy_Elite_Shove` (empurra player) |
| **Telegrafe** | círculo grande · windup ~0.7s |
| **Spawn** | máx 1 por sala no MVP · ~30% sala 3-4 |

**`GA_Enemy_Elite_Smash` — frame data:**

| Fase | Frames | Notas |
|---|---:|---|
| Windup | 14 | decal círculo 250cm · tag `State.Combat.HyperArmor` |
| Active | 6 | 28 dano · 35 poise · AoE |
| Recovery | 20 | janela após hyperarmor cair |

> 🥊 Player **não cancela** o smash com mash — dash ou esperar recovery. Poise 120 → ~6 lights ou combo mixed pra stagger → **aí** lança.

---

### 👑 Guardião da Camada — `Enemy.Boss.Guardian`

| Campo | Valor |
|---|---|
| **Papel** | Finale M4 — testa a build |
| **BT** | `BT_Boss_Guardian` — 2 fases ([34 §2](34_MiniBoss.md)) |
| **Abilities** | `GA_Boss_Charge` · `GA_Boss_AoE` · `GA_Boss_Combo` (fase 2) |
| **Tags** | `Faction.Boss` · `Faction.Boss.NoLaunch` (fora da janela) |
| **HUD** | barra dedicada ([28 §1](../ui/28_HUD.md)) |
| **Arena** | `Arena_Boss_Guardian` ([52](../world/52_Arena_Level_Design.md)) |

**Ataques-assinatura (telegrafe forte):**

| Ability | Telegrafe | Windup | Dano | Notas |
|---|---|---:|---:|---|
| **Charge** | linha reta | 0.8s | 35 | dash telegrafado — esquiva lateral |
| **AoE** | círculo grande | 0.9s | 30 | sair da zona |
| **Combo** (F2) | sequência 3 decals | 0.6s×3 | 20/hit | fase 2 — ritmo mais rápido |

**Janela de vulnerabilidade** ([34 §4](34_MiniBoss.md)):

```
Após Charge ou AoE (recovery) OU poise quebrado em stagger de fase:
  → remove Faction.Boss.NoLaunch por 4s
  → player PODE lançar e combar
  → tag State.Combat.Boss.Vulnerable (HUD pulse)
```

---

## 5. Templates de encontro (ligação com arenas)

Cada `UDDREncounterData` referencia `UDDREnemyData*` + spawn tags. Detalhe de arenas: **[52 — Level Design](../world/52_Arena_Level_Design.md)**.

| Encounter | Arena | Onda 1 | Onda 2 | Depth |
|---|---|---|---|:---:|
| `DA_Encounter_Crypt_Easy` | Crypt_01 | 4× Grunt | — | 1 |
| `DA_Encounter_Rift_Mixed` | Rift_02 | 3× Grunt | 2× Archer | 2 |
| `DA_Encounter_Chamber_Pressure` | Chamber_04 | 2× Grunt + 1× Archer | 2× Grunt | 3 |
| `DA_Encounter_Pit_Elite` | Pit_05 | 2× Grunt | 1× Brute | 3-4 |
| `DA_Encounter_Boss` | Boss_Guardian | 1× Guardião | — | Boss |

```cpp
USTRUCT() struct FDDRSpawnEntry {
    TObjectPtr<UDDREnemyData> Enemy;
    int32 Count = 1;
    FName SpawnPointTag;    // "Spawn_N", "Spawn_E" — marcador na arena
};
```

---

## 6. Escalada por profundidade (curva)

| Depth | HP mult | Poise mult | Mix |
|:---:|---:|---:|---|
| 1 | 1.0× | 1.0× | só Grunt |
| 2 | 1.1× | 1.0× | Grunt + Archer |
| 3 | 1.2× | 1.1× | + Archer · chance Brute |
| 4 | 1.3× | 1.15× | mix completo |
| Boss | 1.0× (fixo) | — | Guardião |

> Aplicar via `GE_EncounterScale` no spawn ou multiplicador no EncounterManager ([33 §4](33_Spawning_Encounters.md)). CurveTable `CT_EnemyScaleByDepth` ([44](../systems/44_Data_Driven.md)).

---

## 7. Reações obrigatórias (todos)

| Reação | Asset / tag | Prioridade |
|---|---|:---:|
| Flinch 4-way | montages Hit_* ([22](../22_Animation_Inventory.md)) | 🟢 |
| Stagger | `ReactType=Stagger` + anim longa | 🟢 |
| SM aérea | `State.Combat.Airborne` + IA pause ([30 §6](30_AI_Overview.md)) | 🟢 |
| Knockdown pós-slam | Hit air large to floor + getup | 🟢 |
| Morte | `GameplayCue.Death.Enemy` | 🟢 |

> Inimigo sem reação = saco de HP sem alma — metade do juice do pilar ([31 §5](31_Enemy_Archetypes.md)).

---

## 8. Checklist

- [ ] 4× `UDDREnemyData` criados (Grunt, Archer, Brute, Boss)
- [ ] Abilities inimigas com `FDDRAttackData` + telegrafe ([32](32_Enemy_Combat_Behavior.md))
- [ ] `DA_Projectile_ArcherBolt` + `GA_Enemy_Ranged_Bolt` ([37](../combat/37_Projectiles.md))
- [ ] BT por arquétipo (Grunt / Archer / Brute / Boss)
- [ ] Poise calibrado: Grunt lança fácil · Brute exige combo · Boss só na janela
- [ ] 5× `UDDREncounterData` pareados com arenas ([52](../world/52_Arena_Level_Design.md))
- [ ] Escalada por depth (CurveTable ou GE no spawn)
- [ ] Reações: flinch + stagger + SM aérea + death cue
- [ ] Token de ataque respeitado no BT ([33 §5](33_Spawning_Encounters.md))

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Combo aéreo trivial | Poise baixo demais no trash | §2 — Grunt ~20, não 100 |
| Elite lança no primeiro hit | Poise do Brute baixo | §2 — PoiseMax ~120 |
| Boss trivialmente combado | Sem `NoLaunch` / sem hyperarmor | [34 §4](34_MiniBoss.md) |
| Ranged parece unfair | Sem linha de telegrafe | §4 Olho da Fenda |
| Todos iguais | Um DataAsset só | §3 — um asset por arquétipo |

---

## 10. Próximo passo

→ [52 — Arena & Level Design](../world/52_Arena_Level_Design.md) (onde spawnar) · [33 — Spawning](33_Spawning_Encounters.md) · [34 — Mini-Boss](34_MiniBoss.md) · [42 — Run Manager](../systems/42_Run_Room_Manager.md).
