# 19 — Roster de Abilities (Profundo) · 🟢 P0

> O **conteúdo GAS** do combate: cada habilidade como **dado** — sua classe base, taxonomia de tags, fichas completas do roster MVP e o padrão de **conceder/trocar abilities em runtime** (o que os Ecos fazem 20× por run). Aprofunda o §4 do [05 — Arquitetura GAS](../systems/05_GAS_Architecture.md): lá você montou o ASC/AttributeSet; **aqui** você especifica as abilities que vivem nele.

> **Pré-requisitos:** [05 — GAS](../systems/05_GAS_Architecture.md) (ASC, Effects, Tags), [07 — Input](../systems/07_Input.md) (InputID → ability), [15 — Combate](15_Combat_Overview.md) (montages, janelas, gramática de cancelamento), [16 — Combos Aéreos](16_Aerial_Combos.md) (launcher/juggle/slam), [03b — Ecos](../design/03b_Reward_System.md) (recompensa data-driven).

> 🔒 **Single-player, local-authoritative** (decisão 2026-06, [Index](../00_Index.md)). Sem replicação/predição. Toda ability roda local; `InitAbilityActorInfo(this, this)` com server==client. Onde docs antigos do GAS falarem "networked/prediction window/minimal replication", **trate como resolvido: não se aplica aqui.** Isso simplifica MUITO o roster — sem `bReplicateInputDirectly`, sem prediction key dramas, sem mixed replication mode.

> ❌ **Fora de escopo (delegado):** frame data, hit detection, fórmula de dano, máquina de combo → [18 — Combat System Deep](18_Combat_System_Deep.md). VFX/SFX/juice → [21 — Juice & FX](../feel/21_Juice_FX.md). Responsividade/feel → [20 — Game Feel](../feel/20_Game_Feel.md). Arquitetura GAS base (ASC/AttributeSet/Cues) → [05](../systems/05_GAS_Architecture.md). Aqui é **a ability como dado**: o que ela é, que tags carrega, o que concede/cancela.

---

## 1. Por que uma classe base própria

Sem uma base, cada ability vira um floco de neve: instancing inconsistente, tags digitadas à mão, helpers copiados. **`UDDRGameplayAbility`** padroniza o roster e é onde Os Ecos "encaixam" (§4). Toda ability do jogo herda dela.

```cpp
UCLASS(Abstract)
class UDDRGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UDDRGameplayAbility();

    // Ativa-na-concessão? (ex.: passivas de Eco). MVP: false p/ ações.
    UPROPERTY(EditDefaultsOnly, Category="DDR|Activation")
    bool bActivateOnGranted = false;

    // Tag de input que ativa esta ability (alternativa Lyra-style ao InputID enum, doc 07 §4).
    UPROPERTY(EditDefaultsOnly, Category="DDR|Input")
    FGameplayTag InputTag;

    // Helper: aplica um GE no próprio dono via SetByCaller (magnitudes data-driven, §4).
    UFUNCTION(BlueprintCallable, Category="DDR")
    FActiveGameplayEffectHandle ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GE,
                                                  const TMap<FGameplayTag,float>& SetByCallerMap);

protected:
    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* Info,
                             const FGameplayAbilitySpec& Spec) override; // auto-activate
};
```

### Config comum (defaults da base)

| Campo | Valor padrão DDR | Por quê (single-player) |
|---|---|---|
| **Instancing Policy** | `InstancedPerActor` | Estado por-ator (ComboIndex, handles de task) sem custo de re-instanciar por ativação. Padrão recomendado p/ a maioria. |
| **Net Execution Policy** | `LocalOnly` | 🔒 Single-player: roda **só local**, sem round-trip server. Mais simples e responsivo que `LocalPredicted`. |
| **Net Security Policy** | `ClientOrServer` | Irrelevante sem rede; deixe o default. |
| **Replication Policy** | `DoNotReplicate` | Nada a replicar. |
| **Retrigger Instanced Ability** | conforme ability | Combo (`GA_Attack_Light`) **não** re-ativa do zero — avança seção (doc 15 §2). Dash pode. |

> 🔑 **Por que `LocalOnly` e não `LocalPredicted`?** Predição existe pra esconder latência de rede. Sem rede, ela só adiciona complexidade (prediction keys, rollback) sem benefício. `LocalOnly` = a ability executa imediatamente no único "lado" que existe. Esta é a maior simplificação que o single-player traz pro roster.

> 🧰 **Helpers que a base oferece** (use em toda ficha): `ApplyEffectToSelf` (acima), `K2_CommitAbility`, `GetDDRCharacter()` (cast do avatar), `SendLocalGameplayEvent(Tag, Payload)` (ponte AnimNotify→ability sem rede). Mantenha-os finos — lógica pesada vai pro [18](18_Combat_System_Deep.md).

---

## 2. Taxonomia de Gameplay Tags

Toda regra de combate é **tag**. Convenção: hierarquia por ponto, nome em inglês (casa com GAS), registrada no [04 §4](../systems/04_Project_Setup.md). Quatro famílias governam o roster:

| Família | Função | Exemplos |
|---|---|---|
| **`Ability.*`** | **Identidade** da ability (o que ela é). Vai em `AbilityTags`. Alvo de `CancelAbilitiesWithTag`. | `Ability.Attack.Light` · `Ability.Attack.Launcher` · `Ability.Attack.Air` · `Ability.Attack.AirSlam` · `Ability.Dash` |
| **`State.*`** | **Estado** transiente do ator (concedido por GE/ability). Gate de ativação e leitura do AnimBP. | `State.Combat.Attacking` · `State.Combat.InAir` · `State.Combat.Airborne` (alvo) · `State.Movement.Dashing` · `State.Dead` |
| **`Cooldown.*`** | **Janela de recarga**. Tag concedida por um GE Duration; lida em `ActivationBlockedTags`. | `Cooldown.Dash` · `Cooldown.Launcher` · `Cooldown.Ultimate` |
| **`Cost.*`** | **Marcador de recurso** (opcional no MVP — stamina é P1). Identifica o GE de custo. | `Cost.Stamina` · `Cost.Stamina.Dash` |

### Convenção de slots numa ficha de ability

Cada ability declara tags em **5 slots** do `UGameplayAbility`. Saber qual vai onde elimina o bug nº 1 do GAS (tag no slot errado):

| Slot (campo UE) | O que colocar | Efeito |
|---|---|---|
| **`AbilityTags`** | a própria `Ability.*` | identidade; é por ela que outras abilities cancelam/bloqueiam esta |
| **`ActivationOwnedTags`** | `State.*` que a ability *concede enquanto roda* | ex.: `State.Combat.Attacking` aplicada ao dono durante a ativação (removida no End) |
| **`ActivationRequiredTags`** | tags que o dono **precisa ter** | ex.: `GA_AirAttack` requer `State.Combat.InAir` |
| **`ActivationBlockedTags`** | tags que **impedem** a ativação | ex.: `State.Dead`, `Cooldown.X` |
| **`CancelAbilitiesWithTag`** | `Ability.*` de **outras** abilities a cancelar ao ativar esta | ex.: `GA_Dash` cancela `Ability.Attack` (dash-cancel, §3) |

> ⚙️ **`ActivationOwnedTags` vs aplicar GE de tag:** `ActivationOwnedTags` adiciona/remove a tag **automaticamente** no ciclo da ability (zero código, some no End/Cancel). Use-o para `State.Combat.Attacking`. Reserve **GE de tag** (`GE_Dead`, `GE_Cooldown_*`) para estados que **persistem além** da ability ou têm duração própria.

> 🧩 **A gramática de cancelamento ([15 §6](15_Combat_Overview.md)) é só tags.** A tabela estado×ação vira `CancelAbilitiesWithTag` (cancelamento "sempre", ex.: dash) + um flag de **janela** ligado por `AnimNotifyState` lido pelo input buffer (cancelamento "só na janela", ex.: attack→launcher). Esta doc define **quais tags** cada ability cancela; o *quando* (janelas/frame data) é do [18](18_Combat_System_Deep.md).

---

## 3. Roster MVP — fichas completas

Formato de **ficha padronizada** (mesma ordem em todas, skimmable). Custos são generosos por ser single-player ([§5](#5-custo--recurso)). Cooldowns via `CooldownGameplayEffectClass` ([05 §10](../systems/05_GAS_Architecture.md)).

> 📋 **Legenda dos slots de tag:** Owned = `ActivationOwnedTags` · Req = `ActivationRequiredTags` · Block = `ActivationBlockedTags` · Cancel = `CancelAbilitiesWithTag`. Todas têm `State.Dead` em **Block** (vem da base) — omitido nas fichas por brevidade.

### 🗡️ GA_Attack_Light — combo de chão

| Campo | Valor |
|---|---|
| **Input** | `IA_Attack` (Started) → InputID `Attack` (doc 07 §4) |
| **AbilityTag** | `Ability.Attack` + `Ability.Attack.Light` |
| **Owned** | `State.Combat.Attacking` |
| **Req** | — |
| **Block** | — (encadeia consigo via janela, não bloqueia) |
| **Cancel** | — (é cancelada por Dash/Launcher, não cancela ninguém) |
| **Custo** | nenhum (MVP) |
| **Cooldown** | nenhum (governado pela montage) |
| **Montage** | `AM_Combo` (seções Atk1→Atk4), Slot `DefaultSlot` |
| **Instancing** | `InstancedPerActor`, **Retrigger = false** (avança seção, não re-ativa) |

**Targeting (soft-lock + warp):** `FaceAndSetupMotionWarp(Ground)` no startup de **cada** golpe (Atk1 e a cada `AdvanceCombo`) — soft-lock → face → warp target `AttackWarp` (cap ~200 cm). Run-attack opener usa perfil `RunAttack`. A montage `AM_Combo` precisa de notify **Motion Warping** **em cada seção** (Atk1–4) — o C++ atualiza o alvo, mas o lunge só ocorre na seção que tem a janela. [57 §2b](57_M1_Combo_Editor_Setup.md) · [60 §7](../systems/60_M2_Editor_Setup.md) · spec [18 §6](18_Combat_System_Deep.md).

**Fluxo de Ability Tasks:**
```
ActivateAbility:
  1. CommitAbility
  2. ComboIndex := próxima seção (1ª ativação → Atk1; reativação na janela → Atk2/3/4)
  2b. FaceAndSetupMotionWarp(profile=Ground ou RunAttack no opener)
  3. Task PlayMontageAndWait(AM_Combo, StartSection=Atk{ComboIndex})
  4. Task WaitGameplayEvent("Combat.Hit")  → encaminha alvo p/ doc 18 (dano)
  5. ANS_ComboWindow liga flag "pode-encadear"; input buffer (doc 07 §5) +
       flag → MontageJumpToSection(Atk{n+1}) [reusa a MESMA instância]
  6. OnMontageEnd / OnCancelled → reset ComboIndex → EndAbility
```

**Concede/dispara:** envia `GameplayEvent "Combat.Hit"` para si (a hitbox do AnimNotify dispara), consumido pelo pipeline de dano do [18](18_Combat_System_Deep.md). É a **ponte para o launcher**: na janela de combo, apertar Launcher faz attack→launcher (gramática [15 §6](15_Combat_Overview.md)).

> 🔒 **Single-player:** `WaitGameplayEvent` recebe evento **local** do notify — sem rede, sem ordem de chegada. O ComboIndex vive na instância (`InstancedPerActor`); zero sync.

> 🛠️ **Setup REAL no editor (M1):** passo a passo da montage + graph BP em **[57 — M1: Combo no Editor](57_M1_Combo_Editor_Setup.md)**. ⚠️ Inclui o fix do bug clássico: **Montage Sections devem ficar SEM link** (botão *Clear*) — seções linkadas auto-avançam e ignoram a janela de combo. O avanço é da ability (`MontageJumpToSection`), nunca da montage.

---

### 🔨 GA_Attack_Heavy — golpe pesado (ramo do combo) · 🟡 P1

> *Fecha o contrato:* o grafo de combo ([18 §4.1](18_Combat_System_Deep.md), ramo H1→H2) e a escala de juice ([21 §7](../feel/21_Juice_FX.md), coluna Heavy) **já contam com o Heavy** — aqui está a ability.

| Campo | Valor |
|---|---|
| **Input** | `IA_Heavy` (Started) → InputID `Heavy` |
| **AbilityTag** | `Ability.Attack` + `Ability.Attack.Heavy` |
| **Owned** | `State.Combat.Attacking` |
| **Cancel** | `Ability.Attack.Light` (light→heavy na janela de combo) |
| **Custo / Cooldown** | nenhum (ritmo via recovery, não cooldown) |
| **Montage** | `AM_Combo_Heavy` (golpes lentos/pesados, frame data próprio — [18 §1](18_Combat_System_Deep.md)) |
| **Instancing** | `InstancedPerActor` |

Mesma máquina do Light (seções + buffer + `Combat.Hit`), mas **mais lento, mais dano/poise**. Dá ao combo de chão o 2º eixo (light × heavy). **Por arma**, o ramo heavy muda ([50](50_Weapons_Arsenal.md)).

> 🎚️ **MVP mínimo:** dá pra shipar **light-only** no M1; Heavy entra quando o combate de chão ganha profundidade (P1). Mas como 18/21 já o assumem, esta ficha evita o "ramo sem ability".

---

### 🚀 GA_Launcher — inicia o aéreo

| Campo | Valor |
|---|---|
| **Input** | `IA_Launcher` (Started) → InputID `Launcher` |
| **AbilityTag** | `Ability.Attack` + `Ability.Attack.Launcher` |
| **Owned** | `State.Combat.Attacking`, depois `State.Combat.InAir` (no player) |
| **Req** | — (chão; mas tipicamente ativada da janela de combo) |
| **Block** | `Cooldown.Launcher`, `State.Combat.InAir` (não relança no ar) |
| **Cancel** | `Ability.Attack.Light` (cancela o combo de chão → vira launcher) |
| **Custo** | nenhum (ou Stamina leve — §5) |
| **Cooldown** | `GE_Cooldown_Launcher` (~1.5–2.5 s; anti-spam, [16 §9](16_Aerial_Combos.md)) |
| **Montage** | `AM_Launcher` (uppercut) |
| **Instancing** | `InstancedPerActor` |

**Targeting:** `FaceAndSetupMotionWarp(Launcher)` — warp horizontal curto (~180 cm), Z do clip; montage `AM_Launcher` com janela `AttackWarp` — [60 §7.2](../systems/60_M2_Editor_Setup.md).

**Fluxo de Ability Tasks:**
```
ActivateAbility:
  1. CommitAbility (paga cooldown)
  1b. FaceAndSetupMotionWarp(profile=Launcher)
  2. Task PlayMontageAndWait(AM_Launcher)
  3. Task WaitGameplayEvent("Combat.Launch")  (notify de impacto)
       → NO ALVO: aplica GE_Airborne (concede State.Combat.Airborne) +
                  RootMotionSource(vertical) dirige à AltitudeAlvo e SEGURA (hang)
       → NO PLAYER (follow): RootMotionSource à MESMA AltitudeAlvo +
                  Owned ganha State.Combat.InAir → habilita GA_AirAttack/AirSlam
  4. OnMontageEnd → EndAbility (mantém State.Combat.InAir até pousar/slam)
```

> ⚠️ **Contrato técnico (revisão de design, Davi · risco ALTA — [16 §2](16_Aerial_Combos.md)):** a subida é **`UAbilityTask_ApplyRootMotionConstantForce` / `...MoveToForce`** (RootMotionSource **ancorado**), **NUNCA** `LaunchCharacter`. `LaunchCharacter` é balístico não-determinístico: com gravidades diferentes (player vs alvo), os corpos divergem na 2ª pancada e o follow launch quebra. RootMotionSource *dirige* a altura → co-altitude garantida. 🔒 Single-player remove o motivo extra (predição) — mas a razão de **feel/determinismo continua válida**, então o contrato permanece.

**Concede/dispara:** `State.Combat.Airborne` (alvo, gate do juggle + pausa de IA, [16 §8](16_Aerial_Combos.md)) e `State.Combat.InAir` (player, gate das abilities aéreas). Estes dois tags são o **switch** que liga todo o combo aéreo.

---

### 💨 GA_AirAttack — ataque aéreo (juggle)

| Campo | Valor |
|---|---|
| **Input** | `IA_Attack` (Started) — **a mesma** do combo de chão (o `State.Combat.InAir` decide qual sai) |
| **AbilityTag** | `Ability.Attack` + `Ability.Attack.Air` |
| **Owned** | `State.Combat.Attacking` |
| **Req** | **`State.Combat.InAir`** ← só ativa no ar |
| **Block** | — |
| **Cancel** | — |
| **Custo** | nenhum |
| **Cooldown** | nenhum (governado pela montage + decay do juggle) |
| **Montage** | `AM_AirCombo` (seções, espelha o de chão) |
| **Instancing** | `InstancedPerActor` |

**Targeting:** `FaceAndSetupMotionWarp(Air)` com `bPreferAirborne` — prioriza alvo `State.Combat.Airborne`; warp menor (~120 cm). **Sem** janela Motion Warp em `AM_AirCombo` (drift indesejado no ar) — [60 §7.2](../systems/60_M2_Editor_Setup.md).

**Fluxo de Ability Tasks:** idêntico ao `GA_Attack_Light` (seções + buffer + `Combat.Hit`), **mais** o re-float: cada hit aplica um `RootMotionSource` curto (PopHeight, com decay) no alvo ([16 §3](16_Aerial_Combos.md)). O **cap de hits** é dono da ability (lê atributo `MaxJuggleHits`, [03b §6](../design/03b_Reward_System.md)).

> 🪂 **Roteamento de input:** `IA_Attack` chama `AbilityLocalInputPressed(Attack)`; o GAS ativa a ability **concedida com esse InputID cujas `ActivationRequiredTags` batem**. Como `GA_AirAttack` requer `InAir` e `GA_Attack_Light` não, o estado do player resolve qual sai — sem `if` no input. Mantenha **ambas** concedidas; o gate de tag faz a seleção.

> 🥋 **Jump-cancel do AirAttack → re-sobe** (cancelamento-âncora nº 3, [15 §6](15_Combat_Overview.md)): na janela aérea, `IA_Jump` cancela o AirAttack e dispara um novo `RootMotionSource` pra cima no player — estende o juggle por skill. Implementado como `GA_Jump` (ou a própria GA_AirAttack) com `CancelAbilitiesWithTag: Ability.Attack.Air` + pop vertical.

---

### 💥 GA_AirSlam — finisher descendente

| Campo | Valor |
|---|---|
| **Input** | `IA_AirSlam` (Started) → InputID `AirSlam` |
| **AbilityTag** | `Ability.Attack` + `Ability.Attack.AirSlam` |
| **Owned** | `State.Combat.Attacking` |
| **Req** | **`State.Combat.InAir`** |
| **Block** | — |
| **Cancel** | `Ability.Attack.Air` (corta o juggle pra fechar) |
| **Custo** | nenhum |
| **Cooldown** | nenhum (custo é "encerra o combo") |
| **Montage** | `AM_Slam` (golpe descendente) |
| **Instancing** | `InstancedPerActor` |

**Targeting:** `FaceAndSetupMotionWarp(Slam)` — só **face** (`bPreferAirborne`); **sem** lunge horizontal. **Sem** janela Motion Warp em `AM_AirSlam` — [60 §7.2](../systems/60_M2_Editor_Setup.md).

**Fluxo de Ability Tasks:**
```
ActivateAbility:
  1. CommitAbility
  1b. FaceSoftLockTarget(bPreferAirborne=true)  // perfil Slam — sem warp
  2. Task PlayMontageAndWait(AM_Slam)
  3. No notify "Combat.SlamDown":
       - player desce rápido (RootMotionSource down forte)
       - alvo arremessado ao chão (RootMotionSource down no alvo)
  4. No impacto com o chão (notify "Combat.SlamLand"):
       - HARD LAND (doc 13 §6): GameplayCue.Land.Hard (poeira+shake) → doc 21
       - dano em ÁREA (sphere overlap) → doc 18
       - REMOVE State.Combat.InAir (player) + State.Combat.Airborne (alvo)
  5. EndAbility → ambos voltam ao fluxo de chão
```

**Concede/dispara:** **remove** as tags aéreas (encerra o combo de ambos os lados) e dispara o Hard Land + AoE. É o **clímax** — todo o juice se concentra aqui ([16 §5](16_Aerial_Combos.md)), mas o juice em si é do [21](../feel/21_Juice_FX.md).

> 🔒 **Single-player:** o slam usa `RootMotionSource` *descendente* (não o force ancorado da subida) — aqui a posição final é o chão (determinada por colisão), então um impulso forte serve. Sem rede, sem reconciliação de pouso.

---

### ⚡ GA_Dash — dash com i-frames

| Campo | Valor |
|---|---|
| **Input** | `IA_Dash` (Started) → InputID `Dash` |
| **AbilityTag** | `Ability.Dash` |
| **Owned** | `State.Movement.Dashing` |
| **Req** | — |
| **Block** | `Cooldown.Dash` |
| **Cancel** | **`Ability.Attack`** ← dash-cancel **sempre** (escape + fluidez, âncora nº 1) |
| **Custo** | Stamina (se ativa — §5) ou nenhum |
| **Cooldown** | `GE_Cooldown_Dash` (**~0.5–0.8 s** — curto de propósito: o dash é o verbo de sobrevivência, deve sentir "sempre lá"; ver [20 §4.1](../feel/20_Game_Feel.md)) |
| **Montage** | `AM_Dodge` **direcional 8-way** (seções por ângulo, facing travado) — **auto-detect**: clips COM root motion → o **clip dirige a cápsula** (×`RootMotionTranslationScale`); in-place → ConstantForce 550/0.22. Setup: [59](59_Directional_Dodge.md) |
| **Instancing** | `InstancedPerActor` |

**Fluxo de Ability Tasks:**
```
ActivateAbility:
  1. CommitAbility (custo + cooldown)
  2. Aplica GE_Dash_IFrames — i-frame em 2 CAMADAS:
       (a) generoso/front-loaded (~0.2–0.3s, concede State.Movement.Dashing) → PERDÃO ("confio no botão")
       (b) sub-janela "perfect" nos ~6 primeiros frames → recompensa o TIMING
       → dano checa State.Movement.Dashing e IGNORA (doc 18 lê a tag)
  3. RootMotionSource (constant force) na direção do input/cursor
  4. PERFECT-DODGE: se um hit cruzou a sub-janela (b) → GameplayCue.Dodge.Perfect
       + witch-time (SetGlobalTimeDilation ~0.3x + CustomTimeDilation inverso no player; trivial em single-player)
  5. OnRootMotionFinished / Timer → EndAbility (GE de i-frame expira sozinho)
```

**Implementação C++ (notas úteis):**

| Tópico | Decisão DDR | Detalhe |
|---|---|---|
| **Direção** | último input ou forward | `GetCharacterMovement()->GetLastInputVector()`; se ~zero → `GetActorForwardVector()` ([07 §3](../systems/07_Input.md)) |
| **Movimento no tempo** | `RootMotionSource` (canônico) | Distância/duração via atributo ou `SetByCaller` — **não** `SetActorLocation` + `VInterpTo`/`Timeline` (briga com CMC e cancelamentos) |
| **Parâmetros** | `UPROPERTY(EditAnywhere, Category="Dash")` | Distância, duração, cooldown tunáveis no editor/asset — mesma filosofia de data-driven do §4 |
| **Anti-parede** | 🟡 P1 opcional | Antes de aplicar RM: `UKismetSystemLibrary::CapsuleTraceMultiForObjects` na direção → **clamp** da distância se bater ([52 §1](../world/52_Arena_Level_Design.md)). Evita atravessar parede em arenas fechadas |
| **Cargas múltiplas** | ❌ fora do MVP | Hades usa `MaxDashCount`/charges; aqui é **1 dash + cooldown** (~0.5–0.8s). Cargas extras = Eco/meta **pós-MVP** |
| **M0 placeholder** | CMC `TryDash()` | Velocidade + `MOVE_Flying` no [CMC](../../Source/DarkDungeonRift/Movement/DDRCharacterMovementComponent.cpp) — **descartar** ao migrar pro `GA_Dash` ([54 §3.6](../systems/54_M0_Editor_Setup.md)) |

> 📎 **Referência externa (tutorials Hades-style):** sweep de cápsula e interpolação BP são válidos como *ideias*; a **spec canônica** deste projeto é GAS + tags + `RootMotionSource` + perfect-dodge ([56](56_Defensive_Combat.md)). Não duplique lógica de dash no CMC depois do M1.

> ⚔️ **Dash-cancel é o cancelamento-âncora nº 1** ([15 §6](15_Combat_Overview.md)). `CancelAbilitiesWithTag: Ability.Attack` faz o dash **sempre** interromper o combo (chão ou ar) — responsividade e escape. É a regra de cancelamento mais importante do MVP; sem ela o combate sente "preso".

> 🛡️ **i-frames = tag, não código de dano.** `GA_Dash` só **concede** `State.Movement.Dashing` (via GE Duration). Quem *lê* e ignora o dano é o pipeline do [18](18_Combat_System_Deep.md). Isto mantém a ability burra e data-driven — um Eco pode estender a duração do i-frame mexendo no GE, sem tocar a ability.

> 🎛️ **Números de feel do dash são canônicos em [20 §8](../feel/20_Game_Feel.md):** distância **500–650 cm**, duração **0.20–0.25 s**, i-frames **0.20–0.30 s**, cooldown **~0.5–0.8 s**. A ability os consome via `SetByCaller`/atributo (§4) — fonte única, lida por feel **e** ability (um Eco que "buffa o dash" mexe no atributo).

> 🥷 **Perfect-dodge — M1 (Review de Combate, Rha+Vi):** detalhe completo em **[56 — Combate Defensivo](56_Defensive_Combat.md)**. Resumo: um `if` no pipeline de dano ([18 §3](18_Combat_System_Deep.md)) — "hit cruzou sub-janela perfect? → witch-time + cue". Zero sistema novo, reusa o dash do MVP. A cue `GameplayCue.Dodge.Perfect` **já existe** em [21](../feel/21_Juice_FX.md). ⚠️ **Roll está CORTADO do MVP** — uma esquiva só (Dodge→Dash); Roll só volta pós-MVP com assinatura distinta (longo, comprometido, **sem** i-frame frontal). Origem: [Review de Combate](../design/Design_Review_Combat_2026-06.md).

---

## 4. Abilities data-driven (para Os Ecos mexerem)

> 🔑 **Princípio [03b §1](../design/03b_Reward_System.md): "todo Eco transforma o combo."** Para um Eco *modular* uma ability sem reescrevê-la, a ability **não pode ter números hard-coded**. Tudo que um Eco possa querer mexer entra por um destes três canais:

| Canal | Para quê | Como o Eco usa |
|---|---|---|
| **`SetByCaller`** (tag→float) | magnitudes por-ativação (dano, PopHeight, distância do dash) | o GE de dano/efeito tem modifier `SetByCaller`; a ability seta o valor lendo um **atributo** (que o Eco buffa) |
| **Nível da ability** (`AbilityLevel`) | escalonar a ability inteira por tier | `GE`/curvas indexadas por nível; conceder a ability em nível maior = versão mais forte |
| **Atributo lido pela ability** | parâmetros "donos da ability" (cap de juggle, nº de hits) | Eco aplica `GE_RunUpgrade_*` Infinite no atributo (ex.: `MaxJuggleHits`); a ability lê em runtime |

> 🔗 **Defaults canônicos** (`MaxJuggleHits`=**7**, `PopHeightMult`=1.0, `PoiseDamageMult`=1.0, `BonusHitStopFrames`=0) vivem no registro **[05 §3.1](../systems/05_GAS_Architecture.md)**. A ability **lê**; o Eco **modifica**. Aqui não se redefine valor.

```cpp
// Exemplo: PopHeight do re-float NÃO é constante — vem de atributo (Eco-mutável):
const float Pop = ASC->GetNumericAttribute(UDDRAttributeSet::GetJugglePopHeightAttribute());
// aplica RootMotionSource com 'Pop'. Eco "+altura de juggle" = GE Infinite no atributo. Zero edição da ability.

// Exemplo: dano via SetByCaller (magnitude data-driven):
FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(GE_Damage, GetAbilityLevel());
Spec.Data->SetSetByCallerMagnitude(DDRTags.Cost_Stamina /*ou Data.Damage*/, ComputedDamage);
```

> 🧩 **Quem é dono do quê** (evita stacking quebrado, [03b §6](../design/03b_Reward_System.md)): o **cap de hits do juggle pertence à `GA_AirAttack`** (lê `MaxJuggleHits`), **não** a um `GE` empilhável. Ecos ajustam o atributo; a ability é a autoridade única. Regra geral: **estado contínuo/cap → atributo lido pela ability; efeito pontual → `SetByCaller` no GE**.

**Checklist data-driven por ability:** nenhum número mágico no `.cpp`; magnitudes vêm de `SetByCaller` ou atributo; a montage/GE são `EditDefaultsOnly` (trocáveis por subclasse/Eco); o que um Eco "transforma" (ex.: slam→AoE elétrica) é um **GE adicional condicional**, não um `if` na ability.

---

## 5. Custo & recurso (stamina)

🔒 **Single-player pode ser generoso.** Stamina é **opcional no MVP** ([05 §3](../systems/05_GAS_Architecture.md)) — comece **sem** e adicione só se o combate precisar de freio.

| Mecanismo | Como (GAS) | Quando usar |
|---|---|---|
| **Cooldown** (recomendado p/ MVP) | `CooldownGameplayEffectClass` → GE Duration concede `Cooldown.*` → ability tem em `ActivationBlockedTags` | Dash e Launcher (anti-spam). Barato, sem atributo. |
| **Custo de Stamina** (P1) | `CostGameplayEffectClass` → GE Instant com modifier negativo em `Stamina` (`SetByCaller`) | Só se quiser economia de recurso. Dash é o candidato natural. |
| **Sem custo** | nada | Attacks (chão/ar/slam) — o ritmo é governado por montage/decay, não recurso. |

> 💡 **Regra de polegar:** ações **defensivas/de reposicionamento** (dash) podem custar; ações **ofensivas de combo** (attacks) **não** custam — senão o jogador para de combar pra "administrar recurso", matando o pilar. Cooldown no launcher é o único freio ofensivo, e leve.

> ⚙️ `CommitAbility` aplica **custo E cooldown** de uma vez (e falha cedo se não puder pagar). Sempre comite no início da ficha. Sem rede, não há prediction key — o commit é síncrono e local.

---

## 6. Conceder & trocar abilities em runtime (o padrão estável)

> ❓ **Pergunta aberta da revisão (Davi→Mara, [03b §6](../design/03b_Reward_System.md) / [Design Review §6](../design/Design_Review_2026-06.md)):** conceder/trocar/empilhar abilities **20× por run** é estável no Modelo A (ASC no Character)? **Resposta proposta abaixo** — valide num teste isolado no M1.

🔒 **Single-player facilita muito:** sem replicação de specs, sem prediction key ao dar/remover, sem `MarkAbilitySpecDirty`. `GiveAbility`/`ClearAbility` são **operações locais síncronas**. O risco real não é rede — é **leak de spec** e **handle inválido**.

### Padrão DDR: um `UDDREcoComponent` dono dos handles concedidos

```cpp
// Concessão (Eco pego): guarde SEMPRE o handle retornado.
FGameplayAbilitySpecHandle UDDREcoComponent::GrantAbility(TSubclassOf<UGameplayAbility> Ab, int32 Level)
{
    FGameplayAbilitySpec Spec(Ab, Level);                 // InputID opcional aqui
    const FGameplayAbilitySpecHandle H = ASC->GiveAbility(Spec);
    GrantedByEco.Add(H);                                  // rastreia p/ limpeza
    return H;
}

// Troca (Eco que SUBSTITUI uma ability, ex.: "dash causa dano"):
void UDDREcoComponent::ReplaceAbility(FGameplayAbilitySpecHandle Old, TSubclassOf<UGameplayAbility> New)
{
    if (Old.IsValid()) { ASC->ClearAbility(Old); GrantedByEco.Remove(Old); }  // remove a antiga
    GrantAbility(New, 1);                                                       // concede a nova
}

// Fim de run: limpeza determinística (evita leak entre runs).
void UDDREcoComponent::ClearAllGranted()
{
    for (const FGameplayAbilitySpecHandle& H : GrantedByEco)
        if (H.IsValid()) ASC->ClearAbility(H);
    GrantedByEco.Reset();
}
```

| Risco | Sintoma | Mitigação DDR |
|---|---|---|
| **Leak de spec** | abilities acumulam entre runs; "fantasmas" ativam | rastrear **todo** handle em `GrantedByEco`; `ClearAllGranted()` no fim da run (ou recriar o pawn — o roguelike já faz, [05 §2](../systems/05_GAS_Architecture.md)) |
| **`ClearAbility` durante ativação** | ability some no meio do golpe → crash/estado preso | use `SetRemoveAbilityOnEnd()` ou só troque **fora de ativação** (entre salas — o Eco é escolhido na recompensa, não no meio do combo) |
| **Handle inválido reutilizado** | `ActivateAbility` num spec já removido | sempre `H.IsValid()` antes de usar; nunca cache o `FGameplayAbilitySpec*` (ponteiro), só o **handle** |
| **Dois Ecos concedem a mesma ability** | duplicata ativa 2× | antes de `GiveAbility`, cheque `ASC->FindAbilitySpecFromClass(Ab)`; se existe, **suba o nível** em vez de duplicar |

> ✅ **Padrão recomendado:** **conceda/troque Ecos só na tela de recompensa entre salas** (estado seguro, fora de combate). Isso elimina 90% dos riscos (nunca mexe em spec durante ativação). O `UDDREcoComponent` é a **autoridade única** sobre o que foi concedido — nada chama `GiveAbility` direto. Como o pawn é recriado a cada run, mesmo um leak é contido a uma run; ainda assim, `ClearAllGranted()` por higiene.

> 🧪 **Valide no M1** ([Roadmap M1](../17_Implementation_Roadmap.md)): conceda+remova uma ability 30× num loop e confirme — sem crescimento de specs (`ASC->GetActivatableAbilities().Num()` estável), sem warning de handle. Prova o encanamento antes do M4 depender dele.

---

## 7. Abilities "assinatura" (extensibilidade além do MVP)

Para mostrar que o roster **escala** sem reescrever a base — duas ultimates que reusam tudo acima. Não são MVP; são prova de arquitetura.

### 🌩️ GA_Ultimate_Maelstrom — "trovão final" (família Tempestade, [03b §3](../design/03b_Reward_System.md))

| Campo | Valor |
|---|---|
| **AbilityTag** | `Ability.Ultimate.Maelstrom` |
| **Req** | `State.Combat.InAir` (é um finisher aéreo alternativo) |
| **Block** | `Cooldown.Ultimate` |
| **Cancel** | `Ability.Attack.Air` |
| **Cooldown** | `GE_Cooldown_Ultimate` (longo, ~recurso de meta-charge) |

**O que é:** um slam que, em vez de AoE simples, encadeia raios entre **todos** os inimigos `State.Combat.Airborne` próximos (limpa-tela controlado). **Como reusa:** é `GA_AirSlam` + um `GE` condicional extra disparado por tag — exatamente o padrão "Eco transforma o slam" do [03b §4 (sinergia Trovão final)](../design/03b_Reward_System.md). Concedida por meta-progressão ([03b §7](../design/03b_Reward_System.md)) como **escolha desbloqueada**, não percentual.

### 🕳️ GA_Ultimate_Singularity — "vazio" (família Vazio)

| Campo | Valor |
|---|---|
| **AbilityTag** | `Ability.Ultimate.Singularity` |
| **Req** | — (chão ou ar) |
| **Block** | `Cooldown.Ultimate` |
| **Cancel** | `Ability.Attack` |

**O que é:** cria um ponto que **puxa** inimigos próximos (aplica `State.Combat.Airborne` + RootMotionSource convergente) — agrupa o lote pra um combo aéreo coletivo. **Como reusa:** mesma maquinaria do launcher (RootMotionSource + tag `Airborne`), aplicada a vários alvos. Mostra que **launcher é uma primitiva reutilizável**, não um caso único — qualquer ability pode "lançar" via o mesmo contrato do [16 §2](16_Aerial_Combos.md).

> 🎯 **A lição das assinatura:** nenhuma das duas precisou de classe-base nova nem de sistema novo. Tags + RootMotionSource ancorado + GE condicional cobrem ultimates inteiras. **Se uma ability futura não couber nesse molde, é sinal de revisitar a base (§1) — não de criar um sistema paralelo.**

---

## 8. Checklist

- [ ] `UDDRGameplayAbility` base: `InstancedPerActor`, `NetExecutionPolicy = LocalOnly`, helpers (`ApplyEffectToSelf`, `SendLocalGameplayEvent`)
- [ ] Tags registradas nas 4 famílias (`Ability.*`/`State.*`/`Cooldown.*`/`Cost.*`) — [04 §4](../systems/04_Project_Setup.md)
- [ ] 5 fichas MVP completas (Attack_Light, Launcher, AirAttack, AirSlam, Dash) com tags nos slots certos
- [ ] `GA_Dash.CancelAbilitiesWithTag = Ability.Attack` (dash-cancel sempre — âncora nº 1)
- [ ] `GA_Launcher.CancelAbilitiesWithTag = Ability.Attack.Light` (attack→launcher — âncora nº 2)
- [ ] Jump-cancel do AirAttack re-sobe o player (âncora nº 3)
- [ ] `GA_AirAttack`/`GA_AirSlam` com `ActivationRequiredTags = State.Combat.InAir`
- [ ] Launcher usa **RootMotionSource ancorado** (não `LaunchCharacter`) — contrato [16 §2](16_Aerial_Combos.md)
- [ ] Abilities data-driven: zero número mágico; magnitudes via `SetByCaller` / atributo
- [ ] Cap de juggle é dono da **ability** (`MaxJuggleHits`), não de `GE` empilhável
- [ ] Cooldowns via `CooldownGameplayEffectClass`; custo de Stamina só se necessário (P1)
- [ ] `UDDREcoComponent` rastreia handles; `ClearAllGranted()` no fim de run
- [ ] Conceder/trocar Ecos **só na recompensa entre salas** (fora de ativação)
- [ ] Runtime granting validado em loop no M1 (sem leak de spec)

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Air attack sai no chão | Faltou `State.Combat.InAir` em `ActivationRequiredTags` | §3 (ficha AirAttack) |
| Mesma tecla, ataque errado sai | Ambas (chão/ar) concedidas, mas Req não distingue | garanta `InAir` só na aérea; mantenha as duas concedidas (§3) |
| Dash não cancela o combo | `Ability.Attack` ausente em `CancelAbilitiesWithTag` do dash | §3 (ficha Dash) — âncora nº 1 |
| Launcher relança no ar / spam | Faltou `State.Combat.InAir`/`Cooldown.Launcher` em Block | §3 (ficha Launcher) |
| Eco "+dano" não afeta a ability | número hard-coded no `.cpp` em vez de atributo/`SetByCaller` | §4 — mova p/ atributo lido em runtime |
| Abilities "fantasma" entre runs | leak de spec; handles não limpos | §6 — `ClearAllGranted()` + recriar pawn |
| Crash ao pegar Eco no meio do combo | `ClearAbility` durante ativação | §6 — troque só na recompensa (fora de ativação) |
| Cooldown não bloqueia | ability sem `CooldownGameplayEffectClass` ou tag não em Block | §5 + [05 §10](../systems/05_GAS_Architecture.md) |
| Tag não gate corretamente | tag no slot errado (Owned vs Block vs Req) | §2 — tabela de slots |

---

## 10. Próximo passo

→ A **máquina de combo, frame data, hit detection e fórmula de dano** que estas abilities acionam: [18 — Combat System Deep](18_Combat_System_Deep.md). **Defesa** (perfect-dodge, parry): [56 — Combate Defensivo](56_Defensive_Combat.md). O **juice** do impacto/slam: [21 — Juice & FX](../feel/21_Juice_FX.md). O pool de Ecos que concede/transforma estas abilities: [03b — Sistema de Recompensa](../design/03b_Reward_System.md).
