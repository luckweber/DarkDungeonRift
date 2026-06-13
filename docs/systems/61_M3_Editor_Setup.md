# 61 — M3 Editor Setup (passo a passo) · 🟡 LUTAR

> **O que fazer no Unreal Editor** para o **M3 — Lutar** (inimigos com IA numa arena). O código entrega a mente e o dano; **você** cria os BPs de inimigo, Behavior Trees, Blackboard, NavMesh, montages de ataque com telegrafe, o projétil do Atirador e a arena com ondas. Formato irmão do [54 (M0)](54_M0_Editor_Setup.md) · [55 (M1)](55_M1_Editor_Setup.md) · [60 (M2)](60_M2_Editor_Setup.md).

> ✅ **STATUS C++:** contrato do **§1 implementado e compilado** (`ADDREnemyCharacter`, `UDDREnemyData`, poise/stagger, IA, abilities inimigo, projétil+pool, encounter manager + token). **Próximo passo:** passos de editor abaixo (§2–§9).

> **Pré-req:** M2 completo ([60](60_M2_Editor_Setup.md)) — juggle/slam divertidos no dummy. **Docs:** [30 IA](../enemies/30_AI_Overview.md) · [31 Arquétipos](../enemies/31_Enemy_Archetypes.md) · [32 Comportamento](../enemies/32_Enemy_Combat_Behavior.md) · [37 Projéteis](../combat/37_Projectiles.md) · [51 Catálogo MVP](../enemies/51_Enemy_Catalog_MVP.md) · [17 Roadmap §5](../17_Implementation_Roadmap.md)

---

## 0. Checklist rápido

- [x] C++ do M3 compilado (§1)
- [ ] NavMesh na arena (§2)
- [ ] `BB_DDREnemy` + `BT_Grunt_Melee` com **pausa no Airborne** (§3-4)
- [ ] `BP_Enemy_Grunt` + `DA_Enemy_Grunt_Melee` + `AM_Enemy_Slash` com telegrafe (§5)
- [ ] `BP_Enemy_Archer` + projétil + **linha de telegrafe** (§6)
- [ ] Material de telegrafe no chão (§7) — a leitura topdown!
- [ ] Arena: `BP_DDREncounterManager` + spawn points tag `EnemySpawn` + `StartEncounter` (§8)
- [ ] PIE: limpar 4-8 inimigos usando chão + aéreo (§9)

---

## 1. Contrato C++ do M3 (implementar antes do editor)

| Classe | O que faz | Status |
|---|---|---|
| `ADDREnemyCharacter : ADDRCharacterBase` | base de inimigo: lê `UDDREnemyData` no spawn (stats → AttributeSet, mesh, BT, abilities), AIControllerClass default | ✅ |
| `UDDREnemyData : UPrimaryDataAsset` | a planilha do inimigo ([51 §3](../enemies/51_Enemy_Catalog_MVP.md)): Health/PoiseMax/MoveSpeed/Mass/Armor, EssenceDrop, Abilities, BehaviorTree, Telegraph, Mesh | ✅ |
| **Poise no `UDDRAttributeSet`** | `Poise`/`PoiseMax` + regen com delay; quebra → `State.Combat.Stagger` (porta do launch, [18 §5](../combat/18_Combat_System_Deep.md)) | ✅ |
| `ADDREnemyAIController` | possui o pawn, roda o BT, aggro por raio/arena ([30 §5](../enemies/30_AI_Overview.md)); seta `TargetActor` no Blackboard | ✅ |
| `UBTTask_DDRActivateAbility` | task de BT que ativa uma `GameplayAbility` por tag e espera terminar (ataque = ability, simétrico ao player) | ✅ |
| `GA_Enemy_Melee_Slash` | windup (telegrafe decal+cue) → montage com `ANS_DDRHitbox` → recovery vulnerável ([32 §2](../enemies/32_Enemy_Combat_Behavior.md)) | ✅ |
| `GA_Enemy_Ranged_Bolt` | windup (linha no chão + tell) → spawna projétil no socket `muzzle` → recovery ([37 §2](../combat/37_Projectiles.md)) | ✅ |
| `ADDRProjectileBase` + `UDDRProjectileData` + pool | um actor, muitos dados; PMC + overlap → `GE_DDRDamage`; pooling 16-32 ([37 §1/§4/§6](../combat/37_Projectiles.md)) | ✅ |
| **Token de ataque** | contador no Encounter/Arena manager: só **2** inimigos em *Attack* por vez ([32 §5](../enemies/32_Enemy_Combat_Behavior.md)); BT pede/devolve | ✅ |
| **Filtro de facção** no `CanHitActor` | inimigo não acerta inimigo (`Faction.Enemy`); player vs dummy (sem tag) continua M2 | ✅ |
| Juggle/launch API (`StartAirborne`/pop/slam) | inimigo herda da base — **o juggle do dummy funciona igual** | ✅ M2 |
| **Ragdoll knockdown** (`bRagdollOnSlammed`) | herda da base; precisa de **Physics Asset** na mesh ([60 §4.1](60_M2_Editor_Setup.md)) | ✅ M2 |
| `ANS_DDRHitbox` / dano via `GE_DDRDamage` | os notifies e o pipeline servem pros DOIS lados | ✅ M1/M2 |
| Tags `Faction.Enemy` · `Enemy.*` (doc 51) · `State.Combat.Airborne` | nativas em `DDRTags` · aplicadas em `ApplyEnemyData` | ✅ |
| Build.cs | `AIModule`, `NavigationSystem`, `Niagara` (BT/Blackboard/NavMesh) | ✅ |

> 🧠 **Decisões já tomadas (não rediscutir):** Behavior Tree, não State Tree ([30 §2](../enemies/30_AI_Overview.md)) · inimigo é GAS igual ao player ([30 §1](../enemies/30_AI_Overview.md)) · data-driven: novo inimigo = novo DataAsset, **não** nova classe ([31 §1](../enemies/31_Enemy_Archetypes.md)).
>
> ⚙️ **Nota da base:** o dummy precisou de `bRunPhysicsWithNoController=true` porque não tem Controller. Inimigos do M3 **têm AIController** — não precisam do flag (a queda do juggle/slam funciona nativamente).

---

## 2. NavMesh na arena

1. No `L_DDR_M0_Test` (ou na arena nova do [52](../world/52_Arena_Level_Design.md)): **Place Actors → Volumes → Nav Mesh Bounds Volume** cobrindo o chão jogável inteiro (estique o brush).
2. Aperte **P** no viewport — o chão deve pintar de **verde**. Sem verde = IA não anda.
3. Settings default do `RecastNavMesh` servem no MVP. Buracos no verde → suba `Cell Size` ou confira colisão dos props.

---

## 3. Blackboard — `BB_DDREnemy`

Content Browser → `Content/DarkDungeonRift/AI/` → **Add → Artificial Intelligence → Blackboard**.

| Key | Tipo | Uso |
|---|---|---|
| `TargetActor` | Object (Actor) | o player — o AIController seta no aggro |
| `AIState` | Enum (Idle/Chase/Attack/Reposition) | debug/transições ([30 §3](../enemies/30_AI_Overview.md)) |
| `bInRange` | Bool | alcance de ataque (service/controller atualiza) |
| `bHasAttackToken` | Bool | recebeu o token? ([32 §5](../enemies/32_Enemy_Combat_Behavior.md)) |
| `DesiredRange` | Float | só ranged: 600-900 ([37 §7](../combat/37_Projectiles.md)) |
| `bPlayerTooClose` | Bool | só ranged: kite |

---

## 4. Behavior Tree — `BT_Grunt_Melee` (nó a nó)

**Add → Artificial Intelligence → Behavior Tree** → assigne `BB_DDREnemy`.

```
ROOT
└── Selector  ⟵ decorator: Gameplay Tag Condition (a REGRA Nº1 — §4.1)
    ├── Sequence "Atacar"
    │     • decorator: Blackboard (bInRange == true)
    │     • decorator: Blackboard (bHasAttackToken == true)
    │     └── BTTask_DDRActivateAbility (GA_Enemy_Melee_Slash)
    ├── Sequence "Perseguir"
    │     • decorator: Blackboard (TargetActor is set)
    │     └── Move To (TargetActor, Acceptable Radius ~150)
    └── Wait (0.5-1.0s) "Idle/Reposicionar"
```

### 4.1 🔥 A pausa no Airborne (sem isto o pilar quebra)

No **Selector raiz** → Add Decorator → **Gameplay Tag Condition**:
- **Actor to check:** Self · **Tags to Match:** `Any` · **Gameplay Tags:** `State.Combat.Airborne`, `State.Combat.Stagger`
- ⚠️ **INVERTA**: a árvore só roda se **NÃO** tiver as tags (no decorator, condição "None"/invertida conforme a versão da UI).
- **Observer aborts: Both** — no instante em que o launcher acerta, a IA **congela** e o inimigo vira o boneco do juggle ([30 §6](../enemies/30_AI_Overview.md)). A tag sai (pós-slam/getup) → BT retoma sozinho.

### 4.2 `BT_Ranged_Archer` (variação)

Copie o BT do grunt e troque o miolo: "Perseguir" vira **manter `DesiredRange`** (Move To um ponto a 600-900 do player) + branch **kite** (`bPlayerTooClose` → Move To afastando) + ataque = `GA_Enemy_Ranged_Bolt`. Detalhe em [37 §7](../combat/37_Projectiles.md).

---

## 5. O Grunt — `BP_Enemy_Grunt` (a cobaia do pilar)

1. Blueprint Class → parent **`DDREnemyCharacter`** → `BP_Enemy_Grunt` em `Content/DarkDungeonRift/Characters/Enemies/`.
2. **Mesh:** skeletal (mannequin serve no MVP) — **com Physics Asset** assignado (ragdoll do slam, [60 §4.1](60_M2_Editor_Setup.md)). Materiais escuros ≠ player.
3. **AIControllerClass** = `DDREnemyAIController` · **Auto Possess AI** = Placed in World or Spawned.
4. **Sombra blob** (decal do [60 §7](60_M2_Editor_Setup.md)) — leitura de altura no juggle, igual ao dummy.
5. **`DA_Enemy_Grunt_Melee`** (Misc → Data Asset → `DDREnemyData`), valores do [51 §2](../enemies/51_Enemy_Catalog_MVP.md): Health **80** · PoiseMax **20** · Regen 25/s (delay 1.0s) · MoveSpeed **420** · Mass 1.0 · Essência 2 · Abilities = `GA_Enemy_Melee_Slash` · BT = `BT_Grunt_Melee`. Assigne o DataAsset no BP.
6. **`AM_Enemy_Slash`** (montage do golpe — qualquer clip de attack do pack serve no MVP):
   - **`ANS_DDRHitbox`** no swing: Damage **12** · HitStop **2** · sweep frontal (o inimigo não tem arma com sockets — fallback frontal já funciona).
   - **SEM** `ANS_DDRComboWindow` (inimigo não combina).
   - Windup ~**0.45s** ANTES do hitbox = a janela de leitura ([32 §1](../enemies/32_Enemy_Combat_Behavior.md)). O decal de telegrafe (§7) liga nesse trecho (cue da ability).
7. **Reações** (o canvas do pilar, [31 §5](../enemies/31_Enemy_Archetypes.md)): sistema completo no **[63 — Reações](../combat/63_Hit_Reactions_Knockdown.md)** ✅ — flinch 4-way (leve/pesado/aéreo) automático no pipeline de dano + **knockdown ANIMADO do slam → get-up** (IA pausada via Stagger). Crie as 4 montages do [63 §4](../combat/63_Hit_Reactions_Knockdown.md) e assigne nos BPs de inimigo.

---

## 6. O Atirador — `BP_Enemy_Archer`

1. `BP_Enemy_Archer` (mesmo parent) + **`DA_Enemy_Ranged_Archer`**: Health **60** · Poise **40** · MoveSpeed **380** · BT = `BT_Ranged_Archer` · Abilities = `GA_Enemy_Ranged_Bolt`.
2. Socket **`muzzle`** no skeleton (mão direita), origem do tiro.
3. **`BP_Projectile_Bolt`** (child de `DDRProjectileBase`) + **`DA_Projectile_ArcherBolt`**: Speed **2400** · Lifetime **3s** · HitRadius **12** · Damage **15** · gravity **0** (voo horizontal no MVP, [37 §3](../combat/37_Projectiles.md)).
4. **Canal de colisão `Projectile`** (Project Settings → Collision → New Object Channel): Block WorldStatic, Overlap Pawn, Ignore o instigator.
5. **Tracer**: Niagara luminoso no plano horizontal (sem tracer o tiro é invisível em -55°, [37 §5](../combat/37_Projectiles.md)).
6. **Telegrafe de linha** (P0!): durante o windup (~0.55s), linha vermelha pulsante atirador→alvo no chão + zona de impacto + tell sonoro. Gancho: cue `GameplayCue.Proj.Telegraph` (§7).

---

## 7. Telegrafe no chão (o item nº1 da justiça topdown)

> 🛠️ **Passo a passo completo (materiais, BP do decal, GameplayCues, valores): [64 — M3 Decals](64_M3_Decals_Telegraph_Setup.md).** Abaixo, o resumo:

1. **`M_Telegraph`**: domínio **Deferred Decal**, blend Translucent, **vermelho pulsante** (Sine no Time → Opacity 0.3-0.6). Variações: cone (máscara radial angular) e **linha** (retângulo esticado, pro Atirador).
2. Os cues das abilities aplicam: `GameplayCue.Enemy.Telegraph` (decal cone no grunt) e `GameplayCue.Proj.Telegraph` (linha no archer) — crie `GameplayCueNotify_Burst`/`Actor` BPs com essas tags que spawnam o decal pela duração do windup.
3. **Teste de leitura:** em PIE, você deve saber **onde NÃO estar** olhando só pro chão. "Morri sem ver" = telegrafe reprovado ([32 §9](../enemies/32_Enemy_Combat_Behavior.md)).

---

## 8. Arena: ondas + token (escopo M3 = 1 arena, 1-2 ondas)

> **Classe C++:** `ADDREncounterManager` · **BP no level:** `BP_DDREncounterManager` (child recomendado). Detalhe de design longo prazo: [33 — Spawning](../enemies/33_Spawning_Encounters.md) · arenas: [52](../world/52_Arena_Level_Design.md).

### 8.0 Como o C++ orquestra (não rediscutir)

```
BP_DDREncounterManager no level
  │
  ├─ BeginPlay → varre o mapa por TargetPoints com tag "EnemySpawn"
  │
  ├─ StartEncounter()  ← VOCÊ dispara (NÃO é automático no BeginPlay nativo)
  │     └─ StartWave(0) → spawna inimigos nos spawn points
  │
  ├─ Inimigo morre (tag State.Dead no ASC) → AliveEnemyCount--
  │     └─ se 0 → StartWave(CurrentWaveIndex + 1) automaticamente
  │
  └─ Token (MaxConcurrentAttackTokens, default 2)
        └─ DDREnemyAIController::UpdateBlackboard
              → Encounter->TryGrantAttackToken se bInRange
              → BB bHasAttackToken = true/false
              → morte / Airborne / Stagger → token devolvido
```

> ⚠️ **`StartEncounter()` não roda sozinho.** Sem chamar essa função, a arena fica vazia mesmo com Waves configuradas no BP.

### 8.1 Spawn points na arena

1. Level de teste: `L_DDR_M3_Test` (ou arena do [52](../world/52_Arena_Level_Design.md)).
2. **Place Actors → Target Point** — coloque **4–8** pontos espalhados (longe de paredes/cantos).
3. Em **cada** Target Point → **Tags** → adicione exatamente **`EnemySpawn`** (o C++ só registra pontos com essa tag).
4. **NavMesh** (§2): **P** no viewport → chão verde. Sem verde a IA não persegue.

> O spawn point é escolhido **aleatoriamente** entre os tagueados (`PickSpawnPoint`). Spawn por tag nomeada (`Spawn_N`) = P1/M4 ([33 §2](../enemies/33_Spawning_Encounters.md)).

### 8.2 Criar `BP_DDREncounterManager`

1. **Add → Blueprint Class** → parent **`DDREncounterManager`** → `Content/DarkDungeonRift/Game/` → `BP_DDREncounterManager`.
2. Arraste **uma instância** para o level (posição irrelevante — só organização; spawns vêm dos Target Points).
3. **Class Defaults** → categoria **DDR | Encounter**:

| Campo | Valor M3 | Notas |
|---|---|---|
| **Default Enemy Class** | **`BP_Enemy_Grunt`** | Não use `DDREnemyCharacter` cru — o BP traz arma, reações, sombra blob |
| **Max Concurrent Attack Tokens** | **2** | Anti-overwhelm ([32 §5](../enemies/32_Enemy_Combat_Behavior.md)) |
| **Waves** | array 1–2 elementos | ver §8.3 |

### 8.3 Configurar ondas (1–2 no M3)

Cada elemento de **Waves** = uma onda. Campos por onda (`FDDRWave`):

| Campo | Uso |
|---|---|
| **Spawns** | lista `{ Enemy Data, Count }` — qual DataAsset e quantos |
| **Delay Before Seconds** | pausa **antes** de spawnar esta onda (0 na onda 1; 2–3s na onda 2) |
| **Max Alive** | teto de spawns **desta onda** (≥ soma dos Count) |

**Exemplo M3 — só grunts (2 ondas):**

| Waves[Index] | Spawns | Delay Before | Max Alive |
|:---:|:---|---:|---:|
| **[0]** Onda 1 | `DA_Enemy_Grunt_Melee` × **3** | 0 | 6 |
| **[1]** Onda 2 | `DA_Enemy_Grunt_Melee` × **2** | 2.5 | 4 |

**Exemplo M3+ — quando tiver archer (§6):**

| Waves[Index] | Spawns | Delay Before | Max Alive |
|:---:|:---|---:|---:|
| **[0]** | Grunt × 3 | 0 | 6 |
| **[1]** | Grunt × 2 + `DA_Enemy_Ranged_Archer` × 1 | 3 | 4 |

> Ao matar **todos** os vivos da onda atual, a próxima começa sozinha. Log: `[ENCOUNTER] onda N iniciada` → ao fim `[ENCOUNTER] ondas completas`.

### 8.4 Disparar o encontro (`StartEncounter`)

Escolha **uma** opção:

**A — Teste rápido PIE (recomendado agora)**  
No `BP_DDREncounterManager` → **Event Graph**:

```
Event BeginPlay → Start Encounter
```

**B — Produção M3/M4**  
Trigger Box na entrada da arena → **On Actor Begin Overlap** (filtrar player) → `Start Encounter` (bool `bStarted` para rodar só uma vez).

> Portas que travam/abrem ao limpar = **M4** ([33 §2](../enemies/33_Spawning_Encounters.md), [42](../systems/42_Run_Room_Manager.md)). M3 só precisa spawn + token + limpar ondas.

### 8.5 Token no Behavior Tree (obrigatório)

O C++ **concede** o token no Blackboard; o BT **só ataca** se `bHasAttackToken == true`.

No `BT_Grunt_Melee`, sequência **Atacar** precisa de **dois** decorators Blackboard:

- `bInRange == true`
- **`bHasAttackToken == true`**

Sem o segundo, todos atacam juntos e o token não faz efeito.

```
Selector  [decorator §4.1: NÃO Airborne/Stagger/Dead · Observer aborts Both]
├─ Sequence "Atacar"     [bInRange] [bHasAttackToken]
│    └─ BTTask_DDRActivateAbility (GA_Enemy_Melee_Slash)
├─ Sequence "Perseguir"  [TargetActor is set]
│    └─ Move To (Acceptable Radius ~150)
└─ Wait 0.5–1.0s
```

**Fluxo do token:** inimigo entra em alcance → controller pede token ao manager → se pool < 2, `bHasAttackToken=true` → BT ataca → ability termina / morre / entra Airborne → token devolvido.

### 8.6 Checklist §8 antes do PIE

- [ ] 4–8 Target Points com tag **`EnemySpawn`**
- [ ] NavMesh verde (§2)
- [ ] `BP_DDREncounterManager` no level com **Waves** preenchidas
- [ ] **Default Enemy Class** = `BP_Enemy_Grunt`
- [ ] **Max Concurrent Attack Tokens** = 2
- [ ] **`StartEncounter`** chamado (BeginPlay ou trigger)
- [ ] BT **Atacar** exige **`bHasAttackToken`**

### 8.7 Logs esperados

| Log | Significado |
|---|---|
| `[ENCOUNTER] onda 0 iniciada (N spawns)` | onda 1 spawnou |
| `[ENCOUNTER] onda 1 iniciada (N spawns)` | onda 2 após limpar a 1 |
| `[ENCOUNTER] ondas completas` | todas as ondas mortas |
| `[ENCOUNTER] sem TargetPoints tag EnemySpawn` | faltam spawn points tagueados |
| `[ENCOUNTER] sem ondas configuradas` | array Waves vazio |
| `[ENEMY] ... EnemyId=Enemy.Grunt.Melee` | DataAsset aplicado no spawn |

---

## 9. Teste PIE (critério do M3)

| Teste | Esperado |
|---|---|
| Spawnar grunt | persegue (NavMesh) → telegrafa (decal!) → golpe → recovery vulnerável |
| Ficar parado no telegrafe | tomar dano — e **entender por quê** (leu o chão) |
| **E no grunt** | poise 20 quebra rápido → **launcher → juggle → slam funcionam IGUAIS ao dummy** |
| Inimigo no ar | **IA congelada** (não ataca flutuando — §4.1) |
| Slam no grunt | ragdoll + (M3) sem getup animado ainda — levanta pelo recover padrão |
| Archer | mantém distância, **linha no chão** antes do tiro, kite se você cola |
| Tomar tiro | tracer visível veio de algum lugar legível |
| Cercado por 5 | só **2 telegrafam por vez** (token) — pressão sem massacre |
| Limpar onda 1 | onda 2 spawna após delay (§8.3) — log `[ENCOUNTER] onda 1` |
| 2 inimigos lado a lado | golpe de um **não** acerta o outro (filtro de facção) |
| `ddr.CombatDebug 1` | sweeps dos dois lados + logs `[HIT]` simétricos |

**✅ M3 pronto quando:** limpar uma arena de 4-8 inimigos com combos de chão E aéreo é gostoso e tem risco real ([17 §5](../17_Implementation_Roadmap.md)). O combate "ao vivo" supera o boneco.

---

## 10. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Inimigo parado, não persegue | sem NavMesh (P não mostra verde) / `TargetActor` vazio | §2 · aggro do controller |
| Inimigo "ataca" enquanto voa no juggle | decorator do Airborne ausente/sem Observer aborts | §4.1 — **Both** |
| "Morri sem ver o golpe" | telegrafe só na animação | §7 — decal no chão obrigatório |
| Massacrado quando cercado | sem token de ataque / BT sem `bHasAttackToken` | §8.5 — máx. 2 + decorator no BT |
| Arena vazia no PIE | `StartEncounter` nunca chamado | §8.4 — BeginPlay ou trigger |
| Log `sem TargetPoints tag EnemySpawn` | spawn points sem tag | §8.1 — tag **`EnemySpawn`** |
| Todos atacam juntos | BT Atacar sem `bHasAttackToken` | §8.5 |
| Inimigo spawna sem mesh/reações | `Default Enemy Class` = C++ cru | §8.2 — use **`BP_Enemy_Grunt`** |
| Inimigo acerta outro inimigo | filtro de facção | §1 — `Faction.Enemy` |
| Nada spawna no PIE | `StartEncounter` não foi chamado | §8.4 |
| Launcher não lança o inimigo | Poise ainda não quebrou (by design no elite) — no grunt: poise > 20? | [51 §2](../enemies/51_Enemy_Catalog_MVP.md) |
| Inimigo cai DURO de pé no slam | mesh sem Physics Asset | [60 §4.1](60_M2_Editor_Setup.md) |
| Tiro invisível / injusto | sem tracer/linha | §6-7 |
| Archer atira colado em você | sem branch de kite | §4.2 |
| FPS cai com a horda | sem pooling de projétil / IA sem LOD | [37 §6](../combat/37_Projectiles.md) · [30 §7](../enemies/30_AI_Overview.md) |

---

## 11. Próximo passo

→ **M4 — Run:** salas encadeadas + mini-boss + Ecos ([42](42_Run_Room_Manager.md), [34](../enemies/34_MiniBoss.md), [40](../design/40_Eco_Pool_Catalog.md)). O Brutamontes (elite de poise 120) também entra no M4 ([51 §1](../enemies/51_Enemy_Catalog_MVP.md)).
