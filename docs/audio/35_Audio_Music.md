# 35 — Áudio & Música · 🟡 P1 (núcleo 🟢)

> Sistema de áudio completo: arquitetura (SoundClass/SoundMix/MetaSounds), **áudio de combate** (escala de peso), música adaptativa, **áudio como leitura** em topdown, projéteis, e integração com Settings. Aprofunda a camada de som do [21 — Juice §9](../feel/21_Juice_FX.md). 🎮 Single-player → sem rede.

> 🎯 **Áudio é metade do "juice".** Um hit sem som **certo** sente fraco mesmo com VFX perfeito. E em topdown, o áudio te avisa do que você **não vê** nas bordas. Não é decoração — é feedback e leitura.

---

## 1. Arquitetura (SoundClass / SoundMix / MetaSounds)

```
SoundClass (hierarquia → liga nos sliders de volume, doc 27 §1):
  Master
   ├─ Music
   ├─ SFX
   │   ├─ SFX_Combat   (hits, abilities, projéteis)
   │   ├─ SFX_World    (ambiente, props, portas)
   │   └─ SFX_Enemy    (tells, grunts, death)
   └─ UI
```

| Conceito | Papel no DDR |
|---|---|
| **SoundClass** | Categorias de volume; cada `SoundBase`/`MetaSound` aponta pra uma classe |
| **SoundMix** | Aplica volumes dos sliders + ducking dinâmico |
| **MetaSound** | Variação procedural (pitch, sample shuffle) — anti-metralhadora |
| **SoundCue** | Legado; use só pra sons simples one-shot sem variação |
| **Submix** | Mastering por categoria; sidechain ducking música↔SFX |
| **Attenuation** | Spatialização 2D/3D por tipo de som |

> 🔗 Os **sliders de volume** do [doc 27 §1](../ui/27_Settings.md) controlam estas SoundClasses via SoundMix. Ligue no boot do `GameInstance`.

### `UDDRAudioSubsystem` (opcional mas recomendado)

```cpp
UCLASS()
class UDDRAudioSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void ApplySettings(const UDDRSettingsSave* Save);  // volumes dos sliders
    void SetMusicState(EDDRMusicState State);         // Menu/Combat/Boss
    void DuckMusic(float Amount, float Duration);     // slam, boss intro
    UAudioComponent* PlaySFX2D(USoundBase* Sound, EDDRSFXCategory Cat);
    UAudioComponent* PlaySFXAtLocation(USoundBase* Sound, FVector Loc,
                                       const FSoundAttenuation* Atten);
};
```

Centraliza mix, ducking e troca de música — evita `UGameplayStatics::PlaySound` espalhado.

---

## 2. 🥊 Áudio de combate — escala por peso (espelha o juice)

O som do hit deve **escalar com a importância do golpe**, igual à [escala de juice do doc 21 §7](../feel/21_Juice_FX.md):

| Golpe | Camadas de som | Sensação |
|---|---|---|
| **Light** | whoosh + tap seco | rápido, leve |
| **Heavy** | whoosh + thud grave | peso |
| **Launcher** | whoosh + "whoomp" ascendente | "subiu!" |
| **Air hit** | tick metálico curto | mantém ritmo aéreo |
| **Slam (clímax)** | impacto + **sub-bass** + reverb + debris | o EVENTO sonoro do combo |
| **Projétil impact** | zip + thwack seco | ranged legível |

**Camadas (layering):** um hit = 2-3 sons sobrepostos (swing/whoosh + impacto + "crunch" de material). É o que dá "carne" à pancada.

### MetaSound — receita anti-metralhadora

```
MS_Hit_Impact (MetaSound):
  [Random] → escolhe 1 de 3 samples de impacto
  [Pitch Shift] → ±5-10% aleatório
  [Gain] → escala por SetFloatParameter "HitWeight" (0.3 light → 1.0 slam)
  [Output]
```

```cpp
// Na GameplayCue de hit:
UAudioComponent* AC = UGameplayStatics::SpawnSound2D(...);
if (UMetaSoundSource* MS = Cast<UMetaSoundSource>(Sound))
    AC->SetFloatParameter("HitWeight", Weight);  // 0.3 / 0.6 / 1.0
```

> 🔊 **MetaSound randomiza** pitch ±, sample alternado → 8 hits seguidos soam vivos, não idênticos. Sem isso, o combo vira "tá-tá-tá" robótico.

### Catálogo SFX de combate (MVP)

| Asset | Uso | SoundClass | Prioridade |
|---|---|---|:---:|
| `MS_Hit_Light` | jab de combo | SFX_Combat | 🟢 |
| `MS_Hit_Heavy` | golpe pesado | SFX_Combat | 🟢 |
| `MS_Launch` | launcher | SFX_Combat | 🟢 |
| `MS_Slam` | slam + sub-bass | SFX_Combat | 🟢 |
| `MS_AirTick` | hit aéreo | SFX_Combat | 🟡 |
| `SFX_Dash` | dash/whoosh | SFX_Combat | 🟢 |
| `SFX_PerfectDodge` | "ting" + drone | SFX_Combat | 🟡 |
| `SFX_Proj_Fire` | disparo atirador | SFX_Enemy | 🟢 |
| `SFX_Proj_Impact` | projétil acerta | SFX_Combat | 🟢 |
| `SFX_Telegraph_*` | tell por ataque inimigo | SFX_Enemy | 🟢 |

---

## 3. ⏱️ Áudio respeita o time-dilation (regra de leitura nº3, doc 21)

O jogo usa **slow-mo/hit-stop** (witch-time do perfect-dodge, slam) — e [21 regra #3](../feel/21_Juice_FX.md) manda: **o áudio escala com o `time-dilation`**, senão fica "quebrado" (som a 1× sobre mundo a 0.3×).

| Modo | Implementação |
|---|---|
| **Hit-stop (freeze)** | `GlobalTimeDilation=0` → sons pausam automaticamente se usam clock de mundo |
| **Slow-mo (0.3×)** | pitch baixa junto (`SetPitchMultiplier` ∝ dilation) — efeito "Witch Time" |
| **Slam slow** | duck música 50% + sub-bass em pitch normal (contraste) |

```cpp
// Ao aplicar slow-mo global:
const float Dilation = 0.3f;
GetWorldSettings()->SetTimeDilation(Dilation);
// Opcional: pitch-shift ambiente (não hits — hits já congelam)
MusicComponent->SetPitchMultiplier(FMath::Lerp(0.7f, 1.f, Dilation));
```

> Em single-player isso é trivial — o áudio segue o `GlobalTimeDilation`. Aproveite: o slow-mo *com* áudio esticado é metade do prazer do perfect-dodge.

---

## 4. 🎧 Áudio como leitura topdown

Em topdown, ameaças nas **bordas da tela** somem da vista — mas o **som as anuncia**:

| Uso | Efeito | Implementação |
|---|---|---|
| **Tell sonoro de telegrafe** ([32 §1](../enemies/32_Enemy_Combat_Behavior.md)) | cada ataque inimigo tem som de windup distinto | `SFX_Telegraph_MeleeSwing`, `SFX_Telegraph_RangedCharge` |
| **Spatialização leve** | ouvir "de que lado" vem o perigo/projétil | Attenuation com `bSpatialize=true`, falloff curto (~1500uu) |
| **Som de spawn** ([33 §3](../enemies/33_Spawning_Encounters.md)) | inimigo entrando avisa antes de chegar | `SFX_Enemy_Spawn` 2D ou posicional |
| **Pickup de Eco / cura** | confirma o que você pegou sem olhar o HUD | `SFX_Eco_Pickup` + sting musical curto |
| **Projétil em voo** | "zip" direcional ([37 §8](../combat/37_Projectiles.md)) | attenuation no tracer; volume sobe ao se aproximar |

### Attenuation topdown (preset sugerido)

```
ATT_DDR_Combat:
  Distance Algorithm: Linear
  Falloff Distance: 800 - 2000 uu
  Spatialization: Enabled (HRTF opcional)
  Focus Factor: 0.5  (sons fora da tela ainda audíveis)
```

> 🎯 O **tell sonoro do telegrafe** é tão importante quanto o decal no chão — junta visão (decal) + audição (tell) pra leitura redundante e justa.

---

## 5. 🎵 Música — estados & transições

### Máquina de estados musical

```
[Menu/Hub] ──(entra na sala)──→ [Combat_Low]
                                    │
                         (3+ inimigos vivos)
                                    ↓
                               [Combat_High]
                                    │
                              (boss spawn)
                                    ↓
                               [Boss]
                                    │
                              (vitória/morte)
                                    ↓
                            [Menu] / [Stinger]
```

| Estado | Conteúdo | MVP | Pós-MVP |
|---|---|---|---|
| **Menu/Hub** | 1 loop calmo, 2-3 min | 🟢 1 track | variações |
| **Combat_Low** | tensão baixa, poucos inimigos | 🟡 1 loop | camada base adaptativa |
| **Combat_High** | intensidade, percussão extra | 🟡 stinger/layer | stems separados |
| **Boss** | tema dedicado | 🟡 1 track | fases musicais (HP%) |
| **Victory/Death** | sting 3-5s | 🟡 P1 | — |

### Implementação MVP (simples)

```cpp
enum class EDDRMusicState { Menu, Combat, Boss, Victory, Death };

void UDDRAudioSubsystem::SetMusicState(EDDRMusicState State)
{
    if (CurrentState == State) return;
    // Crossfade 1-2s entre AudioComponents
    FadeOut(CurrentMusic, 1.5f);
    CurrentMusic = SpawnMusicLoop(Tracks[State]);
    FadeIn(CurrentMusic, 1.5f);
}
```

| Trigger | Quem chama |
|---|---|
| Entra na arena | `EncounterManager` → `Combat` |
| Boss spawn | `EncounterManager` → `Boss` |
| Limpa sala | `RunManager` → sting `Victory` → `Menu` |
| Player morre | `GE_Death` → `Death` |

### Ducking (música abaixa pro SFX brilhar)

| Evento | Duck amount | Duração |
|---|---|---|
| Slam impact | -6 dB | 0.4s |
| Boss intro | -9 dB | 2s |
| Perfect dodge | -3 dB | 0.8s (witch-time) |

Use **SoundMix modulator** ou sidechain no submix de música.

### Adaptativa (P1/P2)

- **Stems:** Combat = `Base.opus` + `Percussion.opus` + `Brass.opus`; código liga/desliga camadas por intensidade.
- **Quartz** (UE5): transições no compasso; quantize mudanças de estado ao próximo beat.
- Referência: Hades (layers por combate), Death Must Die (intensidade por densidade).

---

## 6. 🐝 Concurrency (horda não pode estourar o ouvido)

Com 8 inimigos + combo, dezenas de sons disparam juntos. Sem controle = clipping/caos.

| SoundClass | Max instances | Replacement rule |
|---|---|---|
| `SFX_Combat` (hit) | 6 | mais antigo |
| `SFX_Enemy` (tell) | 4 | mais antigo |
| `SFX_Combat` (proj impact) | 4 | mais antigo |
| `UI` | 2 | não repetir |

Configure em **Project Settings → Audio → Concurrency**.

> Evita o "muro de som" da horda e protege o mix. O slam sempre passa (prioridade alta / volume override).

---

## 7. UI & ambiente

| Categoria | Exemplos | SoundClass |
|---|---|---|
| **UI** | click, hover, confirm, error | UI |
| **Ambiente** | vento dungeon, tocha, porta | SFX_World |
| **Feedback positivo** | level up Eco, cura | UI + sting musical curto |

Volume UI independente do SFX de combate — jogadores frequentemente abaixam combate mas mantêm UI audível.

---

## 8. Integração com Settings ([27](../ui/27_Settings.md))

```cpp
void UDDRAudioSubsystem::ApplySettings(const UDDRSettingsSave* S)
{
    USoundMix* Mix = LoadSoundMix("SM_DDR_User");
    Mix->SetSoundMixClassOverride(World, SC_Master,  S->VolumeMaster  / 100.f);
    Mix->SetSoundMixClassOverride(World, SC_Music,   S->VolumeMusic   / 100.f);
    Mix->SetSoundMixClassOverride(World, SC_SFX,     S->VolumeSFX     / 100.f);
    Mix->SetSoundMixClassOverride(World, SC_UI,      S->VolumeUI      / 100.f);
    PushSoundMixModifier(World, Mix);
}
```

Sliders aplicam **ao vivo** enquanto arrasta ([27 §4](../ui/27_Settings.md)).

---

## 9. Prioridade MVP

| Item | Prioridade |
|---|---|
| SoundClasses + ligar aos sliders (27) | 🟢 P0 |
| 1 SFX por ação (hit/dash/launcher/slam) | 🟢 P0 |
| Tell sonoro de telegrafe (melee + ranged) | 🟢 P0 (leitura) |
| SFX projétil (fire + impact) — [37](../combat/37_Projectiles.md) | 🟢 P0 |
| Música: loop menu + combate + boss | 🟡 P1 |
| Layering + MetaSound (variação de pitch) | 🟡 P1 |
| Áudio segue time-dilation | 🟡 P1 |
| Concurrency limits | 🟡 P1 |
| Ducking no slam/boss | 🟡 P1 |
| Música adaptativa (stems) | 🔵 P2 |
| Ambiente/room tone | 🔵 P2 |

---

## 10. Checklist

- [ ] Hierarquia de SoundClass ligada aos sliders ([27 §1](../ui/27_Settings.md))
- [ ] `UDDRAudioSubsystem` (mix, música, ducking)
- [ ] 1 SFX por ação; combate em camadas (whoosh+impacto+crunch)
- [ ] **Escala de som por peso** (light→slam, espelha [21 §7](../feel/21_Juice_FX.md))
- [ ] MetaSound randomiza pitch (anti-metralhadora)
- [ ] Áudio segue `GlobalTimeDilation` (slow-mo)
- [ ] **Tell sonoro** em cada telegrafe de inimigo ([32 §1](../enemies/32_Enemy_Combat_Behavior.md))
- [ ] SFX de projétil (windup + fire + impact) — [37](../combat/37_Projectiles.md)
- [ ] Concurrency limita vozes de hit na horda
- [ ] Música por contexto (menu/combate/boss) com crossfade
- [ ] GameplayCue dispara SFX (não hardcode nas abilities)

---

## 11. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Combo soa "metralhadora" | Mesmo sample sem variação | §2 — MetaSound randomiza pitch |
| Slow-mo soa "quebrado" | Áudio a 1× sobre mundo lento | §3 — seguir time-dilation |
| Slider de volume não funciona | SoundClass não ligado ao SoundMix | §1 / §8 |
| Horda estoura/clippa | Sem concurrency | §6 |
| Não ouço o ataque vindo | Sem tell sonoro / sem spatialização | §4 |
| Música corta abrupto | Sem crossfade | §5 — fade 1.5s |
| Slam perde impacto | Música compete com sub-bass | §5 — ducking |
| Dois SFX duplicados | PlaySound espalhado + Cue | Centralize na GameplayCue |

---

## 12. Próximo passo

→ [36 — VFX & Niagara](../feel/36_VFX_Niagara.md) (o par visual) · [37 — Projéteis](../combat/37_Projectiles.md) (SFX ranged) · [21 — Juice](../feel/21_Juice_FX.md) (visão geral sensorial) · [27 — Settings](../ui/27_Settings.md) (volumes).
