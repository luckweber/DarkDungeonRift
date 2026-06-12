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
- [ ] Arena: spawn de 1-2 ondas + token de ataque (§8)
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
| Tags `Faction.Enemy` · `State.Combat.Airborne` | já declaradas e aplicadas pelo juggle | ✅ |
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

Mesma receita do `M_BlobShadow` ([60 §7](60_M2_Editor_Setup.md)):

1. **`M_Telegraph`**: domínio **Deferred Decal**, blend Translucent, **vermelho pulsante** (Sine no Time → Opacity 0.3-0.6). Variações: cone (máscara radial angular) e **linha** (retângulo esticado, pro Atirador).
2. Os cues das abilities aplicam: `GameplayCue.Enemy.Telegraph` (decal cone no grunt) e `GameplayCue.Proj.Telegraph` (linha no archer) — crie `GameplayCueNotify_Burst`/`Actor` BPs com essas tags que spawnam o decal pela duração do windup.
3. **Teste de leitura:** em PIE, você deve saber **onde NÃO estar** olhando só pro chão. "Morri sem ver" = telegrafe reprovado ([32 §9](../enemies/32_Enemy_Combat_Behavior.md)).

---

## 8. Arena: ondas + token (escopo M3 = 1 arena, 1-2 ondas)

1. Coloque 4-8 **Target Points** (spawn points) na arena.
2. Encounter manager mínimo (BP no level ou o futuro `ADDREncounterManager`): onda 1 = 3 grunts → ao limpar → onda 2 = 2 grunts + 1 archer → portas abrem ([roadmap §5](../17_Implementation_Roadmap.md), detalhe completo no [33](../enemies/33_Spawning_Encounters.md) pro M4).
3. **Token de ataque = 2**: o manager concede `bHasAttackToken` a no máx. 2 inimigos; devolvido no fim da ability. Cercado ≠ atacado por todos ([32 §5](../enemies/32_Enemy_Combat_Behavior.md)).

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
| Massacrado quando cercado | sem token de ataque | §8 — máx. 2 |
| Inimigo acerta outro inimigo | filtro de facção pendente no `CanHitActor` | §1 — C++ |
| Launcher não lança o inimigo | Poise ainda não quebrou (by design no elite) — no grunt: poise > 20? | [51 §2](../enemies/51_Enemy_Catalog_MVP.md) |
| Inimigo cai DURO de pé no slam | mesh sem Physics Asset | [60 §4.1](60_M2_Editor_Setup.md) |
| Tiro invisível / injusto | sem tracer/linha | §6-7 |
| Archer atira colado em você | sem branch de kite | §4.2 |
| FPS cai com a horda | sem pooling de projétil / IA sem LOD | [37 §6](../combat/37_Projectiles.md) · [30 §7](../enemies/30_AI_Overview.md) |

---

## 11. Próximo passo

→ **M4 — Run:** salas encadeadas + mini-boss + Ecos ([42](42_Run_Room_Manager.md), [34](../enemies/34_MiniBoss.md), [40](../design/40_Eco_Pool_Catalog.md)). O Brutamontes (elite de poise 120) também entra no M4 ([51 §1](../enemies/51_Enemy_Catalog_MVP.md)).
