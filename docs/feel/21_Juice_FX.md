# 21 — Juice & FX (o espetáculo) · 🟢 P0 (núcleo) + 🟡 P1 (polish)

> **"Juice it or lose it."** Este doc cobre o **output sensorial** do combate: tudo que os olhos, ouvidos e mãos do jogador **recebem** quando um golpe conecta. Hit-stop, screen shake, slow-mo, VFX, SFX, damage numbers, GameplayCues — o "espetáculo" que transforma uma colisão de hitbox numa **pancada**.

> 🎁 **O presente single-player.** O jogo é **local-authoritative** ([decisão 2026-06, Index](../00_Index.md)). Sem netcode, sem reconciliação de cliente. Isso torna **slow-mo e hit-stop globais TRIVIAIS**: `CustomTimeDilation` (por ator) e `GetWorldSettings()->SetGlobalTimeDilation()` (mundo todo) funcionam direto, sem dívida de predição. **Abuse disso** — é o que dá ao DDR o feel de um character-action AAA de graça.

> **Distinção de escopo (não confunda):**
> - **Juice (este doc)** = o que os sentidos **recebem** (output: FX, som, freeze, shake). O espetáculo.
> - **Feel ([20](20_Game_Feel.md))** = o que as mãos **dão** (input/tato: peso, responsividade, cinestesia). O toque.
> - **O que dispara cada efeito / regras** → [Combat System Deep (18)](../combat/18_Combat_System_Deep.md). Aqui é o *o quê*, lá é o *quando*.

> **Pré-req:** [Hit-stop = freeze global (15 §5)](../combat/15_Combat_Overview.md) · [Slam = clímax (16 §5)](../combat/16_Aerial_Combos.md) · [Sombra no chão (06 §4)](../systems/06_Camera_TopDown.md) · [GameplayCues (05 §7)](../systems/05_GAS_Architecture.md).

> 👁️ **5 regras de leitura topdown (Review de Combate — Vi):** em topdown *leitura É juice*; juice mal-feito vira ruído que ATRAPALHA.
> 1. **Flash > partícula** na horda — o flash branco diz "acertei ISTO"; partícula some no caos.
> 2. **Damage numbers só em crit/slam** — número em todo hit polui a leitura.
> 3. **Todo FX respeita o `time-dilation`** — Niagara e áudio escalam com o slow-mo, senão o espetáculo se trai (faísca "voa" no freeze; som a 1× sobre slow 0.3×).
> 4. **FX achatados no chão > verticais** — shockwave/ring/decal lê melhor de cima (exceção: o trail do launch, que comunica "subiu!").
> 5. **Telegrafe de inimigo = juice defensivo P0** — decal vermelho pulsante no chão. Sem ele, em topdown denso o jogador morre sem entender e o **Pilar 4 quebra**.
>
> Detalhe de cada uma nas seções abaixo. Origem: [Review de Combate](../design/Design_Review_Combat_2026-06.md).

---

## 1. O que é "juice" (e por que ele É o combate)

Sem juice, dois Actors com `bCanEverTick` trocam números e o combate sente **molhado** — a hitbox conecta mas nada *acontece*. Juice é a camada de feedback que diz ao cérebro do jogador "**isso teve consequência**". DMC, Bayonetta e Hades vivem disso: a mecânica é simples, o **espetáculo** é o que vicia.

A regra de ouro: **todo evento de gameplay merece feedback redundante em múltiplos canais** (visão + audição + tato). Um hit forte deve treme a tela, congelar o tempo, soltar faísca, tocar um "crunch" e vibrar o controle — **ao mesmo tempo**. Redundância não é exagero; é como o cérebro confirma o impacto.

> 🎚️ **Princípio mestre deste doc:** a quantidade de juice **escala com a importância do golpe** (§7). Um jab é um tremor; o **slam é um terremoto**. Se tudo grita, nada grita.

---

## 2. Taxonomia de juice (o catálogo de técnicas)

Cada técnica abaixo é um "tempero". O combate completo combina vários por hit. Custo e prioridade indicados.

| # | Técnica | O que faz | Como (UE5.4) | Custo | Prio |
|---|---|---|---|---|:---:|
| 1 | **Hit-stop / freeze** | Congela N frames no impacto → **peso** | Freeze global (§3) | barato | 🟢 P0 |
| 2 | **Screen shake** | Treme a câmera no acerto | `UCameraShakeBase` via cue (§4) | barato | 🟢 P0 |
| 3 | **Hit flash** | Alvo pisca branco | Material param no mesh do alvo (§5) | barato | 🟢 P0 |
| 4 | **Hit VFX** | Faísca/spark no contato | Niagara via cue (§8) | barato | 🟢 P0 |
| 5 | **Hit SFX** | Whoosh + impacto + crunch | MetaSound em camadas (§9) | barato | 🟢 P0 |
| 6 | **Controller rumble** | Vibração no acerto | `Force Feedback` / `PlayDynamicForceFeedback` (§6) | barato | 🟢 P0 |
| 7 | **Damage numbers** | Número flutuante do dano | Widget pool / DrawText (§5) | médio | 🟡 P1 |
| 8 | **Time-dilation / slow-mo** | Bullet-time em momentos-chave | `SetGlobalTimeDilation` (§3.2) | barato (SP!) | 🟡 P1 |
| 9 | **Camera kick / punch** | Empurrão rápido da câmera (recoil) | offset aditivo no boom (§4) | médio | 🟡 P1 |
| 10 | **Impact frames** | 1-2 frames de pose/flash exagerados | freeze + escala/flash combinados (§5) | médio | 🟡 P1 |
| 11 | **Post-process no hit** | Chromatic aberration + vignette pulsando | MID em `PostProcessVolume` (§5) | médio | 🟡 P1 |
| 12 | **Radial blur / zoom punch** | Pulso de zoom no slam | FOV kick + radial blur MID (§7) | médio | 🔵 P2 |

> 🥊 **Hit-stop é o truque #1.** Se você só implementar UMA coisa deste doc, é o §3. 2-3 frames de freeze transformam o feel mais que qualquer partícula.

---

## 3. Hit-stop (freeze global) — o spec consolidado · 🟢 P0

> Consolidação do [doc 15 §5](../combat/15_Combat_Overview.md). **Este doc é o dono da escala/intensidade do freeze.** O 15 estabeleceu *que* é freeze global; aqui define-se *quanto* por tipo de golpe.

### 3.1 — Por que freeze GLOBAL (não pausa de montage)

Pausar só a montage do atacante **vaza no aéreo**: o CMC continua aplicando launch/gravidade durante o "freeze", o alvo **desliza** e o hit-stop perde o efeito ([15 §5](../combat/15_Combat_Overview.md), [16 §3](../combat/16_Aerial_Combos.md)). O hit-stop AAA congela **tudo** pelos mesmos N frames:

```
No notify de hit (após resolver dano):
  1. congela atacante + alvo (zera velocidade do CMC de ambos por N frames)
  2. pausa as montages dos dois corpos
  3. SÓ DEPOIS do freeze soltar → aplica launch / re-float / knockback
```

**Implementação single-player (o caminho fácil):** `Actor->CustomTimeDilation = 0.f` no atacante e no alvo por N frames, depois restaura `1.f`. Como não há replicação, isto "simplesmente funciona" — sem `FSavedMove`, sem reconciliação.

```cpp
// Pseudo — UDDRHitStopComponent (atacante chama no impacto)
void FreezePair(AActor* Attacker, AActor* Target, float Frames)
{
    const float Seconds = Frames / 60.f;            // assume 60fps de referência
    SetFrozen(Attacker, 0.f); SetFrozen(Target, 0.f);
    // timer (em tempo REAL, não dilatado) restaura ao fim:
    GetWorld()->GetTimerManager().SetTimer(Handle,
        [=]{ SetFrozen(Attacker, 1.f); SetFrozen(Target, 1.f); },
        Seconds, false);
}
// SetFrozen: Actor->CustomTimeDilation = X (e Mesh->GlobalAnimRateScale se quiser)
```

> ⚠️ **Timer em tempo real.** Use um handle que NÃO seja afetado pela dilatação que você acabou de aplicar — senão o freeze nunca solta. `FTimerManager` roda em tempo de mundo; se você dilatar o **mundo** (não só o ator), agende a restauração via `FApp::GetDeltaTime` não-dilatado ou um tick de `RealTimeSeconds`. Para freeze **por-ator** (`CustomTimeDilation`), o timer global fica imune — caminho recomendado.

> 🪂 **No juggle, ordem importa:** o re-float ([16 §3](../combat/16_Aerial_Combos.md)) e o hit-stop disparam no mesmo frame. **Congele os dois corpos, aplique o pop DEPOIS do freeze soltar.** Senão o pop "vaza" durante o congelamento.

### 3.2 — Escala de intensidade do freeze (🔥 o coração deste doc)

Quanto mais **importante/pesado** o golpe, mais longo o freeze. Esta tabela é a fonte canônica:

| Golpe | Frames de freeze | ≈ segundos (60fps) | Sensação |
|---|:---:|:---:|---|
| **Light** (jab de combo) | 2 | ~0.033 s | tap seco, mal percebido conscientemente |
| **Heavy** (golpe pesado) | 3-4 | ~0.05-0.066 s | pancada com peso |
| **Launcher** (uppercut) | 4-5 | ~0.066-0.083 s | "isso lançou" — marca a transição p/ ar |
| **Air hit** (juggle) | 3 | ~0.05 s | mantém ritmo aéreo sem matar o flow |
| **Slam / Finisher** ★ | **6-8** | ~0.1-0.133 s | **terremoto** — o clímax (§7) |
| **Perfect parry/dodge** | 8-10 | ~0.133-0.166 s | freeze quase teatral antes do slow-mo |

> 🎛️ **Tunar com debug.** Exponha um `ddr.HitStopScale <float>` (multiplicador global, default 1.0) pra varrer todos os freezes de uma vez no playtest. Hit-stop é número, não intuição — você precisa *sentir* varrendo valores. (No espírito do `ddr.CombatDebug`, [15 §6](../combat/15_Combat_Overview.md).)

### 3.3 — Time-dilation / slow-mo (a liberdade single-player) · 🟡 P1

Diferente do hit-stop (freeze de N frames duros), o **slow-mo** desacelera o mundo a uma fração (ex.: 0.3×) por um período mais longo, com **ease-in/ease-out**. Reservado pra **momentos dramáticos**:

| Gatilho | Escala | Duração | Curva | Quem desacelera |
|---|:---:|:---:|---|---|
| **Perfect dodge** ([feel 20](20_Game_Feel.md)) | 0.25-0.35× | 0.6-1.0 s | ease-out rápido | **mundo** (`SetGlobalTimeDilation`) |
| **Slam finisher** (impacto) | 0.4× | 0.2-0.3 s | snap-in, ease-out | mundo |
| **Última kill da sala** | 0.3× | 0.8 s | cinemático | mundo |
| **Início de boss** | 0.5× | scriptado | sequencer | mundo |

```cpp
// Single-player: trivial. Sem netcode pra atrapalhar.
GetWorldSettings()->SetGlobalTimeDilation(0.3f);
// ... restaurar via timer em RealTime ou Lerp por tick não-dilatado:
GetWorldSettings()->SetGlobalTimeDilation(1.0f);
```

> 🧊 **Player imune ao próprio slow-mo (perfect dodge):** desacelere o mundo, mas dê ao **player** `CustomTimeDilation = 1/0.3 ≈ 3.33` pra ele se mover em velocidade normal enquanto inimigos "congelam". É o efeito "Witch Time" da Bayonetta — e em SP é uma linha. Ver [feel 20](20_Game_Feel.md) (o dodge é input; o slow-mo é o output que ele dispara).

> ⚠️ **Anim em slow-mo precisa interpolar.** Com o mundo a 0.3×, montages ficam lentas de propósito — bom. Mas garanta que VFX/SFX disparados durante o slow respeitem a dilatação (Niagara tem `Local Space`/time scale; áudio pode pitch-down pra reforçar). Não dispare um som a 1.0× durante slow 0.3× — soa quebrado.

---

## 4. Câmera: shake, kick e punch · 🟢 P0 (shake) / 🟡 P1 (kick)

A câmera topdown ([06](../systems/06_Camera_TopDown.md)) é fixa (pitch -55°, sem orbit), o que **ajuda**: shake aditivo não briga com controle de câmera do jogador.

| Efeito | O que é | Como (UE5.4) | Escala por hit |
|---|---|---|---|
| **Screen shake** | Tremor procedural curto | `UCameraShakeBase` (perlin/wave) via `PlayWorldCameraShake` ou cue | amplitude ∝ peso (§7) |
| **Camera kick / punch** | Empurrão direcional rápido (recoil 1-frame) | offset aditivo no `SpringArm` (impulso + spring-return) | só heavy+ |
| **Zoom/FOV punch** | Pulso curto de FOV (slam) | Lerp do `FieldOfView`/`TargetArmLength` e volta | só slam |

```cpp
// Screen shake via GameplayCue (recomendado — desacopla):
// no GameplayCue_Hit_Heavy::OnExecute →
if (APlayerCameraManager* PCM = GetLocalCameraManager(Target))
    PCM->StartCameraShake(HeavyHitShakeClass, Intensity);
```

**Direção do shake importa em topdown:** um kick na **direção do impacto** (do atacante → alvo, projetado no plano da tela) lê melhor que tremor isotrópico. Para o slam (vertical), um kick **pra baixo** + zoom-punch vende o "DESCEU".

> 🎥 **Não enjoe.** Shake forte demais ou frequente vira náusea e some o detalhe. Lights mal devem tremer; reserve amplitude pra heavy/launcher/slam. Exponha `ddr.ShakeScale` pra tunar/desligar (acessibilidade — alguns jogadores querem zero shake).

> 🔵 **Radial blur / motion blur no slam (P2):** um pulso de radial blur centrado no impacto (MID em post-process, [ue-materials-rendering]) intensifica o clímax. Polish — não bloqueante.

---

## 5. Hit flash, damage numbers, impact frames, post-process

### Hit flash (alvo pisca) · 🟢 P0
No impacto, o mesh do alvo pisca **branco** (ou cor de elemento) por ~2-4 frames. Implementação: parâmetro escalar `HitFlash` (0→1→0) num **MID** no material do alvo, dirigido por curva curta.

```cpp
// no alvo, ao receber dano (ou via GameplayCue.Hit no alvo):
DynMat->SetScalarParameterValue("HitFlashAmount", 1.f);
// timeline/curva traz de volta a 0 em ~0.1s
```
> 🎯 **Leitura topdown:** o flash é o feedback mais barato de "acertei ISTO" quando há horda. Em densidade alta ([Death Must Die-like](../design/01_Game_Vision.md)), o flash é o que o olho segue.

### Damage numbers · 🟡 P1
Número flutuante que sobe e some no ponto de impacto. **Pool de widgets** (não crie/destrua por hit — performance em horda):

| Aspecto | Decisão |
|---|---|
| Cor por tipo | branco = normal · amarelo = crit · ciano = elemental |
| Tamanho ∝ dano | crit/slam = maior + outline grosso |
| Anti-spam | agregue hits do mesmo alvo no mesmo frame; cap de N na tela |
| Render | `WidgetComponent` billboard OU `DrawText` em HUD projetado (mais barato em massa) |

> Em topdown com horda, damage numbers viram **ruído** rápido. Considere: só crits/slams mostram número grande; hits normais só flash+spark. Tune por densidade.

### Impact frames · 🟡 P1
A técnica de animação 2D ("smear/impact frame") traduzida pra 3D: no frame do hit, combine **freeze (§3) + hit flash exagerado + escala momentânea** (alvo "incha" 5-10% por 1-2 frames). Vende deformação sem rig de squash. Reserve pra heavy/slam.

### Post-process no hit · 🟡 P1
Pulso curto de **chromatic aberration** + **vignette** quando o **player** toma dano (não nos hits que ele dá — senão polui). Um MID num `PostProcessVolume` global ou no `PlayerCameraManager`:

| Evento | Efeito PP | Intensidade |
|---|---|---|
| Player leva hit | chromatic + vignette vermelho pulsa | ∝ dano tomado |
| Player low HP | vignette vermelho sustentado + leve desaturação | estado contínuo |
| Slam (que o player dá) | leve chromatic global no impacto | sutil |

---

## 6. Controller rumble (tato) · 🟢 P0

O canal **tátil** é metade do impacto e o mais esquecido. UE5.4: `Force Feedback Effects` (curvas L/R/triggers) ou `PlayDynamicForceFeedback` (procedural).

| Golpe | Padrão de rumble | Intensidade |
|---|---|---|
| Light hit | pulso curtíssimo | baixa |
| Heavy / launcher | pulso médio com leve sustain | média |
| **Slam** ★ | thump grave longo + rampa | **alta** |
| Perfect dodge | tap duplo nítido | média |
| Player toma dano | buzz áspero | ∝ dano |

> 🎮 Sincronize o rumble com o **freeze** (§3): o pico da vibração no frame congelado amplia o "soco". Dispare ambos no mesmo notify. Sempre dê opção de desligar (acessibilidade).

---

## 7. 🔥 Escala de intensidade por importância do golpe (a tabela mestra)

**O princípio central do doc:** `light < heavy < launcher < SLAM`. O slam é o **clímax** e deve ser um **EVENTO sensorial** — todos os canais no máximo. Esta tabela cruza **tipo de golpe × canal de juice**:

| Canal ↓ \ Golpe → | **Light** | **Heavy** | **Launcher** | **Air hit** | **🌟 SLAM** |
|---|:---:|:---:|:---:|:---:|:---:|
| **Hit-stop (frames)** | 2 | 3-4 | 4-5 | 3 | **6-8** |
| **Screen shake** | mínimo | médio | médio-forte | leve | **forte + kick↓** |
| **Camera kick** | — | leve | médio | — | **forte (downward)** |
| **Slow-mo** | — | — | — | — | **0.4× · 0.25s** |
| **Hit flash** | sim | sim | sim (+ tint) | sim | **sim + impact frame** |
| **VFX (Niagara)** | spark pequeno | spark + ring | trail vertical | spark aéreo | **shockwave + poeira + crack** |
| **SFX** | whoosh+tap | whoosh+thud | whoosh+"whoom" | tick aéreo | **crunch grave + sub-bass + debris** |
| **Rumble** | tap | médio | médio | tap | **thump longo** |
| **Damage number** | (opcional) | normal | normal | pequeno | **grande + outline** |
| **Post-process** | — | — | — | — | **leve chromatic pulse** |
| **FOV/zoom punch** | — | — | — | — | **sim (zoom-in→out)** |

> 💥 **O slam é o pagamento de TODO o combo aéreo.** O jogador subiu o inimigo, manteve no ar com skill ([16](../combat/16_Aerial_Combos.md)) — o slam é a recompensa sensorial. Não economize: freeze longo + slow-mo + shockwave + poeira + sub-bass + rumble + zoom-punch, **todos juntos**. Se o slam não arrepia, o pilar do jogo não fecha.

> 🪜 **Curva de escalada legível:** o jogador deve *sentir* na pele que o launcher é "mais" que o jab e o slam é "mais" que tudo — sem ler número. A diferença de juice **comunica a hierarquia dos golpes** melhor que qualquer tutorial.

---

## 8. VFX — catálogo Niagara (conceitual) · 🟢 P0 (hit) / 🟡 P1 (resto)

> Conceitual — sistemas Niagara disparados por GameplayCues (§10). Implementação de spawn/parâmetros: skill **ue-niagara-effects**. Materiais de partícula: **ue-materials-rendering**.

| FX (Niagara System) | Quando | Conteúdo visual | Prio |
|---|---|---|---|
| `NS_Hit_Spark` | todo hit conectado | faíscas curtas na normal do impacto + flash | 🟢 P0 |
| `NS_Hit_Ring` | heavy+ | anel de choque achatado (lê de cima) | 🟡 P1 |
| `NS_Slash_Trail` | swing da arma | trail ao longo do arco (sockets `weapon_start/end`) | 🟡 P1 |
| `NS_Launch_Trail` | launcher | **trail vertical** subindo do alvo (comunica "subiu!") | 🟡 P1 |
| `NS_Juggle_Spark` | cada air hit | spark menor + leve poeira aérea | 🟡 P1 |
| `NS_Slam_Shockwave` | impacto do slam | **onda radial no chão** (lê AoE) + crack + dust burst | 🟢 P0 (do pilar) |
| `NS_Land_Dust` | hard land ([13 §6](../locomotion/13_Jump_Fall_Landing.md)) | poeira radial no pouso | 🟡 P1 |
| `NS_Death_Dissolve` | morte de inimigo | dissolve/burst (combina com hit final) | 🟡 P1 |
| `NS_Telegraph_*` | telegrafe de inimigo (§9 leitura) | decal/forma de aviso no chão | 🟢 P0 (Pilar 4) |

> 🎯 **Topdown lê o CHÃO.** Priorize FX **achatados/no plano do chão** (rings, decals, shockwaves) sobre verticais altos — a câmera de cima vê o chão melhor que a altura. O `NS_Slam_Shockwave` no chão comunica a área de dano do slam ([16 §5](../combat/16_Aerial_Combos.md)) de forma legível. Exceção deliberada: `NS_Launch_Trail` é vertical **de propósito** (comunica altura, §11).

> ⚠️ **Niagara respeita time-dilation.** Durante hit-stop/slow-mo, partículas precisam congelar/desacelerar junto (senão a faísca "voa" durante o freeze do alvo). Verifique o time-scale do sistema.

---

## 9. SFX — camadas de som de hit · 🟢 P0

Som de hit AAA **não é um arquivo** — é **camadas** montadas (MetaSound, [ue-audio-system]). A receita de 3 camadas:

```
Som de hit = [WHOOSH] (antecipação do swing, antes do contato)
           + [IMPACTO] (o "thud"/"thwack" no frame do hit)
           + [CRUNCH/TAIL] (cauda — "carne", debris, reverb curto)
```

| Camada | Função | Varia por |
|---|---|---|
| **Whoosh** | telegrafa o swing (antecipação) | velocidade/peso da arma |
| **Impacto** | o "soco" no contato | tipo de golpe (§7) |
| **Crunch/tail** | confirma consequência ("doeu") | material do alvo / crit |
| **Sub-bass** (só slam) | peso físico no peito | exclusivo do clímax |

> 🔊 **Leitura sonora em topdown:** com a câmera longe e horda densa, o **som carrega o feedback** que o pixel não dá. Pitch/volume sutilmente diferentes por tipo de golpe ajudam o jogador a *ouvir* o combo. **Variação anti-fadiga:** randomize pitch ±5-10% e use 2-3 samples por camada (`Random` node no MetaSound) — som idêntico repetido vira ruído mental rápido em hack'n'slash.

> 🎚️ Mixagem: o **slam** deve ter o pico de loudness do combate (com sub-bass exclusivo). Lights ficam baixos pra não fadigar. Use submix/ducking pra o slam "abrir espaço" momentâneo.

---

## 10. Catálogo de GameplayCues (como o GAS dispara o juice) · 🟢 P0

GameplayCues **desacoplam** *o que aconteceu* de *como soa/parece* ([05 §7](../systems/05_GAS_Architecture.md)). Cada Cue agrega VFX + SFX + shake + rumble num pacote. Dispare por `ASC->ExecuteGameplayCue(Tag, Params)` ou pelo campo *Gameplay Cues* do próprio `GameplayEffect`.

| GameplayCue Tag | Disparado por | VFX | SFX | Shake/Cam | Rumble | Freeze |
|---|---|---|---|---|:---:|:---:|
| `GameplayCue.Hit.Light` | GA_Attack (light) | `NS_Hit_Spark` (s) | whoosh+tap | mínimo | tap | 2f |
| `GameplayCue.Hit.Heavy` | GA_Attack (heavy) | spark+ring | whoosh+thud | médio | médio | 3-4f |
| `GameplayCue.Hit.Air` | GA_AirAttack | `NS_Juggle_Spark` | tick aéreo | leve | tap | 3f |
| `GameplayCue.Launch` | GA_Launcher | `NS_Launch_Trail` (vert.) | "whoom" | kick médio | médio | 4-5f |
| `GameplayCue.Slam` ★ | GA_AirSlam (impacto) | `NS_Slam_Shockwave`+dust | crunch+sub-bass | **forte+kick↓+zoom** | **thump** | **6-8f**+slow |
| `GameplayCue.Land.Hard` | Hard land ([13 §6](../locomotion/13_Jump_Fall_Landing.md)) | `NS_Land_Dust` | thud+poeira | forte | médio | — |
| `GameplayCue.Dash` | GA_Dash | trail/whoosh do dash | swish | — | leve | — |
| `GameplayCue.Dodge.Perfect` | perfect dodge ([20](20_Game_Feel.md)) | flash/aura time | "ting" + drone | leve | duplo-tap | 8-10f→slow |
| `GameplayCue.Hurt.Player` | player toma dano | sangue/flash | grunt | chromatic+vignette | buzz | — |
| `GameplayCue.Death.Enemy` | inimigo morre | `NS_Death_Dissolve` | death + debris | (se for kill de slam) | — | — |

> 🧩 **Uma Cue = um pacote sensorial.** O designer dispara `GameplayCue.Slam` e **todos** os canais (§7) acendem juntos, sem o código de gameplay saber de Niagara/áudio. Isso mantém [combat (15/16)](../combat/15_Combat_Overview.md) limpo. **Tipos de Cue:** use **Burst** (`GameplayCueNotify_Static`/`Burst`) pra hits instantâneos; **Looping** (`_Actor`) só pra estados contínuos (ex.: aura de juggle, low-HP).

> 📍 **Onde plugar no combate:** o `ANS_Hitbox` ([15 §4](../combat/15_Combat_Overview.md)) detecta o acerto → `GameplayEvent "Combat.Hit"` → a ability aplica `GE_Damage` **e** executa a Cue correspondente ao tipo de golpe. A escala de juice (§7) vem do **tipo de golpe**, não do dano numérico.

---

## 11. Juice específico de topdown (leitura como FX) · 🟢 P0

Em topdown, **leitura É juice**: o feedback que comunica *o que está acontecendo* vale tanto quanto o que comunica *impacto*. Ver [06 §4](../systems/06_Camera_TopDown.md) e [16 §6](../combat/16_Aerial_Combos.md).

| Recurso | Função sensorial | Obrigatoriedade |
|---|---|---|
| **Sombra no chão** (player + alvo) | gap sombra↔corpo = **altura** do juggle | 🟢 **Obrigatório** (requisito do pilar) |
| **`NS_Launch_Trail` vertical** | comunica "subiu!" no launcher | 🟢 P0 |
| **Escala sutil no ar** | corpo no ar levemente maior (perto da câmera) | 🟡 P1 |
| **Outline no alvo juggleado** | foco visual no que está sendo combado | 🟡 P1 |
| **Telegrafe de inimigo** (decal/forma) | leitura de **perigo** ([Pilar 4](../design/01_Game_Vision.md)) | 🟢 P0 |
| **Ênfase de silhueta** | inimigo/perigo em cor saturada salta do fundo escuro | 🟡 P1 |
| **Leve zoom-out no juggle** | cabe a ação vertical na tela | 🟡 P1 |

> 🪂 **A sombra é o juice mais importante do combo aéreo.** Sem ela, o combo é **ilegível** ([16 §6](../combat/16_Aerial_Combos.md)) — não é polish, é o que torna o pilar jogável. O `RootMotionSource` ([16 §2](../combat/16_Aerial_Combos.md)) dirige a altura; a sombra a **comunica**.

> 🚨 **Telegrafe = juice defensivo.** O FX que avisa "vai cair um golpe aqui" (decal vermelho pulsante no chão, flash de antecipação) é tão essencial quanto o FX de impacto. Em topdown denso, sem telegrafe legível o jogador morre sem entender — quebra o [Pilar 4](../design/01_Game_Vision.md). Trate telegrafes com o mesmo carinho dos hits.

---

## 12. Orçamento de juice (P0 barato → P2 polish)

**Princípio:** o juice essencial é **barato**. Gaste o esforço na escala (§7), não em FX caros cedo.

| Camada | Itens | Custo | Quando |
|---|---|---|---|
| 🟢 **P0 — essencial barato** | hit-stop (freeze global), screen shake, hit flash, `NS_Hit_Spark`, SFX 3-camadas, rumble, **sombra no chão**, `Slam` cue completa, telegrafe | baixo | **M1** (chão) + **M2** (aéreo/slam) |
| 🟡 **P1 — polish que vende** | slow-mo (dodge/slam), camera kick, damage numbers, post-process no hit, impact frames, trails (slash/launch/juggle), `Land_Dust`, escala/outline aéreo | médio | depois do M2 jogável; passe de feel do **M5** |
| 🔵 **P2 — luxo** | radial/motion blur, zoom-punch refinado, death dissolves elaborados, FX por arma/elemento, rigs de câmera (GameplayCameras) | alto | pós-MVP |

> 💡 **Ordem de retorno (ROI):** hit-stop > SFX em camadas > hit flash > screen shake > rumble > spark VFX. Os 3 primeiros são quase de graça e entregam **80% do feel**. Se o budget acabar, garanta esses. Slow-mo e shockwave do slam entram no M2 porque **o slam é o pilar** — não são "polish opcional" ali.

> 🥊 **Reforço do [doc 15](../combat/15_Combat_Overview.md):** implemente hit-stop **no M1**, junto com o primeiro combo. Não deixe pra "depois" — é o que separa "trocar números" de "bater". O M1 só está pronto quando o combo de chão **tem peso**.

---

## 13. Checklist

**🟢 P0 (M1 — chão):**
- [ ] Hit-stop **freeze global** (CustomTimeDilation por-ator, congela atacante+alvo) — §3
- [ ] Escala de freeze por tipo de golpe (light 2f → heavy 3-4f) — §3.2
- [ ] `ddr.HitStopScale` pra tunar todos os freezes de uma vez — §3.2
- [ ] Screen shake (`UCameraShakeBase`) via GameplayCue — §4
- [ ] Hit flash (MID `HitFlashAmount` no alvo) — §5
- [ ] `NS_Hit_Spark` + SFX 3-camadas (whoosh+impacto+crunch) — §8, §9
- [ ] Controller rumble por hit — §6
- [ ] Cues `Hit.Light/Heavy`, `Dash`, `Hurt.Player` — §10

**🟢 P0 (M2 — aéreo/slam):**
- [ ] **Sombra no chão** (player + alvo) — leitura de altura — §11
- [ ] `NS_Launch_Trail` vertical no launcher — §8, §11
- [ ] **`GameplayCue.Slam` completa** (freeze 6-8f + shockwave + sub-bass + kick↓ + rumble) — §7, §10
- [ ] `NS_Slam_Shockwave` no chão (lê AoE) — §8
- [ ] Telegrafe de inimigo legível (Pilar 4) — §11

**🟡 P1 (polish / passe de feel M5):**
- [ ] Slow-mo (perfect dodge "Witch Time" + slam) via `SetGlobalTimeDilation` — §3.3
- [ ] Camera kick/punch direcional + zoom-punch no slam — §4, §7
- [ ] Damage numbers (pool de widgets, cor por tipo) — §5
- [ ] Post-process no hit (chromatic+vignette ao tomar dano) — §5
- [ ] Trails de slash/juggle, `Land_Dust`, escala/outline aéreo — §8, §11
- [ ] `ddr.ShakeScale` (tuning + acessibilidade) — §4

---

## 14. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Combate sente "molhado" | Sem hit-stop | §3 — freeze global, comece pelo M1 |
| Hit-stop "vaza" / alvo desliza no ar | Pausou só montage; CMC seguiu | §3.1 — congele CMC dos dois, pop **depois** |
| Freeze nunca solta | Timer afetado pela própria dilatação | §3.1 — use timer em tempo real / `CustomTimeDilation` por-ator |
| Tudo grita, nada impacta | Sem escala por golpe | §7 — light mínimo, slam máximo |
| Slam não arrepia | Canais isolados, não combinados | §7 — freeze+slow+shockwave+sub-bass+rumble **juntos** |
| Náusea / shake demais | Amplitude alta/frequente | §4 — reserve shake p/ heavy+; `ddr.ShakeScale` |
| Faísca "voa" durante o freeze | Niagara/áudio ignoram time-dilation | §8, §3.3 — respeite o time-scale |
| Não entendo a altura (aéreo) | Sem sombra no chão | §11 — obrigatório (não é polish) |
| Som de hit fadiga rápido | Sample único repetido | §9 — randomize pitch + 2-3 samples/camada |
| Damage numbers viram ruído | Spam em horda | §5 — agregue/cap; só crit/slam mostram grande |
| Slow-mo soa "quebrado" | SFX a 1.0× durante slow 0.3× | §3.3 — dilate/pitch o áudio junto |
| Player "congela" no próprio slow-mo | Dilatou o mundo sem compensar o player | §3.3 — `CustomTimeDilation` inverso no player |

---

## 15. Próximo passo

→ A **sensação cinestésica** (peso, responsividade, o tato do input que dispara este espetáculo): [20 — Game Feel](20_Game_Feel.md).
→ **O que dispara cada efeito** e as regras de combate por trás: [18 — Combat System Deep](../combat/18_Combat_System_Deep.md).
→ Os **abilities** que invocam cada Cue: [19 — Abilities Deep](../combat/19_Abilities_Deep.md).
→ O clímax onde tudo isto se concentra: [16 — Combos Aéreos §5 (slam)](../combat/16_Aerial_Combos.md).
