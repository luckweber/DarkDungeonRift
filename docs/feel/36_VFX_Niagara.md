# 36 — VFX & Niagara (detalhado) · 🟡 P1 (núcleo 🟢)

> Aprofundamento técnico do VFX do [21 — Juice §2/§8](21_Juice_FX.md): arquitetura Niagara, **spec por sistema**, regras de leitura topdown, GameplayCue, pooling, projéteis, performance na horda, e workflow de editor. 🎮 Single-player.

> 🧭 **Regra-mãe (de [21, Vi](21_Juice_FX.md)):** em topdown, *VFX serve a leitura, não compete com ela*. FX mal-feito vira ruído que esconde o combate. **FX achatados no chão > verticais.**

---

## 1. Arquitetura Niagara

| Conceito | Uso no DDR |
|---|---|
| **Niagara System** | um efeito completo (ex.: `NS_Hit_Spark`) |
| **Emitters** | sub-partes (faísca + fumaça + flash num só system) |
| **User Parameters** | dirigidos por código (cor, escala, direção) — §3 |
| **Module Parameters** | internos ao emitter (velocity, color, size) |
| **Data Interfaces** | SkeletalMesh, StaticMesh, Curve, Array, Audio |
| **Niagara Component** | instância no mundo (spawnada/anexada) |
| **Niagara Actor** | wrapper quando precisa de tick/lifetime no mundo |

### Spawn patterns

| Padrão | API | Quando |
|---|---|---|
| **One-shot no impacto** | `UNiagaraFunctionLibrary::SpawnSystemAtLocation` | hit, slam, death |
| **Attached (trail)** | `SpawnSystemAttached` no mesh/socket | slash trail, dash |
| **Pooling** | `UDDRVFXPool::Acquire(NS, Transform)` | hit-spark, projétil (§6) |
| **Decal paralelo** | `UDecalComponent` + material parametrizado | telegrafe zona (§4) |

### Organização de assets

```
Content/VFX/
  Combat/
    NS_Hit_Spark.uasset
    NS_Slam_Shockwave.uasset
    NS_Projectile.uasset
    NS_Proj_Impact.uasset
  Telegraph/
    NS_Telegraph_Zone.uasset
    NS_Telegraph_Line.uasset
    M_Telegraph_Decal.uasset
  Character/
    NS_Launch_Trail.uasset
    NS_Dash_Trail.uasset
  Materials/
    M_Particle_Add.uasset
    M_Particle_Alpha.uasset
```

---

## 2. Catálogo de VFX — spec detalhado

| Sistema | Quando | Emitters | Lifetime | User Params | Prio |
|---|---|---|---|---|:---:|
| **`NS_Hit_Spark`** | acerto light/heavy | Flash (1 frame) + Sparks (12 part) | 0.15-0.3s | `HitWeight`, `ImpactNormal`, `Tint` | 🟢 |
| **`NS_Hit_Ring`** | heavy+ | Ring mesh no chão (achatado) | 0.25s | `Radius`, `Tint` | 🟡 |
| **`NS_Slash_Trail`** | swing | Ribbon ao longo do arco | duração do swing | `WeaponCurve` (DI) | 🟡 |
| **`NS_Launch_Trail`** | launcher | **Trail VERTICAL** + burst base | 0.4-0.6s | `Height`, `Tint` | 🟢 |
| **`NS_Slam_Shockwave`** | slam | Ring expand + Dust + Crack decal | 0.5-0.8s | `Radius`, `Intensity` | 🟢 |
| **`NS_Land_Dust`** | pouso | Poeira radial no chão | 0.4s | `LandVelocity` | 🟡 |
| **`NS_Death`** | inimigo morre | Burst + dissolve (opcional) | 0.6s | `Tint` | 🟡 |
| **`NS_Telegraph_Zone`** | windup melee | Decal pulsante (círculo/cone) | = windup | `Radius`, `Color`, `PulseRate` | 🟢 |
| **`NS_Telegraph_Line`** | windup ranged | Linha no chão atirador→alvo | = windup | `Start`, `End`, `Color` | 🟢 |
| **`NS_Dash_Trail`** | dash | Afterimage / streak no chão | 0.2s | `Direction` | 🟡 |
| **`NS_Eco_Pickup`** | Eco | Brilho + partículas ascendentes | loop até pickup | `RarityColor` | 🟡 |
| **`NS_Projectile`** | projétil em voo | Core glow + ribbon trail horizontal | até impacto | `Speed`, `Tint` | 🟢 |
| **`NS_Proj_Impact`** | projétil acerta | Burst + faíscas | 0.2s | `ImpactNormal` | 🟢 |

> 🔗 Projéteis: ver fluxo completo em [37 — Projéteis](../combat/37_Projectiles.md).

---

## 3. User Parameters (contrato código ↔ Niagara)

Defina estes **User Parameters** no Niagara System (namespace `User.`):

| Parameter | Tipo | Quem seta | Uso |
|---|---|---|---|
| `User.HitWeight` | float | GameplayCue | 0.3 light → 1.0 slam; escala tamanho/intensidade |
| `User.Tint` | LinearColor | GameplayCue / Eco | cor por família elemental |
| `User.ImpactNormal` | Vec3 | Hit detection | orienta faíscas na superfície |
| `User.Radius` | float | Ability / Telegraph | AoE slam, zona de perigo |
| `User.Direction` | Vec3 | Dash / Knockback | trail, shockwave bias |
| `User.Start` / `User.End` | Vec3 | Ranged telegraph | linha de tiro no chão |
| `User.PulseRate` | float | Telegraph | velocidade do pulsar vermelho |

```cpp
// UDDRGameplayCueNotify — OnExecute
UNiagaraComponent* FX = SpawnNiagaraAtLocation(NS_Hit_Spark, Location);
FX->SetVariableFloat(TEXT("User.HitWeight"), CueParams.HitWeight);
FX->SetVariableLinearColor(TEXT("User.Tint"), CueParams.Tint);
FX->SetVariableVec3(TEXT("User.ImpactNormal"), Hit.Normal);
```

> 🎚️ **Escala por peso:** o mesmo `NS_Hit_Spark` serve light e slam — só muda `HitWeight`. Menos assets, mais consistência ([21 §7](21_Juice_FX.md)).

---

## 4. 👁️ Telegrafe VFX (leitura defensiva P0)

O telegrafe é o VFX **mais importante** do combate topdown ([32 §1](../enemies/32_Enemy_Combat_Behavior.md), [21 regra #5](21_Juice_FX.md)).

### Melee — `NS_Telegraph_Zone`

```
Emitter: DecalMesh (ou Sprite no chão)
  • Forma: círculo (AoE), cone (sweep), retângulo (charge)
  • Material: M_Telegraph_Decal — emissive vermelho, opacity pulsa
  • Scale: = HitRadius do FDDRAttackData (18 §1)
  • Orientação: achatado Y-up (normal do chão)
```

| Parâmetro | Valor típico |
|---|---|
| Cor base | `(1, 0.1, 0.1)` vermelho |
| Pulse | `sin(time * PulseRate)` na opacity |
| Duração | = `StartupFrames` do ataque |

### Ranged — `NS_Telegraph_Line` ([37 §5](../combat/37_Projectiles.md))

```
Emitter: Ribbon/Beam no plano Z=chão
  • User.Start = socket muzzle
  • User.End = posição prevista do player
  • Largura: 8-12uu; emissive forte
```

### Acessibilidade ([27 §2](../ui/27_Settings.md))

Setting **Daltonismo** troca `User.Tint` do telegrafe:
- Protanopia: azul/ciano em vez de vermelho
- Deuteranopia: amarelo/laranja

---

## 5. Projétil VFX ([37](../combat/37_Projectiles.md))

### `NS_Projectile` (em voo)

| Emitter | Config |
|---|---|
| **Core** | sprite pequeno, emissive alto, cor quente (laranja/magenta) |
| **Trail** | ribbon horizontal; comprimento ∝ speed |
| **Ground shadow** (P1) | decal escuro no chão seguindo posição |

- **Plano horizontal** no MVP — a câmera -55° vê o tracer claramente.
- `Fixed Bounds` pequeno (~200uu) — performance.

### `NS_Proj_Impact`

- Burst 8-16 partículas + flash 2 frames.
- `User.ImpactNormal` orienta o burst.
- Pooling obrigatório (§6).

---

## 6. Pooling & lifecycle

```cpp
UCLASS()
class UDDRVFXPool : public UWorldSubsystem
{
    TMap<UNiagaraSystem*, TArray<UNiagaraComponent*>> Inactive;

    UNiagaraComponent* Acquire(UNiagaraSystem* NS, const FTransform& TM)
    {
        UNiagaraComponent* C = Inactive[NS].Num()
            ? Inactive[NS].Pop() : NewObject<UNiagaraComponent>(...);
        C->SetAsset(NS);
        C->SetWorldTransform(TM);
        C->Activate(true);
        C->SetVariableFloat(TEXT("User.AutoRelease"), PoolLifetime);
        return C;
    }

    void Release(UNiagaraComponent* C)
    {
        C->Deactivate();
        Inactive[C->GetAsset()].Add(C);
    }
};
```

| Sistema | Pool size | Auto-release |
|---|---|---|
| `NS_Hit_Spark` | 24 | 0.3s |
| `NS_Projectile` | 32 | on impact / 3s |
| `NS_Proj_Impact` | 16 | 0.25s |
| `NS_Telegraph_*` | 8 | fim do windup |

> 🔥 Em densidade Death-Must-Die, **pooling de hit-spark e projétil** é obrigatório — senão o GC e o spawn engasgam o frame no clímax.

---

## 7. Disparo via GameplayCue (não hardcode)

VFX e SFX são disparados pelas **GameplayCues** ([05 §7](../systems/05_GAS_Architecture.md), catálogo em [21 §10](21_Juice_FX.md)):

```
GameplayCue.Hit.Heavy (executada pelo GE de dano):
   → OnExecute: Pool.Acquire(NS_Hit_Spark, HitLocation)
              + Set User Params (cor, escala, normal)
              + toca SFX (doc 35)
              + screen shake
```

### Tipos de Cue Notify

| Tipo UE | Uso DDR |
|---|---|
| `UGameplayCueNotify_Static` (Burst) | hits, impactos, slam |
| `AGameplayCueNotify_Actor` (Looping) | aura rara, telegrafe longo (boss) |
| `UGameplayCueNotify_BurstLatent` | sequência slam (shockwave → dust) |

---

## 8. Materiais & leitura topdown

| Material | Blend | Uso |
|---|---|---|
| `M_Particle_Add` | Additive | faíscas, flash, core de projétil |
| `M_Particle_Alpha` | Translucent | fumaça, poeira |
| `M_Telegraph_Decal` | Emissive masked | zonas de perigo no chão |

**Regras visuais topdown:**
- **Alto contraste** — partículas devem ler sobre chão escuro *e* claro.
- **Poucos pixels** — flash 1-2 frames > nuvem de 50 partículas ([21 §5](21_Juice_FX.md)).
- **Achatado no chão** — shockwave, ring, decal; exceção: `NS_Launch_Trail` vertical.
- **Sombra no chão** ([06 §4](../systems/06_Camera_TopDown.md)) complementa VFX aéreo.

---

## 9. ⚙️ Performance (horda)

| Técnica | Efeito |
|---|---|
| **Pooling** (§6) | elimina spawn/GC por hit |
| **Fixed Bounds** | evita recálculo de bounds por frame |
| **Scalability / Effect Type** | reduz partículas em qualidade baixa ([27](../ui/27_Settings.md)) |
| **GPU emitters** | faísca/poeira densas (CPU só pra lógica leve) |
| **Significance / distance cull** | FX longe do player = desligado ou simplificado |
| **Max particle count** | cap por emitter (ex.: 16 sparks, não 200) |

### Budget sugerido (60fps, 8 inimigos)

| Métrica | Alvo |
|---|---|
| Niagara systems ativos | < 40 simultâneos |
| Partículas na tela | < 3000 |
| Spawn por frame | < 8 (com pool) |

`stat Niagara` e `profilegpu` no clímax do combo.

---

## 10. ⏱️ Time-dilation

[21 regra #3](21_Juice_FX.md): no slow-mo/hit-stop, o **Niagara deve escalar com o time-dilation**.

| Abordagem | Como |
|---|---|
| **Actor dilation** | spawn no ator congelado → herda `CustomTimeDilation` |
| **System flag** | `FNiagaraSystemScalabilitySettings` → Fixed Delta Time |
| **Manual** | `NiagaraComponent->SetCustomTimeDilation(Dilation)` |

O par **áudio ([35 §3](../audio/35_Audio_Music.md)) + VFX** tem que desacelerar **junto**.

---

## 11. Workflow de editor

1. **Prototype** no Niagara Editor com `User.` params expostos.
2. **Placeholder** — esferas/cubos coloridos antes de arte final.
3. **GameplayCue test** — `GameplayCue.PrintGameplayCueManager` + botão debug.
4. **Preview topdown** — câmera -55° no viewport; não tune de frente.
5. **Scalability** — crie `NS_*_Low` ou Effect Type com % de partículas.
6. **Fixed Bounds** — marque após o visual estabilizar.

---

## 12. Prioridade MVP

| Item | Prioridade |
|---|---|
| `NS_Hit_Spark` + flash | 🟢 P0 |
| `NS_Telegraph_Zone` (decal melee) | 🟢 P0 (leitura) |
| `NS_Telegraph_Line` (ranged) | 🟢 P0 |
| `NS_Launch_Trail` + sombra (pilar) | 🟢 P0 |
| `NS_Slam_Shockwave` (achatado) | 🟢 P0 |
| `NS_Projectile` + `NS_Proj_Impact` | 🟢 P0 |
| User Params (HitWeight, Tint, Normal) | 🟢 P0 |
| GameplayCue wiring | 🟢 P0 |
| Trails slash/dash, poeira, death, Eco | 🟡 P1 |
| Pooling + scalability | 🟡 P1 |
| Ground shadow em projétil | 🟡 P1 |
| Deformer/efeitos avançados | 🔵 P2 |

---

## 13. Checklist

- [ ] VFX disparado por **GameplayCue** ([21 §10](21_Juice_FX.md)), não hardcode
- [ ] User Params: `HitWeight`, `Tint`, `ImpactNormal`, `Radius`, `Start/End`
- [ ] **Escala por peso** (slam = burst grande, [21 §7](21_Juice_FX.md))
- [ ] **FX achatado no chão** (shockwave/ring/decal); trail vertical só no launch
- [ ] `NS_Telegraph_Zone` em todo ataque melee ([32 §1](../enemies/32_Enemy_Combat_Behavior.md))
- [ ] `NS_Telegraph_Line` em todo tiro ranged ([37](../combat/37_Projectiles.md))
- [ ] Sombra no chão (leitura do combo aéreo)
- [ ] **Pooling** de hit-spark e projétil (§6)
- [ ] Niagara segue time-dilation (§10)
- [ ] Fixed Bounds + particle caps (§9)
- [ ] Tune na câmera topdown (-55°), não lateral

---

## 14. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| FPS cai no clímax do combo | Sem pooling | §6 |
| Faísca "voa" no slow-mo | Niagara ignora time-dilation | §10 |
| Tela poluída na horda | Partícula em vez de flash | §8 / [21 §5](21_Juice_FX.md) |
| Combo aéreo ilegível | Sem sombra/trail vertical | §2 / [06 §4](../systems/06_Camera_TopDown.md) |
| VFX espalhado no código | Spawn hardcode em vez de cue | §7 |
| Telegrafe ilegível | Vertical / cor fraca | §4 — achatado + emissive |
| Projétil invisível | Trail vertical ou cor escura | §5 — horizontal + emissive |
| Bounds enormes | Fixed Bounds não setado | §9 |

---

## 15. Próximo passo

→ [35 — Áudio](../audio/35_Audio_Music.md) · [37 — Projéteis](../combat/37_Projectiles.md) · [21 — Juice](21_Juice_FX.md) · [32 — Telegrafe](../enemies/32_Enemy_Combat_Behavior.md).
