# 05 — Arquitetura GAS · 🟢 P0

> Como o **Gameplay Ability System** estrutura combate, stats, habilidades e morte no Dark Dungeon Rift. GAS é o **backbone** do jogo — combos, dash, launcher, upgrades de run e meta, todos passam por aqui.

> **Por que GAS e não código solto?** Porque o jogo é data-driven e roguelike: upgrades concedem/modificam habilidades e effects em runtime. GAS faz isso de fábrica (e já vem networked, se um dia quiser co-op).

---

## 1. As 5 peças do GAS (mapa mental)

| Peça | Classe UE | No nosso jogo |
|---|---|---|
| **ASC** — Ability System Component | `UAbilitySystemComponent` | Cérebro: roda abilities, aplica effects, segura tags. Vive no player e em cada inimigo. |
| **Attributes** — AttributeSet | `UAttributeSet` | Os números: HP, Stamina, AttackPower, MoveSpeed... |
| **Abilities** — GameplayAbility | `UGameplayAbility` | As ações: atacar, dash, launcher, slam, habilidades de upgrade. |
| **Effects** — GameplayEffect | `UGameplayEffect` | Mudanças de número/estado: dano, cura, buff, cooldown, upgrades de run. |
| **Tags** — GameplayTag | `FGameplayTag` | Estado e regras: `State.Combat.InAir`, `Cooldown.Dash`, bloqueios. |

```
Input → ativa Ability → aplica Effect → muda Attribute → dispara GameplayCue (VFX/SFX)
                ↑ gated por Tags ↑
```

---

## 2. Onde o ASC mora (decisão importante)

Duas opções clássicas:

| Modelo | ASC no... | Quando usar |
|---|---|---|
| **A — ASC no Character** | Próprio `ACharacter` | Personagens "burros"/NPCs, jogo sem respawn-sem-troca-de-pawn. **Recomendado p/ MVP.** |
| **B — ASC no PlayerState** | `APlayerState` | Quando o player troca de pawn/respawna mantendo stats, ou multiplayer robusto. |

**Decisão MVP:** **Modelo A (ASC no Character)** — simples, e o roguelike recria o personagem a cada run mesmo. Migrar p/ PlayerState depois é possível se for multiplayer.

```cpp
// ADDRCharacterBase.h
UPROPERTY(VisibleAnywhere, Category="GAS")
TObjectPtr<UDDRAbilitySystemComponent> AbilitySystemComponent;

UPROPERTY()
TObjectPtr<UDDRAttributeSet> AttributeSet;

virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
{ return AbilitySystemComponent; }   // implementa IAbilitySystemInterface
```

> Implemente `IAbilitySystemInterface` na classe base — muita coisa do GAS procura por essa interface.

---

## 3. AttributeSet — os números do jogo

Crie `UDDRAttributeSet`. Atributos MVP:

| Atributo | Uso |
|---|---|
| **Health / MaxHealth** | Vida. 0 → morte. |
| **Stamina / MaxStamina** | Custo de dash/sprint (opcional no MVP — pode começar sem). |
| **AttackPower** | Multiplicador de dano dos combos. |
| **MoveSpeed** | Alimenta o CMC (upgrades de velocidade). |
| **(Run-scoped)** CritChance, Lifesteal... | Adicione conforme upgrades pedirem. |

Padrão de declaração (macro `ATTRIBUTE_ACCESSORS`):

```cpp
UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Vitals")
FGameplayAttributeData Health;
ATTRIBUTE_ACCESSORS(UDDRAttributeSet, Health)
```

**Clamp e reações** ficam em `PreAttributeChange` (clamp) e `PostGameplayEffectExecute` (ex.: detectar Health ≤ 0 → aplicar tag `State.Dead` + disparar morte).

> 🎯 **Dano não é atributo permanente.** Use um *meta attribute* `IncomingDamage` (transiente): o effect de dano escreve nele, e em `PostGameplayEffectExecute` você faz `Health -= IncomingDamage`. Isso permite mitigação/armor no meio.

### 3.1 Atributos de combate & roguelike (registro canônico) {#3-1}

Além dos vitais, declare os atributos que o **combate** e os **Ecos** ([40](../design/40_Eco_Pool_Catalog.md)) leem/modificam. **Este AttributeSet é a fonte da verdade** — os Ecos referenciam estes atributos, não os redefinem:

| Atributo | Default | Lido / modificado por |
|---|---|---|
| **MaxJuggleHits** | **7** | cap do juggle em `GA_AirAttack` ([16 §3](../combat/16_Aerial_Combos.md)); Eco *JugglePlus* +1 |
| **PopHeightMult** | 1.0 | multiplica o re-float aéreo (base ~150 cm é constante de combate, [16 §3](../combat/16_Aerial_Combos.md)); Eco *PopBoost* +0.15 |
| **PoiseDamageMult** | 1.0 | dano de poise ([18 §5](../combat/18_Combat_System_Deep.md)); Eco *PoiseUp* |
| **BonusHitStopFrames** | 0 | hit-stop ([21 §3](../feel/21_Juice_FX.md)); Eco *CritHitstop* |
| **Gold** *(run-scoped)* | 0 | economia in-run / loja ([47](../design/47_Room_Types_Routing.md)) |

> 🔑 **Contrato:** todo Eco que "muda um número do combate" mexe num **destes atributos** via `GE` ([40 §3](../design/40_Eco_Pool_Catalog.md)) — nunca num valor hardcoded. Adicionar um eixo de Eco = **+1 atributo aqui**.

---

## 4. Abilities — as ações do jogo

Crie uma base `UDDRGameplayAbility` (configura policy de instancing, tags padrão). Abilities MVP:

| Ability | Tag | O que faz | Doc |
|---|---|---|---|
| **GA_Attack_Light** | `Ability.Attack.Light` | Combo de chão (avança seção da montage) | [15](../combat/15_Combat_Overview.md) |
| **GA_Launcher** | `Ability.Attack.Launcher` | Lança alvo (e player) pro ar | [16](../combat/16_Aerial_Combos.md) |
| **GA_AirAttack** | `Ability.Attack.Air` | Ataque aéreo (só com tag `State.Combat.InAir`) | [16](../combat/16_Aerial_Combos.md) |
| **GA_AirSlam** | `Ability.Attack.AirSlam` | Finisher descendente | [16](../combat/16_Aerial_Combos.md) |
| **GA_Dash** | `Ability.Dash` | Dash com i-frames + cooldown | [09](../locomotion/09_Gaits.md) |

Anatomia de uma ability de ataque:

```
ActivateAbility:
  1. CommitAbility (checa custo/cooldown)
  2. PlayMontageAndWait (toca AM_Combo)
  3. WaitGameplayEvent "Combat.Hit" (do AnimNotify de hitbox)
       → on event: aplica GE_Damage no alvo via MotionWarp/hit detection
  4. on montage end/interrupt → EndAbility
```

**Conceitos-chave usados:**
- **Ability Tasks** (`UAbilityTask_PlayMontageAndWait`, `WaitGameplayEvent`, `WaitInputPress`) — o "async" do GAS.
- **Tags de bloqueio/cancelamento** — `ActivationBlockedTags` (ex.: não atacar se `State.Dead`), `CancelAbilitiesWithTag`.
- **Input ID** — liga a tecla à ability (ver doc 07).

---

## 5. Effects — mudanças de estado

| GameplayEffect | Duração | Uso |
|---|---|---|
| **GE_Damage** | Instant | Aplica `IncomingDamage` (escala por AttackPower) |
| **GE_Cooldown_Dash** | Duration | Concede tag `Cooldown.Dash` por X s |
| **GE_Heal** | Instant | Cura (recompensa "combo aéreo cura") |
| **GE_RunUpgrade_Damage** | Infinite | "+15% dano" — buff permanente **da run** |
| **GE_Dead** | Infinite | Tag `State.Dead`, zera input |

### ⚠️ Conceder tags num GE (padrão UE 5.3+ — não use a API deprecada)

`InheritableOwnedTagsContainer` está **deprecado** (warning C4996; quebra em engine futura). Todo GE C++ que **concede tag ao alvo** (`GE_Cooldown_*`, `GE_DashIFrames`, `GE_Dead`…) usa o **GE Component**:

```cpp
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

// no PostInitProperties do GE (NÃO no construtor — FindOrAddComponent chama NewObject e crasha o CDO ao abrir o editor):
void UGE_DDRCooldownDash::PostInitProperties()
{
    Super::PostInitProperties();
    if (!HasAnyFlags(RF_ClassDefaultObject)) { return; }

    FInheritedTagContainer TagContainer;
    TagContainer.Added.AddTag(DDRTags::Cooldown_Dash);
    UTargetTagsGameplayEffectComponent& TargetTags = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    TargetTags.SetAndApplyTargetTagChanges(TagContainer);
}
```

> ✅ Aplicado em `GE_DDRCooldownDash` e `GE_DDRDashIFrames`. Em GE **assets** (Blueprint), o equivalente é adicionar o componente **"Grant Tags to Target Actor"** no painel Components do GE.

> 🧩 **Upgrades de run = GameplayEffects Infinite + Abilities concedidas.** Pegou "+15% dano"? Aplique um `GE_RunUpgrade_Damage` Infinite no ASC. Pegou "dash causa dano"? Conceda/troque a `GA_Dash`. O [core loop](../design/03_Core_Loop_Roguelike.md) inteiro se expressa em GAS — por isso ele é P0.

---

## 6. Gameplay Tags — estado e regras

Já registradas no [doc 04 §4](04_Project_Setup.md). Usos típicos:

| Tag | Quem seta | Quem lê |
|---|---|---|
| `State.Combat.InAir` | GA_Launcher (no player) | GA_AirAttack (requer), AnimBP (escolhe poses aéreas) |
| `State.Movement.Dashing` | GA_Dash | i-frames (dano ignora), CMC |
| `State.Dead` | AttributeSet (HP≤0) | bloqueia todas abilities, IA |
| `Cooldown.Dash` | GE_Cooldown_Dash | GA_Dash (ActivationBlocked) |

> 🔗 O **AnimInstance lê tags do ASC** pra decidir poses (ex.: tem `State.Combat.InAir` → usa state machine aérea). Isso conecta GAS ↔ locomoção sem acoplar código. Ver [doc 08 §6](../locomotion/08_Locomotion_Overview.md).

---

## 7. GameplayCues — feedback (VFX/SFX)

Cues desacoplam *o que aconteceu* de *como soa/parece*. Família MVP:

- `GameplayCue.Hit.Light` / `.Heavy` / `.Air` — faísca + som + hit-stop, escalando com o peso do golpe.
- `GameplayCue.Launch` / `GameplayCue.Slam` — pop aéreo / impacto-clímax do finisher.
- `GameplayCue.Land.Hard` — poeira + shake no pouso forte.

Dispare via `ASC->ExecuteGameplayCue(Tag, Params)` ou pelo próprio GameplayEffect (campo *Gameplay Cues*).

> 🎆 **Catálogo canônico de GameplayCues + escala de juice por golpe:** [21 — Juice & FX](../feel/21_Juice_FX.md). (O `Hit.Impact` do rascunho inicial foi expandido pra a família `Hit.Light/.Heavy/.Air` lá — use a de lá como fonte da verdade.)

---

## 8. Fluxo de inicialização (ordem que importa)

```cpp
// 🔒 Single-player (Modelo A): PLAYER em PossessedBy; NPC em BeginPlay.
AbilitySystemComponent->InitAbilityActorInfo(this /*owner*/, this /*avatar*/);
// → conceder abilities iniciais:
for (auto& AbilityClass : DefaultAbilities)
    ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, InputID));
// → aplicar effect de stats iniciais:
ApplyInitialAttributes(); // GE com HP/Stamina/AttackPower base
```

> ⚠️ **`InitAbilityActorInfo` no momento certo** é o erro nº 1 de quem começa GAS. 🔒 **Single-player (Modelo A):** chame em **`PossessedBy`** (player, ao ser possuído pelo controller) e em **`BeginPlay`** (NPCs spawnados). Sem replicação, **não existe `OnRep_PlayerState`** — esse caminho só seria necessário num futuro co-op. Errar o momento = abilities "não ativam" sem erro claro.

---

## 9. Checklist

- [ ] `UDDRAbilitySystemComponent` + `UDDRAttributeSet` criados
- [ ] Classe base implementa `IAbilitySystemInterface`
- [ ] `InitAbilityActorInfo` chamado no lugar certo (`PossessedBy` player / `BeginPlay` NPC — §8)
- [ ] Atributos MVP (Health/Stamina/AttackPower/MoveSpeed) com clamp
- [ ] **Atributos de combate/Eco** declarados (§3.1: MaxJuggleHits=7, JugglePopHeight, PoiseDamageMult, BonusHitStopFrames)
- [ ] Meta-attribute `IncomingDamage` + lógica em `PostGameplayEffectExecute`
- [ ] Morte detectada (HP≤0 → tag `State.Dead`)
- [ ] Abilities MVP concedidas na init (Attack/Dash; Launcher/Air vêm no M2)
- [ ] GE_Damage escala por AttackPower
- [ ] Tags de estado lidas pelo AnimInstance

---

## 10. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Ability não ativa, sem erro | `InitAbilityActorInfo` não chamado / cedo demais | Chame em PossessedBy + OnRep_PlayerState (§8) |
| Dano não aplica | Effect sem modifier no atributo certo, ou sem `PostGameplayEffectExecute` | Use meta-attribute `IncomingDamage` (§3) |
| Tag não bloqueia ability | Tag em `ActivationOwnedTags` vs `ActivationBlockedTags` trocadas | Bloqueio vai em `ActivationBlockedTags` |
| Cooldown não funciona | GE_Cooldown sem a tag, ou ability não referencia | Ability → `CooldownGameplayEffectClass` |
| Ability não concede / atributo não inicia | `InitAbilityActorInfo` cedo demais ou pawn sem controller | §8 — `PossessedBy` (player) / `BeginPlay` (NPC). *(`ReplicatedUsing`/`REPNOTIFY` só se migrar p/ co-op.)* |

---

## 11. Referências

- [Gameplay Ability System — UE oficial](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine)
- **GASShooter / GASDocumentation** (tranek, GitHub) — referência comunitária canônica de GAS.

---

## 12. Próximo passo

→ [06 — Câmera Topdown](06_Camera_TopDown.md) e [07 — Input](07_Input.md) para fechar os sistemas base.
