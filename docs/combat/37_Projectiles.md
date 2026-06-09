# 37 — Projéteis (Atirador & Ranged) · 🟢 P0 (M3)

> Sistema de projéteis para o **Atirador** (inimigo ranged do M3) e base reutilizável para habilidades ranged do player no futuro. Cobre arquitetura do actor, dados, telegrafe de linha, hit detection, integração GAS, VFX/SFX e **leitura topdown**. Pré-req: [31 — Arquétipos](../enemies/31_Enemy_Archetypes.md), [32 — Combate Inimigo](../enemies/32_Enemy_Combat_Behavior.md), [18 — Combate Profundo](18_Combat_System_Deep.md), [36 — VFX](../feel/36_VFX_Niagara.md), [35 — Áudio](../audio/35_Audio_Music.md).

> 🎯 **Por que este doc existe:** o Atirador é **P0 do M3** ([roadmap §5](../17_Implementation_Roadmap.md)). Sem projétil legível (tracer + tell + som), o ranged vira morte injusta em topdown — o jogador morre sem entender de onde veio o tiro.

> 🔒 **Single-player:** sem replicação de projétil. Tudo roda local; pooling e hit detection simplificam.

---

## 1. Arquitetura — um actor, muitos dados

Não crie uma classe por tipo de projétil. Um **`ADDProjectileBase`** + **`UDDRProjectileData`** (DataAsset) cobre flecha, bolt mágico, orbe de fogo, etc.

```
ADDProjectileBase (AActor)
   • UProjectileMovementComponent (ou sweep manual — §4)
   • USphereComponent (colisão)
   • UNiagaraComponent (tracer, opcional no actor)
   • UDDRProjectileData (runtime, copiado do DataAsset)
   • Pool tag / bPooled

UDDRProjectileData (DataAsset)
   • velocidade, lifetime, raio, dano, poise damage
   • telegrafe (cor, tempo, VFX/SFX)
   • VFX/SFX de spawn/tracer/impacto
   • comportamento (homing, pierce, bounce — P2)
```

```cpp
UCLASS()
class UDDRProjectileData : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) float Speed = 2400.f;
    UPROPERTY(EditDefaultsOnly) float Lifetime = 3.f;
    UPROPERTY(EditDefaultsOnly) float HitRadius = 12.f;
    UPROPERTY(EditDefaultsOnly) float BaseDamage = 15.f;
    UPROPERTY(EditDefaultsOnly) float PoiseDamage = 10.f;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayEffect> DamageEffect;
    UPROPERTY(EditDefaultsOnly) FGameplayTag ImpactCue;       // GameplayCue.Proj.Impact
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UNiagaraSystem> TracerFX;
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UNiagaraSystem> ImpactFX;
    UPROPERTY(EditDefaultsOnly) USoundBase* FireSFX;
    UPROPERTY(EditDefaultsOnly) USoundBase* ImpactSFX;
    UPROPERTY(EditDefaultsOnly) FLinearColor TelegraphColor = FLinearColor::Red;
    UPROPERTY(EditDefaultsOnly) float TelegraphDuration = 0.5f;  // windup antes do tiro
    UPROPERTY(EditDefaultsOnly) bool bPierce = false;
    UPROPERTY(EditDefaultsOnly) int32 MaxPierceCount = 0;
};
```

> 🗂️ **Data-driven = variedade barata.** Novo projétil = novo DataAsset, não nova classe C++.

---

## 2. Fluxo do Atirador (telegrafe → tiro → impacto)

O ataque ranged do inimigo é uma **`GA_Enemy_RangedAttack`** (GameplayAbility), simétrico ao melee ([32 §2](../enemies/32_Enemy_Combat_Behavior.md)):

```
[WINDUP — telegrafe]
   • Decal/linha vermelha no chão (trajetória prevista) — §5
   • Flash no inimigo + tell sonoro ([35 §4](../audio/35_Audio_Music.md))
   • Duração = TelegraphDuration do DataAsset (~0.4-0.6s)
        ↓
[ACTIVE — spawn do projétil]
   • ADDProjectileBase spawna no socket "muzzle" / mão
   • Direção = alvo (player) ou última posição conhecida
   • Tracer Niagara liga ([36 §2](../feel/36_VFX_Niagara.md))
        ↓
[RECOVERY — vulnerável]
   • Inimigo lento; janela do player punir/combar
```

> 🚩 **Sem telegrafe de linha, o ranged é injusto em topdown.** A animação de "mirar" sozinha não basta — o jogador olha o chão ([32 §1](../enemies/32_Enemy_Combat_Behavior.md)).

---

## 3. Spawn & direção

```cpp
// GA_Enemy_RangedAttack — fase Active
ADDProjectileBase* SpawnProjectile(AActor* Owner, AActor* Target,
                                 const UDDRProjectileData* Data)
{
    const FTransform SpawnTM = Owner->GetMesh()->GetSocketTransform("muzzle");
    FVector Dir = (Target->GetActorLocation() - SpawnTM.GetLocation()).GetSafeNormal();
    Dir.Z = 0.f;  // topdown: projéteis no plano horizontal (MVP)
    Dir.Normalize();

    auto* Proj = Pool->Acquire<ADDProjectileBase>(ProjectileClass, SpawnTM);
    Proj->Init(Data, Owner, Dir);
    return Proj;
}
```

| Decisão | MVP | Pós-MVP |
|---|---|---|
| **Plano de voo** | horizontal (Z fixo ou leve arco) | arco parabólico, altura variável |
| **Mira** | última posição do player no windup | predição de movimento (lead target) |
| **Origem** | socket `muzzle` no mesh | spawn em múltiplos pontos (boss) |

> 🎯 **Z=0 ou Z fixo no MVP** simplifica a leitura topdown — o tracer fica no plano que a câmera vê melhor. Arco parabólico é P1 (precisa sombra no chão do projétil).

---

## 4. Hit detection — duas abordagens

| Abordagem | Como | Quando | Veredito DDR |
|---|---|---|---|
| **`UProjectileMovementComponent`** | UE move o actor; `OnComponentHit` / overlap no `USphereComponent` | rápido, padrão UE | 🟢 **MVP** — simples e funciona |
| **Sweep manual por frame** | `SweepSingleByChannel` do ponto anterior → atual | evita tunneling em velocidade extrema | 🟡 se PMC falhar em testes |

### PMC (caminho MVP)

```cpp
// ADDProjectileBase::BeginPlay
Sphere->SetSphereRadius(Data->HitRadius);
Sphere->SetCollisionProfileName("Projectile");  // doc 04 §5 — canal próprio
Sphere->OnComponentBeginOverlap.AddDynamic(this, &ADDProjectileBase::OnOverlap);

Movement->InitialSpeed = Data->Speed;
Movement->MaxSpeed = Data->Speed;
Movement->Velocity = LaunchDirection * Data->Speed;
Movement->ProjectileGravityScale = 0.f;  // horizontal no MVP
```

### OnOverlap → dano GAS

```cpp
void ADDProjectileBase::OnOverlap(UPrimitiveComponent* Overlapped,
    AActor* Other, UPrimitiveComponent* OtherComp,
    int32 BodyIndex, bool bFromSweep, const FHitResult& Hit)
{
    if (Other == Instigator || Other == OwnerActor) return;
    if (!Other->Implements<UDamageableInterface>()) return;  // ou IAbilitySystemInterface

    // Aplica GE_Damage via ASC do Instigator (inimigo ou player)
    ApplyDamageToTarget(Other, Data);

    // Juice: GameplayCue de impacto
    if (UAbilitySystemComponent* ASC = GetInstigatorASC())
        ASC->ExecuteGameplayCue(Data->ImpactCue, CueParams);

    if (!Data->bPierce || ++PierceCount >= Data->MaxPierceCount)
        ReturnToPool();
}
```

> 🔗 O dano segue o mesmo pipeline do melee ([18 §3](18_Combat_System_Deep.md)): `GE_Damage` com `SetByCaller` de magnitude, meta-attribute `IncomingDamage`, poise damage separado.

---

## 5. 👁️ Leitura topdown — telegrafe de trajetória

Em topdown, o jogador precisa ver **de onde** e **para onde** o tiro vai **antes** de sair.

| Camada | Implementação | Prioridade |
|---|---|---|
| **Linha no chão** | `UDecalComponent` ou Niagara `NS_Telegraph_Line` (feixe vermelho do atirador → alvo) | 🟢 P0 |
| **Zona de impacto** | círculo no ponto previsto do player (raio ~40cm) | 🟢 P0 |
| **Tracer em voo** | `NS_Projectile` — faixa luminosa no plano horizontal | 🟢 P0 |
| **Sombra do projétil** | decal/blob que segue o tracer no chão | 🟡 P1 |
| **Tell sonoro** | "carregar" distinto por 0.4s antes do tiro ([35 §4](../audio/35_Audio_Music.md)) | 🟢 P0 |

```
Windup (0.5s):
  ┌─────────────────────────────────────┐
  │  🔴━━━━━━━━━━━━━━━━━━━━━━●         │  ← linha vermelha pulsante (chão)
  │         (atirador)      (zona)      │
  └─────────────────────────────────────┘
Active:
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━●━━━━━━━→   ← tracer em voo (Niagara)
```

> 🎯 A **linha no chão** é o equivalente do decal de melee ([32 §1](../enemies/32_Enemy_Combat_Behavior.md)) — é o que torna o ranged **justo** em -55°.

---

## 6. Pooling (obrigatório na horda)

Com 2-4 atiradores + ondas, spawn/destruir projéteis a cada tiro **engasga o GC** ([36 §5](../feel/36_VFX_Niagara.md)).

```cpp
// UDDRProjectilePool (WorldSubsystem ou no EncounterManager)
TSubclassOf<ADDProjectileBase> ProjectileClass;
TArray<ADDProjectileBase*> Inactive;

ADDProjectileBase* Acquire(const FTransform& TM)
{
    ADDProjectileBase* P = Inactive.Num() ? Inactive.Pop() : World->SpawnActor<ADDProjectileBase>(...);
    P->ResetAndActivate(TM);
    return P;
}

void Release(ADDProjectileBase* P)
{
    P->Deactivate();  // esconde, desliga colisão, para movement
    Inactive.Add(P);
}
```

| Regra | Valor sugerido |
|---|---|
| Pool inicial | 16-32 projéteis |
| Lifetime máximo | 3s (auto-release se não acertar) |
| Tracer Niagara | reutilizar component; `ActivateSystem` / `Deactivate` |

---

## 7. Integração com IA (Atirador)

O Behavior Tree do Atirador ([30 §3](../enemies/30_AI_Overview.md), [31](../enemies/31_Enemy_Archetypes.md)):

| Blackboard key | Uso |
|---|---|
| `DesiredRange` | distância ideal (~600-800uu) |
| `bInRange` | pode atirar? |
| `bPlayerTooClose` | recua (kite) |

```
[Perseguir até DesiredRange]
   → [Se bInRange && tem token de ataque] → GA_Enemy_RangedAttack
   → [Se bPlayerTooClose] → MoveTo (recuar)
```

- **Token de ataque** ([32 §5](../enemies/32_Enemy_Combat_Behavior.md)): atiradores também respeitam — no máximo 1-2 tiros simultâneos na tela.
- **Pausa no Airborne** ([30 §6](../enemies/30_AI_Overview.md)): atirador combado no ar não atira.

---

## 8. GameplayCues de projétil

| Tag | Quando | Canais |
|---|---|---|
| `GameplayCue.Proj.Fire` | spawn do projétil | SFX whoosh + flash curto no muzzle |
| `GameplayCue.Proj.Tracer` | em voo (looping, opcional) | Niagara tracer |
| `GameplayCue.Proj.Impact` | overlap/hit | `NS_Proj_Impact` + SFX + shake leve |
| `GameplayCue.Proj.Telegraph` | windup | linha no chão + tell sonoro |

> 🧩 Mesmo padrão do [21 §10](../feel/21_Juice_FX.md): a ability dispara a Cue; o código de gameplay não sabe de Niagara.

---

## 9. Player ranged (futuro — P2)

A mesma infra serve habilidades do player:

| Ability | Diferença |
|---|---|
| `GA_Player_Bolt` (Boon) | Instigator = player; mira = cursor/stick direito (Modelo B, [06 §3](../systems/06_Camera_TopDown.md)) |
| `GA_Player_Spread` | spawna N projéteis com spread angle |

No MVP o player é **melee** — só o inimigo Atirador usa projéteis. Mas projete `ADDProjectileBase` **agnóstico** (Instigator define o lado).

---

## 10. Prioridade MVP

| Item | Prioridade |
|---|---|
| `ADDProjectileBase` + `UDDRProjectileData` | 🟢 P0 |
| `GA_Enemy_RangedAttack` (windup→spawn→recovery) | 🟢 P0 |
| Telegrafe de linha no chão + tell sonoro | 🟢 P0 (leitura) |
| Tracer + impact VFX/SFX (GameplayCue) | 🟢 P0 |
| Pooling | 🟢 P0 (M3 com horda) |
| Dano via GAS (`GE_Damage`) | 🟢 P0 |
| Kite (recuar se player perto) | 🟢 P0 |
| Sombra do projétil / arco parabólico | 🟡 P1 |
| Pierce / homing / bounce | 🔵 P2 |
| Player ranged abilities | 🔵 P2 |

---

## 11. Checklist

- [ ] `ADDProjectileBase` + `UDDRProjectileData` (DataAsset)
- [ ] `GA_Enemy_RangedAttack` com fases windup/active/recovery
- [ ] **Telegrafe de linha** no chão durante windup ([32 §1](../enemies/32_Enemy_Combat_Behavior.md))
- [ ] Tracer Niagara (`NS_Projectile`) + impact (`NS_Proj_Impact`)
- [ ] Tell sonoro de windup ([35 §4](../audio/35_Audio_Music.md))
- [ ] Hit → `GE_Damage` + poise via GAS ([18 §3](18_Combat_System_Deep.md))
- [ ] **Pooling** (16-32 instâncias)
- [ ] Canal de colisão `Projectile` ([04 §5](../systems/04_Project_Setup.md))
- [ ] Atirador faz kite + respeita token de ataque
- [ ] BT pausa ranged em `State.Combat.Airborne`

---

## 12. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Tiro "invisível" em topdown | Sem tracer / cor fraca | §5 — `NS_Projectile` luminoso no plano horizontal |
| Morte sem entender | Sem telegrafe de linha | §5 — decal/linha no windup |
| Projétil atravessa paredes | Canal errado / sem block | Perfil `Projectile` + Block WorldStatic |
| Projétil não dá dano | Overlap sem GAS / Instigator errado | §4 — `ApplyDamageToTarget` via ASC |
| FPS cai com muitos tiros | Sem pooling | §6 |
| Atirador atira colado no player | Sem kite | §7 — `bPlayerTooClose` → recuar |
| Tunneling (passa pelo player) | Velocidade alta + frame skip | Sweep manual ou sub-step no PMC |

---

## 13. Próximo passo

→ [36 — VFX & Niagara](../feel/36_VFX_Niagara.md) (tracer/impact) · [35 — Áudio](../audio/35_Audio_Music.md) (tell + impact SFX) · [31 — Atirador](../enemies/31_Enemy_Archetypes.md) · [32 — Telegrafe](../enemies/32_Enemy_Combat_Behavior.md).
