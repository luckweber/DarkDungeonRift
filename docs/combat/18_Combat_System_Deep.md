# 18 — Sistema de Combate (Aprofundamento) · 🟢 P0

> O **simulador de combate**: as *regras* por baixo do combate. Onde o [doc 15](15_Combat_Overview.md) dá a visão geral, este doc abre o capô — o **modelo de dado do golpe**, a **detecção de acerto** profunda, o **pipeline de dano**, a **gramática de cancelamento como máquina de estados**, as **reações do alvo** e o **targeting/soft-lock topdown**. É o contrato que faz o combate ser *legível* e *justo* em topdown.

> 🔒 **Single-player (local-authoritative).** Tudo aqui roda local: hit detection sem reconciliação, sem `FSavedMove`, sem predição. Manipulamos CMC e ASC direto. Onde docs antigos disserem "networked", leia **resolvido: single-player** (ver [Index → premissas](../00_Index.md)).

> **Pré-requisitos:** [Combate (15)](15_Combat_Overview.md) · [Combos Aéreos (16)](16_Aerial_Combos.md) · [GAS (05)](../systems/05_GAS_Architecture.md) · [Input + buffer (07)](../systems/07_Input.md) · [Canais de colisão (04 §5)](../systems/04_Project_Setup.md).

> **Fora de escopo (delegado):** spec de cada ability → [19 — Abilities Deep](19_Abilities_Deep.md) · "feel" de cancelamento/recovery → [20 — Game Feel](../feel/20_Game_Feel.md) · VFX/SFX/screenshake/hitstop-como-espetáculo → [21 — Juice & FX](../feel/21_Juice_FX.md) · matemática do juggle & SM aérea do alvo → [16 §3](16_Aerial_Combos.md).

---

## 1. `FDDRAttackData` — o modelo de dado do golpe 🎯

**O que é:** todo golpe (cada seção de combo, cada ataque aéreo) é descrito por **um dado**, não por código. A ability lê o dado e *obedece*. Isso é "frame data como filosofia": o designer tuna números, não recompila.

**Por que importa no MVP topdown:** combate em topdown se ganha na **leitura** — startup telegrafado, active claro, recovery punível. Centralizar isso num struct deixa *balancear* virar uma planilha, e garante que hitbox/knockback/hitstop de cada golpe sejam **consistentes** entre chão e ar.

```cpp
USTRUCT(BlueprintType)
struct FDDRAttackData
{
    GENERATED_BODY()

    // — Identidade / combo —
    UPROPERTY(EditDefaultsOnly) FGameplayTag AttackId;        // Attack.Light.1, Attack.Air.2...
    UPROPERTY(EditDefaultsOnly) int32        ComboIndex = 0;  // posição na árvore (§4)
    UPROPERTY(EditDefaultsOnly) FName        MontageSection;  // seção da AM_* a tocar

    // — Frame data (a 60fps; 1 frame = ~16.6ms) —
    UPROPERTY(EditDefaultsOnly) int32 StartupFrames  = 6;     // wind-up (telegrafe)
    UPROPERTY(EditDefaultsOnly) int32 ActiveFrames   = 4;     // hitbox LIGADA
    UPROPERTY(EditDefaultsOnly) int32 RecoveryFrames = 12;    // punível se cancelado

    // — Hitbox (forma + offset relativo à arma/socket) —
    UPROPERTY(EditDefaultsOnly) EDDRHitShape HitShape = EDDRHitShape::Capsule;
    UPROPERTY(EditDefaultsOnly) float  HitRadius   = 35.f;
    UPROPERTY(EditDefaultsOnly) float  HitHalfHeight = 60.f;  // capsule/sweep
    UPROPERTY(EditDefaultsOnly) FName  SocketStart = "weapon_start";
    UPROPERTY(EditDefaultsOnly) FName  SocketEnd   = "weapon_end";

    // — Dano / reação —
    UPROPERTY(EditDefaultsOnly) float  BaseDamage   = 10.f;   // pré-AttackPower (§3)
    UPROPERTY(EditDefaultsOnly) float  PoiseDamage  = 20.f;   // quebra de stagger (§5)
    UPROPERTY(EditDefaultsOnly) FVector KnockbackDir = FVector(1,0,0); // local; X=frente
    UPROPERTY(EditDefaultsOnly) float  KnockbackForce = 400.f;
    UPROPERTY(EditDefaultsOnly) int32  HitStopFrames = 2;     // freeze global (§3 / doc15 §5)
    UPROPERTY(EditDefaultsOnly) EDDRHitReact ReactType = EDDRHitReact::Light;

    // — Aéreo —
    UPROPERTY(EditDefaultsOnly) bool   bIsLauncher  = false;  // inicia juggle (doc16 §2)
    UPROPERTY(EditDefaultsOnly) float  LaunchHeight = 0.f;    // alvo da RootMotionSource
    UPROPERTY(EditDefaultsOnly) bool   bReFloat     = false;  // re-flutua alvo no ar (doc16 §3)

    // — Multi-hit —
    UPROPERTY(EditDefaultsOnly) int32  MaxHitsPerTarget = 1;  // >1 = golpe multi-hit (§2)
};
```

**Onde mora:** um `UDataAsset` (`UDDRAttackSet`) com um `TArray<FDDRAttackData>` por arma/move-set; ou linhas de `DataTable` indexadas por `AttackId`. Soft-load via Asset Manager ([ue-data-assets-tables]). A ability recebe o set e indexa por `ComboIndex`.

### Frame data → AnimNotify (o mapa) 🥊

O struct é a *verdade*; a montage é a *execução temporal*. Casamos os dois assim:

| Campo do struct | Vira na montage | Notas |
|---|---|---|
| `StartupFrames` | offset até o **início** do `ANS_Hitbox` | parte "telegrafada" — leitura topdown |
| `ActiveFrames` | **duração** do `ANS_Hitbox` | hitbox ligada só aqui |
| `RecoveryFrames` | cauda após `ANS_Hitbox` até o fim/próx. seção | janela em que cancelar "salva" o recovery (§4) |
| janela de combo | `ANS_ComboWindow` (doc 15 §3) | aceita próximo input (buffer §4) |
| janelas de cancel | `ANS_CancelWindow` (tag de gate, §4) | liga flag "cancelável-para-X" |

> ⚠️ **Fonte única de verdade.** Se o notify na montage e o `FDDRAttackData` divergirem, *o que liga a hitbox é o notify* — então use o frame data como spec e **autore os notifies a partir dele** (idealmente uma ferramenta de editor que escreve os notifies pelo struct, ver [ue-editor-tools]). Divergência silenciosa = bug de "hitbox sai antes do swing".

---

## 2. Hit detection profundo ✅

**O que é:** descobrir *o que o golpe acertou*, sem hit duplo, sem acertar quem não devia. Duas técnicas; escolha por golpe via `EDDRHitShape`.

| Técnica | Como | Quando | Custo |
|---|---|---|---|
| **Sweep entre sockets** (recomendado) | a cada `NotifyTick`, `SweepMultiByChannel` da pose anterior do socket → pose atual (`weapon_start`→`weapon_end`) | melee preciso de lâmina; evita "tunneling" em frames rápidos | médio |
| **Overlap de volume** | um `UShapeComponent` na arma liga/desliga | golpes lentos/contundentes; AoE estacionário | baixo |
| **Sphere no impacto** (AoE) | `OverlapMultiByChannel` num ponto no notify de impacto | slam/finisher (doc 16 §5) | baixo, pontual |

### Sweep: o algoritmo (anti-tunneling) 🔥

```
ANS_Hitbox.NotifyBegin:
  AlreadyHit.Reset()                          // limpa lista DESTE swing
  PrevStart = Mesh->GetSocketLocation(SocketStart)
  PrevEnd   = Mesh->GetSocketLocation(SocketEnd)

ANS_Hitbox.NotifyTick(dt):
  CurStart = socket(SocketStart); CurEnd = socket(SocketEnd)
  // varre o "leque" do swing entre o frame anterior e o atual:
  for amostra t in [0..1] (ex.: 3-5 amostras p/ swings rápidos):
     A = lerp(PrevStart, CurStart, t); B = lerp(PrevEnd, CurEnd, t)
     SweepMultiByChannel(hits, A, B, FQuat, ECC_Hitbox,
                         FCollisionShape::Capsule(HitRadius, HitHalfHeight))
     for hit in hits:
        Actor = hit.GetActor()
        if !IsHittable(Actor) continue                 // §2.4
        if AlreadyHit[Actor] >= MaxHitsPerTarget continue   // hit-once-per-swing
        AlreadyHit[Actor]++
        SendGameplayEvent(self, Combat.Hit, {Target=Actor, Hit=hit, AttackData})
  PrevStart = CurStart; PrevEnd = CurEnd

ANS_Hitbox.NotifyEnd:
  (nada — AlreadyHit persiste até o próximo NotifyBegin = próximo swing)
```

> 🎯 **Por que sweep e não overlap por frame.** Um swing de 4 frames (`ActiveFrames=4`) move a ponta da lâmina ~150cm; um inimigo fino *entre* dois frames de overlap é atravessado sem detecção ("tunneling"). O sweep da-pose-anterior-à-atual cobre o gap. As 3-5 amostras intermediárias cobrem rotação rápida (o leque do arco), onde o sweep linear ainda fura.

### 2.1 Multi-hit & "hit-once-per-swing"

- **`AlreadyHit`** é um `TMap<TObjectPtr<AActor>, int32>` (alvo → vezes atingido) **por swing**, resetado no `NotifyBegin`. Sem ele: o tick acerta o mesmo alvo todo frame → 4 hits num swing de 1.
- **Multi-hit intencional** (`MaxHitsPerTarget > 1`): broca/serra que *deve* bater N vezes. O contador permite até N; combine com um cooldown curto (`HitInterval`) pra espaçar (senão sai N no mesmo frame).
- **Reset entre seções:** cada seção de combo é um swing novo → `NotifyBegin` zera. Isso é o que deixa o combo de 4 golpes acertar 4×, mas cada golpe individual acertar 1×.

### 2.2 Canais Hitbox / Hurtbox ([doc 04 §5](../systems/04_Project_Setup.md))

| Volume | Em quem | Object channel | Responde a (trace) |
|---|---|---|---|
| **Hitbox** (arma) | atacante (sweep) | — (é o *tracer*) | testa contra `ECC_Hurtbox` |
| **Hurtbox** | corpo do alvo (capsule/bones) | `ECC_Hurtbox` | bloqueia/overlap o sweep de Hitbox |
| **Player body** | player | `ECC_Player` | navegação/empurrão, **não** dano |
| **Enemy body** | inimigo | `ECC_Enemy` | idem |

> 🔑 **Hitbox ≠ corpo físico.** O sweep usa `ECC_Hitbox` mirando **só** `ECC_Hurtbox` (Overlap), ignorando `ECC_Player/Enemy` (capsules de movimento). Assim o golpe não "engata" na capsule de colisão nem empurra fisicamente — ele *só pergunta "tem hurtbox aqui?"*. Hurtbox pode ser uma capsule dedicada ou per-bone (`PhysicsAsset`) para acerto justo em poses extremas.

### 2.3 Hitbox vs Hurtbox: a regra de leitura topdown

Mantenha **Hurtbox um pouco generosa** (raio ≥ silhueta) e **Hitbox honesta** (cola no arco da arma). Em topdown a perspectiva achata a profundidade; uma hurtbox generosa compensa a ambiguidade visual de "acertei?" sem deixar o golpe parecer injusto. O oposto (hitbox generosa) gera "acertos fantasma" que o jogador não entende.

### 2.4 `IsHittable(Actor)` — o que pode ser atingido

```
IsHittable(Actor):
  return Actor != Instigator                       // não auto-acerto
      && Actor implements IAbilitySystemInterface  // tem ASC
      && ASC->HasMatchingGameplayTag(Faction.Enemy) // facção hostil
      && !ASC->HasMatchingGameplayTag(State.Dead)   // já morto = ignore
      && !ASC->HasMatchingGameplayTag(State.Invulnerable) // i-frames/dash
```

Friendly-fire **off** no MVP (single-player, sem co-op). Destrutíveis/props: dêem ASC mínimo + `Faction.Neutral` se quiser que recebam dano, ou trate por `UGameplayStatics::ApplyDamage` separado.

---

## 3. Pipeline de dano (a fórmula) 🎯

**O que é:** do "acertei o alvo" até "o HP caiu". O **encanamento GAS fino** (meta-attribute, `PostGameplayEffectExecute`) vive no [doc 05 §3](../systems/05_GAS_Architecture.md) — aqui está a **fórmula** e a *ordem*.

```
Combat.Hit event (do §2) → ability monta o GE_Damage:
  1. SetByCaller: Damage = AttackData.BaseDamage           (do struct §1)
  2. ASC->ApplyGameplayEffectToTarget(GE_Damage, TargetASC)
  3. GE escreve no meta-attribute IncomingDamage do ALVO
  4. ALVO->PostGameplayEffectExecute  resolve a fórmula → Health -= Final
```

### A fórmula (ponto de partida — tune)

```
Raw      = BaseDamage * (AttackPower / 100)              // escala do atacante
Crit     = (rand() < CritChance) ? Raw * CritMultiplier : Raw   // CritMult ~1.75
Mitig    = Crit * (100 / (100 + Armor))                 // armor = redução com retornos decrescentes
Final    = max(1, round(Mitig))                         // dano mínimo 1 (sempre "conta")
Health  -= Final
```

| Termo | Fonte | Valor inicial | Nota |
|---|---|---|---|
| `BaseDamage` | `FDDRAttackData` | por golpe | finisher > jab |
| `AttackPower` | AttributeSet (doc 05 §3) | 100 (base) | upgrades de run escalam (×%) |
| `CritChance` | AttributeSet (run-scoped) | 0% base | Ecos/build sobem |
| `CritMultiplier` | AttributeSet/const | 1.75 | |
| `Armor` | AttributeSet do alvo | por inimigo | `100/(100+Armor)`: 100 armor = -50% |

> ⚠️ **Por que `100/(100+Armor)` e não subtração plana.** Subtração (`dano - armor`) cria breakpoints binários: armor alto **zera** golpes fracos (multi-hit aéreo morre) e quase não afeta golpes fortes. A curva multiplicativa dá **retornos decrescentes suaves** — sempre passa *algum* dano, o que mantém o juggle (muitos hits pequenos) viável. `max(1, ...)` garante que nenhum hit seja "0 dano fantasma".

> 🥊 **Hit-stop e dano são coisas separadas.** `HitStopFrames` (struct) dispara o **freeze global** ([doc 15 §5](15_Combat_Overview.md): congela CMC do atacante *e* do alvo por N frames via `CustomTimeDilation`/pausa), e o `GE_Damage` aplica o número. No aéreo, o **freeze acontece primeiro, o re-float/knockback depois** (doc 16 §3) — senão o pop "vaza" durante o congelamento.

---

## 4. Sistema de combo profundo + gramática de cancelamento 🥋

**O que é:** o combo não é uma fila linear — é um **grafo** com ramos (light-light-**heavy** ≠ light-light-**light**), e por cima dele a **gramática de cancelamento**: *o que interrompe o quê, a partir de qual janela, pra qual estado*. Isto **É** o jogo character-action (revisão de design — Kael, P0 no M1).

**Por que importa no MVP topdown:** sem ramos + cancelamento o combate é *on-rails* — bonito 5 min, raso pra sempre. O grafo dá expressão; a gramática dá skill ceiling. Em topdown, onde não há profundidade de câmera pra "espetáculo", a **profundidade vem da mão** do jogador.

### 4.1 Grafo de combo

```
            ┌─(Light)→ L1 ─(Light)→ L2 ─(Light)→ L3 ─(Light)→ L4 (finisher)
   Idle ────┤                    └─(Heavy)→ LH (lança? → §5/doc16)
            ├─(Heavy)→ H1 ─(Heavy)→ H2 (slam de chão)
            └─(Launcher)→ → entra no fluxo aéreo (doc 16)
```

Cada nó = um `FDDRAttackData` (índice `ComboIndex`). A aresta tomada depende de **qual input** chega na janela de combo. Implementação: um `TMap<FGameplayTag /*input*/, int32 /*próximo ComboIndex*/>` por nó, **ou** um `UDataAsset` de grafo (`UDDRComboGraph`). O `UDDRComboComponent` ([doc 04 §3.1](../systems/04_Project_Setup.md)) guarda o `CurrentNode` e resolve a transição.

```
NaJanelaDeCombo, ao consumir input bufferizado (doc 07 §5):
  Next = CurrentNode.Branches[BufferedInputTag]
  if Next válido: MontageJumpToSection(AttackSet[Next].MontageSection); CurrentNode = Next
  else: (input não-mapeado) ignora; deixa o golpe terminar
SemInput ao fim da janela: reseta CurrentNode = Idle, EndAbility
```

### 4.2 Gramática de cancelamento — a máquina de estados (estado × ação × janela)

Esta expande a tabela do [doc 15 §6](15_Combat_Overview.md) numa SM completa do **atacante** (player). Cada *estado* tem janelas; cada *ação* só sai se sua janela estiver aberta.

```
            dash(✅sempre)
   ┌──────────────────────────────────────────────┐
   ▼                                               │
[Idle/Locomoção] ──attack──► [Startup] ──► [Active] ──► [Recovery] ──(fim)──► [Idle]
                                 │            │              │
              launcher(❌)       │  combo(❌) │  combo(⏳jab)│ combo(✅ encadeia)
              cancel:            │  cancel:   │  cancel:     │ cancel:
                                 ▼            ▼              ▼
                          (comprometido)  dash(✅)        dash(✅) launcher(✅)
                                          launcher(⏳)    jump(⏳)
```

| Estado ↓ \ Ação → | **Dash** | **Próx. ataque** | **Launcher** | **Jump/Air** | Janela (notify) |
|---|:---:|:---:|:---:|:---:|---|
| **Startup** (wind-up) | ✅ escape | ❌ | ❌ | ❌ | — (comprometendo) |
| **Active** (hitbox on) | ✅ escape | ⏳ só se hit | ⏳ se hit | ❌ | `ANS_CancelWindow` abre no *contato* |
| **Recovery** | ✅ sempre | ✅ encadeia | ✅ | ⏳ | `ANS_ComboWindow` + `ANS_CancelWindow` |
| **AirActive** (aéreo) | ⏳ air-dash | ⏳ janela aérea | — | ✅ **jump-cancel→re-sobe** | doc 16 §4 |
| **Slam** (descida) | ❌ | ❌ | ❌ | ❌ | comprometido (doc 16 §5) |

Legenda: ✅ sempre · ⏳ só na janela · ❌ bloqueado · — n/a. **Valores iniciais — tune no M1.**

> 🔑 **Os 3 cancelamentos-âncora do MVP** (mínimo pra "sentir" character-action):
> 1. **Dash-cancel** de qualquer estado terrestre (sempre) → responsividade + escape.
> 2. **Attack→Launcher** na janela de Active/Recovery → a ponte chão↔ar (alimenta o pilar).
> 3. **Jump-cancel do AirAttack** → re-sobe o player no juggle (o "truque" do aéreo profundo, doc 16).

> 💡 **"Cancel on hit" vs "cancel on whiff".** A coluna ⏳ "só se hit" no Active modela o clássico: você só pode cancelar o golpe num próximo se **acertou** (`AlreadyHit` não-vazia). Whiff (errou) → tem que comer o recovery. Isso recompensa precisão e pune flailing — é o coração da economia de risco character-action.

### 4.3 Como o GAS implementa o cancelamento

- **Cancelamento "sempre" (dash):** `GA_Dash` lista `Ability.Attack` em **`CancelAbilitiesWithTag`** → o dash mata o ataque ativo de qualquer estado. ([doc 05 §4](../systems/05_GAS_Architecture.md)).
- **Cancelamento "só na janela" (⏳):** um `ANS_CancelWindow` liga uma tag `State.Combat.CancelableInto.<X>` enquanto aberto. O input buffer (doc 07 §5), ao consumir, **só ativa** a ability X se a tag de gate correspondente estiver presente. Fora da janela → input fica no buffer (e expira) ou é descartado.
- **"On hit gate":** o `ANS_CancelWindow` do Active só *abre de fato* quando o `Combat.Hit` daquele swing disparou (checa `AlreadyHit.Num()>0`). Sem hit → janela fica fechada → sem cancel.

> 🐞 **`ddr.CombatDebug 1`** (espírito do `ddr.LocomotionDebug`, doc 08 §5; ver também [20 — Game Feel](../feel/20_Game_Feel.md)): desenhe em tela **estado atual, nó de combo, janelas abertas (combo/cancel-into-X), e o último input bufferizado + idade**. Tunar cancelamento sem ver as janelas é às cegas.

---

## 5. Reações do alvo (hit react, poise, stagger, launch) ⚠️

**O que é:** como o inimigo *responde* ao ser atingido — a metade que transforma "número caiu" em "senti que bati". Cobre hit reactions, **poise/stagger/hyperarmor**, knockback, e a **elegibilidade de launch** que abre o gancho pro aéreo.

**Por que importa no MVP topdown:** a reação é o *feedback de leitura* — em topdown, o flinch/stagger do inimigo é metade do "juice" que diz "acertou". E a regra de poise é o que decide se o combo **flui** ou é interrompido pelo inimigo revidando.

### 5.1 Hit reactions (por `ReactType`)

| `EDDRHitReact` | Quando | Efeito |
|---|---|---|
| **Light** | jab, hit fraco | micro-flinch (montage curta no Slot upper), não interrompe a IA por muito |
| **Heavy** | golpe forte | flinch completo, interrompe ação atual da IA |
| **Stagger** | poise quebrado (§5.2) | reação longa, **vulnerável** (janela de combo garantida) |
| **Knockback** | golpe com `KnockbackForce` alto | desliza/cai pra trás (§5.3) |
| **Launch** | `bIsLauncher` + elegível (§5.4) | entra na **SM aérea do alvo** (§5.5 / doc 16 §3.1) |

A reação toca uma montage de flinch no ASC do alvo (via `GameplayCue` ou `PlayMontage` direto no alvo). Direção do flinch = ângulo `atacante→alvo` (4-direcional: front/back/L/R) pra leitura topdown correta.

### 5.2 Poise / Stagger / Hyperarmor 🔥

O sistema que decide *se o inimigo pode ser interrompido agora*. Modelo (Souls-like, simplificado):

```
Alvo tem atributo Poise (ex.: 100) e PoiseMax.
Ao tomar hit:  Poise -= AttackData.PoiseDamage
  se Poise <= 0:
     → ReactType = Stagger (reação longa, vulnerável)
     → Poise = PoiseMax (recarrega); inicia cooldown de regen
  senão:
     → reação normal (Light/Heavy conforme o golpe), IA pode revidar
Poise REGENERA (ex.: +20/s) após PoiseRegenDelay (~1.5s) sem tomar hit.
```

| Conceito | Mecânica | Uso |
|---|---|---|
| **Poise** | "vida de stagger"; zera → stagger | inimigos grandes = poise alto (custam combo pra abrir) |
| **Stagger** | estado vulnerável pós-quebra | janela garantida pro player encadear/launchar |
| **Hyperarmor** | tag `State.Combat.HyperArmor` no inimigo durante certos golpes → **ignora flinch** (ainda toma dano) | telegrafe de chefe; ensina o player a respeitar |

> 🎯 **Poise é o ritmo do combate topdown.** Sem poise, ou todo hit trava o inimigo (combo trivial, sem risco) ou nenhum trava (player nunca tem turno). Poise dá o *toma-lá-dá-cá*: golpes acumulam até **abrir** o inimigo, e aí o combo aéreo flui. É o que faz "quebrar a guarda" sentir uma conquista.

> ⚠️ **Hyperarmor ≠ invulnerável.** Hyperarmor ignora a *reação* (não cambaleia), mas o dano e o hitstop continuam. É telegrafe de "este golpe vai sair, recue" — não um escudo. Hitstop no acerto contra hyperarmor é o tell visual.

### 5.3 Knockback

`KnockbackDir` (local ao atacante; X=frente) × `KnockbackForce`, aplicado ao alvo via **RootMotionSource** (não `LaunchCharacter` — single-player ainda quer trajetória previsível e que respeite o freeze global). Knockback forte que bate em parede pode disparar um *wall-splat* (reação extra) — P1, anota pra [21 — Juice & FX](../feel/21_Juice_FX.md).

### 5.4 Elegibilidade de launch (o gate do aéreo)

Nem todo inimigo deve voar a qualquer momento. O launcher (`bIsLauncher`) só inicia juggle se:

```
CanLaunch(Target, AttackData):
  return AttackData.bIsLauncher
      && !Target.HasTag(State.Combat.Airborne)        // já no ar? (re-float é outra coisa, doc16 §3)
      && !Target.HasTag(Faction.Boss.NoLaunch)        // chefes/elites podem ser imunes
      && Target.Poise <= 0  OU  AttackData.bForceLaunch // exige guarda quebrada (recomendado)
      && Target.Mass <= LaunchMassCap                  // "tonelagem": inimigo gigante não voa
```

> 🥊 **Recomendado: launcher exige poise quebrado** (`Poise<=0`). Isso amarra os dois sistemas — você *trabalha* o inimigo no chão (combo) até abrir, **então** lança. Lançar à vontade trivializa; lançar só após quebrar dá arco dramático e ensina o combo. `bForceLaunch` fica pra launchers especiais/Ecos.

### 5.5 Gancho para a SM aérea do alvo

Quando `CanLaunch` passa, o launcher **não** usa `bIsFalling` — aplica a tag `State.Combat.Airborne` e entrega o alvo à sua **state machine própria** (Knockback→Float→SlamDown→Stun). A matemática (PopHeight, decay, hang-time, cap de hits) e a SM **são do [doc 16 §3 e §3.1](16_Aerial_Combos.md)** — não repito aqui. Este doc só garante o *gate* e o *handoff*.

### 5.6 Player flinch & defesa que preserva o combo · 🟡 P1 *(Review de Combate — Rha)*

Tudo acima é reação do **alvo**. O counterplay só fecha se o **player também reage** — senão a defesa não tem sentido dos dois lados.

- **Player flinch / hitreact:** quando o player toma hit, **interrompe o combo** → pune ganância (atacar quando devia defender). Usa os 49 clips de Hit do set ([22](../22_Animation_Inventory.md)). **Calibre por peso:** hit leve = micro-flinch/hitstop (não tira controle); pesado/stagger = interrupção real. Flinch demais = "não tenho controle"; de menos = player vira imune e a defesa perde sentido. Hyperarmor do player (ex.: no slam) é a exceção telegrafada.
- **Dodge-offset:** hoje **toda** janela de cancelamento é *ofensiva* (§4.2) → defender **sempre quebra** o combo, então o jogador escolhe **nunca defender**. O **dodge-offset** (estilo Bayonetta) é uma janela onde **esquivar no meio do combo NÃO derruba o combo** — preserva o pilar e dá resposta ativa ao hyperarmor (§5.2). É o que falta pro combate ser *dança*, não *agressão com escape*.

> 🥷 Conecta com o **perfect-dodge** ([19 GA_Dash](19_Abilities_Deep.md), M1): dodge que preserva o combo + sub-janela *perfect* (witch-time) = a camada defensiva inteira por quase nenhum custo. Origem: [Review de Combate](../design/Design_Review_Combat_2026-06.md).

---

## 6. Targeting / soft-lock / aim-assist topdown 🎯

**O que é:** como o ataque **encara o inimigo certo** e *alcança* ele — sem hard-lock (que engessa um ARPG topdown). Faz acertar parecer **certeiro** mesmo o player sendo orient-to-movement.

**Por que importa no MVP topdown:** o personagem é [orient-to-movement (Stellar-Blade-like; ver Index/projeto)](../00_Index.md), então o "pra onde olho" ≠ "pra onde ataco" por padrão. Sem soft-lock, o golpe sai na direção do *stick*, não do *inimigo* — e em topdown o player erra alvos óbvios. Soft-lock + motion-warp resolvem isso *sem* tirar o controle.

### 6.1 Seleção de alvo (soft-lock, no início da ability)

```
PickSoftLockTarget(Instigator, AttackData):
  Cands = OverlapMulti(sphere R=SoftLockRange ~250cm, ECC_Enemy) filtrado por IsHittable (§2.4)
  para cada Cand, score:
     dist   = |Cand - Instigator|
     facing = dot( AimDir, normalize(Cand - Instigator) )   // 1 = à frente
     score  = facing * FacingWeight - dist * DistWeight
  descarta Cands com facing < cos(SoftLockHalfAngle ~60°)    // só o cone à frente
  return argmax(score)   // ou null → ataque sai "reto" no AimDir
```

`AimDir` = direção do input de movimento (Modelo A) **ou** cursor/right-stick (Modelo B), conforme [doc 07 §1](../systems/07_Input.md). O cone (~120° total) + peso de distância dá o clássico "ele mira o que eu *claramente* quis acertar".

### 6.2 Faceamento do ataque (com orient-to-movement)

```
No ActivateAbility, ANTES do swing:
  Target = PickSoftLockTarget(...)
  if Target:
     DesiredYaw = (Target.Loc - self.Loc).Rotation().Yaw
     RInterpTo(self yaw → DesiredYaw, AttackTurnSpeed)   // snap suave 1-2 frames
```

> 🎯 **Snap só no startup, nunca no active.** Vire o personagem pro alvo durante os `StartupFrames` (telegrafe), **não** durante `ActiveFrames`. Girar com a hitbox ligada = "homing slash" que pune o jogador por mirar e tira a leitura ("por que acertei aquilo?"). Vire antes, comprometa o arco depois. Em topdown isso é a diferença entre "responsivo" e "auto-aim injusto".

### 6.3 Motion-warp-to-target (alcance "certeiro")

Para o golpe **chegar** ao alvo (não bater no vazio), use `MotionWarping` (plugin do [doc 04 §1](../systems/04_Project_Setup.md)):

```
Se Target e dist > ReachThreshold (e dist < MaxWarpDist ~200cm):
  WarpPoint = Target.Loc - AimDir * IdealHitDistance   // para "na cara" do alvo
  MotionWarping->AddOrUpdateWarpTargetFromLocation("AttackWarp", WarpPoint, FacingRot)
  // a montage tem um Motion Warp window que desliza o personagem até lá durante o swing
```

| Parâmetro | Spec (design) | **Default C++** (`UDDRCombatComponent`) |
|---|---|---|
| `SoftLockRange` | ~250 cm | `SoftLockRadius` = **600** (tune no BP; topdown precisa de cone generoso) |
| `SoftLockHalfAngle` | ~60° (120° total) | `SoftLockHalfAngleDegrees` = **75** |
| `IdealHitDistance` | ~90 cm | `IdealHitDistance` = **90** |
| `MaxWarpDist` (ground) | ~200 cm | `MaxWarpDistance` = **200** |
| `MaxWarpDist` (air) | menor | `MaxWarpDistanceAir` = **120** |
| `MaxWarpDist` (launcher) | curto horizontal | `MaxWarpDistanceLauncher` = **180** |
| `SoftLockMaxVerticalGap` | filtro ΔZ | **220** cm — ignora inimigo no céu enquanto você está no chão |
| `AttackTurnSpeed` | ~720°/s | snap **instantâneo** no startup (yaw direto; tune futuro) |

**Implementação C++ (notas úteis):**

| Tópico | Onde | Detalhe |
|---|---|---|
| Soft-lock | `FindSoftLockTarget` | cone na **intenção** (input > facing); fallback = mais próximo no raio |
| Filtro vertical | `SoftLockMaxVerticalGap` | descarta alvos com \|ΔZ\| > cap — evita lock no juggle antigo no céu |
| Airborne priority | `bPreferAirborne` | `GA_AirAttack` / `GA_AirSlam` somam +1000 no score do alvo `Airborne` |
| Tuning aéreo | `GA_Launcher` / `GA_AirAttack` | `LaunchRiseHeight`, `JuggleTargetHeightAbovePlayer`, `AirPopVerticalNudgeScale` — [60 §2/§5](../systems/60_M2_Editor_Setup.md) |
| Face + warp | `FaceAndSetupMotionWarp` | chame **antes** de `PlayMontageAndWait`; perfil via `EDDRMotionWarpProfile` |
| Warp target | `DDRMotionWarpNames::AttackWarp` | montage precisa de notify **Motion Warping** com esse nome |
| Combo seccionado | `UGA_AttackLight::AdvanceCombo` | C++ recalcula `AttackWarp` a cada seção; **`AM_Combo` = 1 janela por seção** (Atk1–4) — [57 §2b](57_M1_Combo_Editor_Setup.md) |
| Componente | `UMotionWarpingComponent` | em `ADDRCharacterBase`; `bSearchForWindowsInAnimsWithinMontages = true` |
| Perfis | `GA_AttackLight` / `GA_AirAttack` / `GA_Launcher` / `GA_AirSlam` | Ground, Air, Launcher, Slam — ver [60 §7.2](../systems/60_M2_Editor_Setup.md) |
| Debug | `ddr.CombatDebug 1` | linha ciano (soft-lock) + esfera magenta (warp point) |

> ⚠️ **Cap no warp = honestidade.** Warp ilimitado vira teleporte e quebra o espaço do combate (player "gruda" em inimigos longe). O `MaxWarpDist` garante que warp é *fechar a última distância*, não *atravessar a sala*. Fora do cap, o golpe erra — e errar tem que ser possível pro acerto ter valor.

**Pins do notify Motion Warping (editor):** target **`AttackWarp`** · Translation ✅ · Ignore Z ✅ · **Warp Rotation ❌** (face no startup via C++; se ligar rotation, use Type = `Default` apenas). Setup: [60 §7.3](../systems/60_M2_Editor_Setup.md).

> 🎮 **Como acertar fica certeiro:** soft-lock escolhe o alvo óbvio → faceamento vira o personagem no startup → motion-warp fecha o gap residual → sweep (§2) confirma o hit. As 4 camadas juntas dão o feel "minha intenção virou acerto" sem hard-lock e sem tirar a mão do jogador. **Setup no editor:** [60 §7](../systems/60_M2_Editor_Setup.md) · perfis por ability em [19 §3](19_Abilities_Deep.md).

---

## 7. Checklist ✅

- [ ] `FDDRAttackData` (struct) + `UDDRAttackSet` (DataAsset) com frame data por golpe
- [ ] Notifies (`ANS_Hitbox`/`ANS_ComboWindow`/`ANS_CancelWindow`) **derivados** do frame data
- [ ] Sweep entre sockets (anti-tunneling, 3-5 amostras) ligado em `ANS_Hitbox`
- [ ] `AlreadyHit` (map por swing) — hit-once-per-swing + multi-hit opt-in
- [ ] Canais `ECC_Hitbox`/`ECC_Hurtbox` configurados; Hitbox testa só Hurtbox
- [ ] `IsHittable` (facção, dead, invuln, self) filtrando hits
- [ ] `GE_Damage` via meta-attribute `IncomingDamage`; fórmula `AP%`+crit+`100/(100+Armor)`+`max(1)`
- [ ] Grafo de combo (`UDDRComboGraph`) com ramos light/heavy + `UDDRComboComponent`
- [ ] SM de cancelamento (estado×ação×janela) + os 3 cancel-âncora (dash/launcher/jump-cancel)
- [ ] "Cancel on hit" gate (Active só cancela se `AlreadyHit` não-vazia)
- [ ] Hit reactions por `ReactType` (direção 4-way) + poise/stagger/hyperarmor
- [ ] `CanLaunch` gate (poise quebrado + mass cap + não-Airborne) → handoff p/ SM do alvo (doc 16)
- [x] Soft-lock (cone+score) → faceamento **no startup** → motion-warp com `MaxWarpDist` — ✅ C++ M1/M2; janelas `AttackWarp` no editor → [60 §7](../systems/60_M2_Editor_Setup.md)
- [ ] `ddr.CombatDebug 1` mostrando estado/nó/janelas/buffer

---

## 8. Troubleshooting 🐞

| Sintoma | Causa | Fix |
|---|---|---|
| Hitbox liga antes do swing aparecer | `StartupFrames` do struct ≠ offset do notify | §1 — autore notify pelo frame data |
| Golpe rápido atravessa inimigo fino | Overlap por frame (tunneling) | §2 — sweep pose-anterior→atual + amostras |
| Acerta N× num swing só | Sem `AlreadyHit`, ou resetando no tick errado | §2.1 — reset só no `NotifyBegin` |
| Golpe "engata" / empurra fisicamente | Hitbox mirando `ECC_Player/Enemy` em vez de só `Hurtbox` | §2.2 — canal correto |
| Armor zera golpes fracos (juggle morre) | Mitigação por subtração plana | §3 — use `100/(100+Armor)` + `max(1)` |
| Combo só linear, sem ramos | Sem grafo; `MontageJumpToSection` fixo | §4.1 — `UDDRComboGraph` por input |
| Cancela golpe que **errou** | Sem "on hit" gate | §4.2/§4.3 — janela só abre com `AlreadyHit>0` |
| Inimigo trava a cada hit (sem risco) | Sem poise / poise baixo demais | §5.2 — dê poise + regen |
| Inimigo nunca cambaleia | Hyperarmor permanente / poise infinito | §5.2 — hyperarmor é só durante golpes |
| Lança inimigo a qualquer hora (trivial) | `CanLaunch` sem gate de poise | §5.4 — exija `Poise<=0` |
| Golpe sai na direção do stick, ignora alvo óbvio | Soft-lock desligado / cone estreito | §6.1/§6.2; tune `SoftLockRadius` no BP — [60 §7.1](../systems/60_M2_Editor_Setup.md) |
| Bate no vazio perto do alvo | Falta notify Motion Warping / `AttackWarp` errado | §6.3 — [60 §7.3](../systems/60_M2_Editor_Setup.md) |
| Atk1 alcança, Atk2+ não | Warp só na 1ª seção do combo | [57 §2b](57_M1_Combo_Editor_Setup.md) — replique o notify em cada seção |
| "Homing slash" / auto-aim injusto | Faceamento rodando no `Active` | §6.2 — vire só no `Startup` (já assim no C++) |
| Personagem "teleporta" pra inimigo longe | Warp sem cap | §6.3 — `MaxWarpDistance` no Combat Component |
| Olha + debug magenta, sem lunge | `Warp Target Name` = `None` | [60 §7.3](../systems/60_M2_Editor_Setup.md) — **`AttackWarp`** |
| Rotação estranha no swing | `Warp Rotation` ON na montage | §6.2 + [60 §7.3](../systems/60_M2_Editor_Setup.md) — desligue; face = startup C++ |
| Soft-lock no inimigo no céu (você no chão) | sem filtro de ΔZ | `SoftLockMaxVerticalGap` (~220) no Combat Component |
| Inimigo sobe demais no combo aéreo | `AirPop` stackava altura | tune `BP_GA_AirAttack` → Juggle Target Height / Air Pop Nudge Scale — [16 §3](16_Aerial_Combos.md) |
| Player teleporta para baixo após launcher | `AirAnchorZ` puxava pro inimigo | recompile — ancora no pulo do player; tune `BP_GA_Launcher` — [60 §2](../systems/60_M2_Editor_Setup.md) |

---

## 9. Próximo passo

→ Spec de cada ability (frame data real, warp por golpe, cooldowns): [19 — Abilities Deep](19_Abilities_Deep.md).
→ "Feel" dos cancelamentos & recovery (responsividade, buffer leniency, debug): [20 — Game Feel](../feel/20_Game_Feel.md).
→ Hitstop-como-espetáculo, screenshake, VFX/SFX de impacto: [21 — Juice & FX](../feel/21_Juice_FX.md).
→ A matemática do juggle e a SM aérea do alvo (gancho do §5.5): [16 §3](16_Aerial_Combos.md).
