# 20 — Game Feel (o "punho") · 🟢 P0

> 🥊 Este doc é sobre **como SENTE segurar o controle** — o loop `input → resposta` e a sensação cinestésica. **Não** é sobre o que os sentidos recebem de volta (faísca, shake, som, slow-mo): isso é **juice**, mora no [feel/21 — Juice & FX](21_Juice_FX.md).
>
> A divisão de trabalho, em uma frase: **Feel = o tato (você → jogo)** · **Juice = o eco (jogo → você)**. Um dash que sai em 30ms com i-frames generosos é *feel*; a trilha azul que ele deixa é *juice*. Os dois juntos viram "game feel" no sentido amplo — mas tunar cada um na sua própria mesa evita confundir "sente lento" (knob de feel) com "sente fraco" (knob de juice).

> 🔒 **Vantagem existencial deste projeto:** o jogo é **single-player local-authoritative** ([Index, Decisões](../00_Index.md)). Sem rede, sem predição, sem reconciliação. O loop `input → CMC/GAS → resposta` é **100% seu** — responsividade aqui é **escolha de design**, nunca um compromisso de netcode. Aproveite com agressividade: hitstop local, `CustomTimeDilation` global, RootMotion sem rollback. Nenhuma decisão deste doc precisa "sobreviver" a um servidor.

---

## 1. O que é "feel" e por que importa 🎯

Game feel é a **resposta tátil em tempo real** a uma ação contínua, vestida de espetáculo (Steve Swink). Em um hack'n'slash de combo, ele é literalmente o produto: o jogador passa 95% do tempo no loop `aperta → personagem responde → lê → aperta de novo`. Se esse loop tem atrito, **nenhuma** quantidade de Ecos, salas ou VFX salva.

A fantasia que o doc de visão promete ([01 §3](../design/01_Game_Vision.md)) é cinestésica antes de ser visual:

- *"o personagem responde **imediato**. Sem input lag, sem patinar pra começar a andar."*
- *"dash → ataque → launcher → combo aéreo → slam → próximo grupo, **sem parar**."*

Esse "imediato" e esse "sem parar" são **knobs**, não desejos. Este doc lista os knobs.

> 🧭 **Princípio mestre — "responda JÁ":** quando responsividade e realismo colidem, **responsividade ganha** (Pilar 4: leitura/clareza > realismo, [01 §2](../design/01_Game_Vision.md)). Um personagem que vira em 1 frame não é "irreal demais" — é *gostoso*. Realismo é o default da física; feel é você quebrando a física de propósito a favor do jogador.

---

## 2. Orçamento de responsividade ⏱️

### 2.1 Latência-alvo

Latência de input = tempo do clique do botão até o **primeiro frame** em que o personagem reage na tela. O orçamento:

| Faixa (input → reação) | Sensação | Veredito |
|---|---|---|
| **≤ 50 ms (~3 frames @60)** | "instantâneo" | 🟢 **Alvo p/ verbos de ação** (dash, attack start, jump) |
| 50–100 ms (3–6 frames) | aceitável | 🟡 Teto tolerável; já dá pra "sentir" |
| > 100 ms (6+ frames) | "atrasado", "molhado" | ❌ Inaceitável para os verbos. Investigue |

Em single-player a 60fps, o piso é ~1 frame (16.6ms). **Não desperdice esse piso** com:
- atraso artificial antes de iniciar montage (toque a anim no mesmo frame da ativação);
- `ETriggerEvent::Triggered`/`Hold` onde deveria ser `Started` ([07 §8](../systems/07_Input.md): dash/attack usam **`Started`** = dispara no down, não espera repetição);
- esperar a anim "terminar a pose anterior" antes de aceitar a próxima (é pra isso que existe cancelamento, §6).

> 🎮 **Regra de bolso:** o **primeiro frame da resposta** é sagrado. O personagem pode levar 12 frames pra *completar* o swing, mas o swing tem que **começar** no frame seguinte ao input. Front-load a resposta.

### 2.2 Janelas de buffer = perdão como ferramenta de design 🪶

A latência mede *quão rápido o jogo responde*. O **buffer** mede *quão cedo o jogo aceita você*. São coisas diferentes e ambas são feel.

O buffer ([07 §5](../systems/07_Input.md)) é **perdão**: o jogador aperta um tico cedo (porque humanos antecipam ritmo), o jogo **guarda** o input e o consome quando a janela abre. O resultado sentido: o combo sai "liso", nunca "engasga". Sem buffer, o jogador precisa do frame exato — e o combate vira um teste de reflexo punitivo em vez de uma expressão fluida.

| Buffer | Valor inicial | O que perdoa |
|---|---|---|
| **Buffer de combo** ([07 §5](../systems/07_Input.md)) | **0.20–0.35 s** antes da janela | apertar o próximo golpe cedo |
| **Jump buffer** ([13 §3](../locomotion/13_Jump_Fall_Landing.md)) | **~0.10 s** antes de pousar | apertar pulo antes de tocar o chão |
| **Coyote time** ([13 §3](../locomotion/13_Jump_Fall_Landing.md)) | **~0.10 s** depois de sair da borda | pular "um tico tarde" saindo de plataforma |
| **Dash buffer** (recomendado) | **~0.15 s** durante recovery do golpe | enfileirar o dash-cancel |

> 🧠 **Filosofia:** o buffer é **invisível quando funciona** e a primeira coisa que sentem quando falta. Ele é a diferença entre "o jogo é difícil" (justo) e "o jogo é teimoso" (injusto). Comece **generoso** (0.3s) e aperte só se o combate ficar "escorregadio demais" — é muito mais raro o jogador reclamar de buffer grande do que pequeno.

> ⚠️ **Buffer ≠ input lag.** Buffer atrasa o *aceite* (você apertou cedo), nunca a *resposta* (quando a janela abre, sai no mesmo frame). Os dois knobs são independentes: persiga **latência baixa E buffer generoso** ao mesmo tempo.

---

## 3. Feel de controle e movimento 🏃

### 3.1 A "curva de peso": responsivo ⇄ pesado

Todo movimento vive num espectro entre **responsivo** (vira/arranca/para no talo — Hyper Light Drifter) e **pesado** (momentum, inércia, o corpo "custa" — Dark Souls). Para um ARPG topdown de horda, você quer **muito responsivo no plano horizontal** (reposicionar entre grupos é vida ou morte, Pilar 4) e só um *tempero* de peso pra não sentir robótico.

Os três knobs que definem onde você cai na curva (todos no `UDDRCharacterMovementComponent`, [09 §7](../locomotion/09_Gaits.md)):

| Knob | Baixo = | Alto = | Valor inicial (DDR) |
|---|---|---|---|
| `MaxAcceleration` | arranque lento, "pesado" | arranque instantâneo, "responsivo" | **2048** (faixa 1500–2500) |
| `BrakingDecelerationWalking` | desliza ao parar | crava ao parar | **2048** |
| `RotationRate` (Yaw) | vira devagar, "barco" | vira no talo | **720 °/s** ([06 §3](../systems/06_Camera_TopDown.md)) |

> 🎚️ **Sentido de cada um:**
> - **Accel alta + braking alto** = personagem "cola" no input: anda quando você quer, para quando você solta. É o default ARPG topdown (Diablo/Hades). **Comece aqui.**
> - **Accel baixa** introduz "ramp-up" — sente peso, mas adiciona latência percebida no *movimento* (não nos verbos). Use com parcimônia; é tempero, não base.
> - **RotationRate 720** = meia-volta em ~0.25s. Suficiente pra "olhar pra onde ando" sem sentir trava. Subir pra 1080+ deixa quase instantâneo (bom pra twin-stick); baixar pra <360 dá "barco" (ruim pra topdown).

### 3.2 Orient-to-movement: o feel do modelo MVP

O MVP usa **orient-to-movement** ([06 §3, Modelo A](../systems/06_Camera_TopDown.md)): o personagem olha pra onde anda, `bUseControllerRotationYaw=false`. Sentido cinestésico:

- **Bom:** o corpo "segue" o stick — natural, fluido, zero microgerência de mira. Casa com a câmera de yaw fixo (o "pra cima" do analógico é sempre o "pra cima" da tela, [07 §3](../systems/07_Input.md)).
- **Cuidado:** com `RotationRate` muito baixo, inverter a direção (180°) cria um "arco de viragem" que sente lag. Com 720 já é confortável; se quiser **snap** em mudanças bruscas de direção, deixe a rotação rápida — o personagem gira andando, nunca parado. (Turn In Place foi **removido do escopo**; a câmera de cima o esconderia de qualquer forma.)

> 🔑 **Decisão de feel herdada do projeto irmão:** em topdown + orient-to-movement, o personagem **gira andando**, raramente parado. Isso *mata* a necessidade de turn-in-place no feel ([06 §3](../systems/06_Camera_TopDown.md)). Não é preguiça — é a câmera escondendo o que não importa pro tato.

### 3.3 Feel de começar e parar

- **Começar:** com `MaxAcceleration` 2048, o personagem atinge ~80% da `RunSpeed` (500) em ~2 frames. É o "sem patinar pra começar" da visão. **Não** baixe accel "pra ficar realista" — patinar no start é a reclamação nº1 de feel.
- **Parar:** `BrakingDecelerationWalking` 2048 crava rápido. As transições *animadas* de parada (distance-match stop, [10](../locomotion/10_Start_Stop_DistanceMatch.md)) são P1 e **estéticas** — o feel da parada vem do braking físico, não da anim. Em topdown, a anim de stop é quase invisível; o que importa é o personagem **estar onde você soltou o stick**.

> 🪂 **Anti-slide no pouso** ([13 §7](../locomotion/13_Jump_Fall_Landing.md)): pousar correndo não pode "patinar". O padrão do DungeonForged — *retain 0.4 + landing brake ~4096* — preserva um tico de momentum (não trava seco, sente natural) mas mata o slide. Tune pro seu feel.

---

## 4. Feel dos verbos: dash, pulo, sprint 🎮

Cada verbo tem uma **assinatura cinestésica** — um perfil de como começa, se sustenta e termina. Acertar a assinatura importa mais que o valor exato.

### 4.1 Dash / esquiva (o verbo de sobrevivência)

O dash é o verbo mais *sentido* do jogo: é escape, é cancelamento ([15 §6](../combat/15_Combat_Overview.md)), é mobilidade. Anatomia do feel:

| Componente | O que é | Valor inicial | Sentido |
|---|---|---|---|
| **Snap inicial** | aceleração do dash no frame 1 | quase-instantâneo (RootMotion/impulso forte) | o dash tem que **arrancar**, não "acelerar até a velocidade de dash" |
| **Distância** | quão longe leva | **500–650 cm** | longe o bastante pra reposicionar entre grupos; curto o bastante pra ser preciso |
| **Duração** | quanto dura o deslocamento | **~0.20–0.25 s** | curto = "snappy"; longo = "voo", perde controle |
| **i-frames (2 camadas)** | invulnerabilidade | **~0.20–0.30 s** front-loaded **+ sub-janela *perfect*** (~6 frames) | (a) generoso = "confio no botão" (perdão); (b) perfect = timing recompensado → witch-time ([19](../combat/19_Abilities_Deep.md), [21](21_Juice_FX.md)) |
| **Cooldown-feel** | quão livre é spammar | **~0.5–0.8 s** ([19](../combat/19_Abilities_Deep.md)) | curto o bastante pra sentir **"sempre lá"**, longo o bastante pra impedir spam de i-frames; gate por stamina é P2 |

> 🛡️ **i-frames são feel, não balance (no MVP).** Um dash sem i-frames, ou com i-frames mesquinhos, ensina o jogador a *não confiar* no dash — e aí o verbo morre. Comece com i-frames **cobrindo a maior parte do deslocamento** (frente-carregados). Se trivializar, encolha *depois*, sentindo. A regra de Hyper Light Drifter / Hades: o dash é a tábua de salvação, e ele tem que *parecer* uma.

> 🥷 **A tensão generoso × perfect (Review de Combate — Nø×Rha):** i-frame generoso (perdão) **briga** com perfect-dodge (recompensar timing) — se o dash inteiro é invulnerável, nunca há o "quase morri e revidei". **Resolução: 2 camadas** — base generosa (o feel acima) **+ sub-janela *perfect*** nos ~6 primeiros frames (counterplay → witch-time, [19](../combat/19_Abilities_Deep.md)). As duas mesas ganham. Perfect-dodge entra no **M1**.

> 🧊 **Dash-cancel-sempre** (o feel, não a regra — regra em [18](../combat/18_Combat_System_Deep.md)): poder cortar *qualquer* golpe num dash a qualquer momento é o que faz o combate sentir **livre** em vez de **on-rails**. Ver §6.

### 4.2 Pulo / queda (o lado SENTIDO dos truques do doc 13)

O [doc 13](../locomotion/13_Jump_Fall_Landing.md) lista os truques de pulo como parâmetros; aqui é **o que cada um faz à sua mão**:

| Truque ([13 §3](../locomotion/13_Jump_Fall_Landing.md)) | O que a MÃO sente | Por quê |
|---|---|---|
| **Gravidade assimétrica** (sobe leve, cai pesado; `GravityScale` 1.75, descida ×1.2–1.5) | o pulo tem **autoridade**: você manda nele, ele não flutua | gravidade real (1.0) sente "lunar" e sem controle. >1 na descida = "o chão te chama" |
| **Apex hang** (gravidade reduzida no topo) | um **micro-momento de flutuação** no auge — a janela de decisão | dá tempo de mirar o próximo input no pico. É o "hang" que o ar precisa |
| **Variable jump height** (soltar cedo = pulo baixo) | o pulo **obedece a intenção**: tap = pulinho, hold = pulão | sem isso, todo pulo é igual e sente "burro" |
| **Coyote time** (~0.1s pós-borda) | perdão: "eu *tinha* apertado a tempo" | corrige a injustiça de 1-frame na borda |
| **Jump buffer** (~0.1s pré-pouso) | o pulo encadeado "simplesmente sai" ao tocar o chão | sem isso, pular-ao-pousar exige timing frame-perfect |

> 🪜 **Conexão crítica:** estes mesmos truques são o que faz o **juggle aéreo sentir bom** depois (§5). O "apex hang" do pulo é prima do "hang-time" do alvo juggleado ([16 §3](../combat/16_Aerial_Combos.md)). Construa-os no CMC **agora** — você está tunando o pilar, não polindo o traversal.

### 4.3 Sprint (cadência, não só velocidade)

Sprint ([09 §3](../locomotion/09_Gaits.md)) é um **burst** de reposicionamento (Run 500 → Sprint 750). O feel:

- **Engate:** segurar `IA_Sprint` (`Started`) deve sentir uma **mudança de marcha** perceptível. A diferença 500→750 é sutil em topdown — por isso o **feedback** (trail/FOV, [09 §3.3](../locomotion/09_Gaits.md)) é o que *vende* o sprint. (O feedback em si é juice → [21](21_Juice_FX.md); aqui só registramos que sem ele o sprint sente "igual a run".)
- **Cadência:** com `MaxAcceleration` alta, o ramp 500→750 é quase imediato — bom. Não interpole devagar "pra sentir peso": em topdown o jogador quer a velocidade *agora*.
- **Quebra de sprint:** sprint **cancela ao atacar** ([09 §3.2](../locomotion/09_Gaits.md)) — decisão de combate. Sentido: o jogador *compromete* — corre OU luta. Isso é bom (cria intenção), desde que o re-engate seja instantâneo ao soltar o ataque.

---

## 5. Feel aéreo: a flutuação, o hang, o peso do slam 🪂

O combo aéreo é o **pilar** ([01 §4](../design/01_Game_Vision.md), [16](../combat/16_Aerial_Combos.md)). Aqui tratamos só o **lado cinestésico** — como o ar deve *pesar* na mão. O espetáculo (trails, zoom, shake do slam) é juice → [21](21_Juice_FX.md).

O ar do combo tem **três tempos**, cada um com um feel próprio:

| Tempo | O que a mão sente | Knob de feel | Valor inicial |
|---|---|---|---|
| **1. O pop (launcher)** | o alvo (e o player, se follow) **salta** — vertical, autoritativo, previsível | `AltitudeAlvo` via `RootMotionSource` (**não** Launch balístico) | **250–350 cm** ([16 §2](../combat/16_Aerial_Combos.md)) |
| **2. A flutuação (juggle)** | o alvo **paira** — cada hit "segura" o tempo, o ar é "pegajoso" no bom sentido | `Hang-time` por hit + `PopHeight` re-float | **~0.35 s** hang · **120–180 cm** pop ([16 §3](../combat/16_Aerial_Combos.md)) |
| **3. O peso (slam)** | o golpe **desce com força** — comprometido, sem volta, o ar "endurece" no impacto | velocidade de descida + hitstop maior no toque | descida rápida (RootMotion down) · hitstop **3–5 frames** ([15 §5](../combat/15_Combat_Overview.md)) |

### 5.1 A "flutuação" — por que o ar deve ser pegajoso

No chão, o ritmo é horizontal e rápido. No ar, o feel inverte: **o tempo desacelera**. Cada hit aéreo dá um "pop" curto que **reseta a queda** do alvo ([16 §3](../combat/16_Aerial_Combos.md)) — a sensação é de estar *segurando* o inimigo no ar com os próprios golpes, como malabarismo (o nome é literal: *juggle*). O `decayFactor` 0.85 faz cada pop dar menos altura → o combo tem um **arco natural de descida**, não paira pra sempre (sem isso vira "pudim flutuante", [16 §3](../combat/16_Aerial_Combos.md)).

> 🎈 **O "hang" é a assinatura tátil do pilar.** Se o alvo cai rápido demais, o jogador sente *pressa* e *frustração* (não dá tempo de encadear). Se paira demais, sente *mole* e *sem peso*. O ponto doce — ~0.35s de hang por hit, decaindo — é onde o ar sente **controlável mas urgente**. Isso é número, não intuição: use `ddr.JuggleDebug 1` ([16 §3](../combat/16_Aerial_Combos.md)) pra *ver* altura/PopHeight/decay enquanto tuna.

### 5.2 Hitstop no ar **é feel de peso** (cuidado técnico)

O hitstop ([15 §5](../combat/15_Combat_Overview.md)) é **freeze global** — congela a anim **e a velocidade (CMC) dos dois corpos** por N frames, e só **depois** aplica o pop/launch. Por que isto é um item de *feel* e não de juice: se o freeze **não** congelar o CMC do alvo, o re-float "vaza" durante o congelamento, o corpo **desliza** no ar, e o peso de cada hit **evapora** — o jogador sente o ar "molhado" sem saber por quê. O peso sentido de cada pancada aérea *depende* do freeze ser global.

> ⚙️ Single-player torna isso **trivial e seguro**: pausar montage + zerar velocity local, sem nenhum rollback de rede. É exatamente o tipo de "agressão de feel" que o local-authoritative libera ([Index, Decisões](../00_Index.md)). Aproveite.

### 5.3 Follow launch — o feel ainda não resolvido ⚠️

O **follow launch** (player sobe junto do alvo, [16 §2](../combat/16_Aerial_Combos.md)) é o que dá a fantasia DMC: você *vai junto* pro ar, não fica olhando de baixo. Cinesteticamente é **essencial** pro pilar — sem ele o combo aéreo sente "remoto", como controlar algo à distância.

**Mas** é a "pergunta de um milhão" ([16 §6](../combat/16_Aerial_Combos.md), [Design Review tensão 3](../design/Design_Review_2026-06.md)): em topdown, o player subindo pode sumir sob o próprio sprite ou virar massa ambígua com o inimigo. **Isto é uma tensão feel × leitura, e o spike M⁻¹ ([Roadmap](../17_Implementation_Roadmap.md)) existe pra resolver.** Do lado *feel*, registre as três alternativas a sentir no spike:

1. **Follow total** (player = altura do alvo) — máximo de fantasia, máximo de risco de leitura.
2. **Follow parcial** (player sobe menos que o alvo) — meio-termo; o player "acompanha" sem colar.
3. **Player no chão** (só o alvo sobe) — leitura segura, mas o ar sente remoto.

> 🧪 **Critério de feel pro spike:** depois de lançar, *você* sente que **está no combo** (no controle do que sobe) ou que está **assistindo** o inimigo subir? Se for "assistindo", o follow precisa de mais presença — mesmo que custe leitura (a leitura se resolve com juice/câmera; a sensação de presença, não).

---

## 6. Cancelamento como experiência sentida 🥋

> As **regras** (o que cancela o quê, em qual janela, pra qual estado) estão no [combat/18 — Combat System Deep](../combat/18_Combat_System_Deep.md) e na tabela de [15 §6](../combat/15_Combat_Overview.md). Aqui: **como SENTE**, e por que o feel de cancelamento *é* o skill ceiling.

Cancelamento é a diferença entre um combate que é **bonito por 5 minutos e raso pra sempre** (on-rails) e um que tem **expressão infinita** ([Design Review, Kael](../design/Design_Review_2026-06.md)). O sentido tátil:

- **Recovery frames = comprometimento.** Quando você ataca, há um período de "recuperação" onde está vulnerável. Isso é **bom** — cria peso, intenção, risco. Um combate sem recovery (tudo cancelável instantâneo) vira mush sem tensão.
- **Janelas de cancelamento = liberdade dentro do comprometimento.** Dentro de janelas específicas, certas ações *cortam* a recovery. O jogador que **conhece** as janelas flui; o que não conhece, espera. Essa diferença de fluidez **entre dois jogadores apertando os mesmos botões** é o skill expression.

### Os três cancelamentos-âncora (o feel de cada um)

| Cancelamento ([15 §6](../combat/15_Combat_Overview.md)) | Como SENTE | Por que é gostoso |
|---|---|---|
| **Dash-cancel (sempre)** | "saída de emergência" sempre disponível — corto qualquer golpe e saio | converte comprometimento em **escolha**: ataco sabendo que posso abortar. Remove o medo de "travar" no golpe errado |
| **Attack → Launcher (na janela)** | a **ponte chão↔ar** — sinto o combo "decolar" sob meu comando | é o momento em que o combo de chão *vira* o pilar aéreo. O timing da janela = a recompensa de saber a hora |
| **Jump-cancel do AirAttack** | "re-subo" o player no juggle — estendo o ar com meus próprios pulos | o truque do combo profundo: quem domina, mantém o alvo no ar mais tempo. **Pura expressão** |

> 🔑 **Por que "dash-cancel-sempre" sente tão bem:** ele resolve a ansiedade fundamental do hack'n'slash — *"e se eu apertar o golpe errado e ficar preso?"*. Com dash-cancel universal, **nenhum input é uma armadilha**. Você ataca com confiança porque a saída está sempre a um botão. Esse é o destravamento psicológico que faz o combate sentir **seu**, não scriptado.

> 🎚️ **O equilíbrio de feel:** recovery curto demais = sem peso (botão-mash vence). Recovery longo demais sem janelas de cancel = punitivo, "travado". O ponto doce: golpes **comprometem o bastante pra ter peso**, mas têm janelas generosas o bastante pra **recompensar quem conhece**. Tune as *janelas* (não a existência da recovery) — alargar/encolher janela é o dial de "quão técnico" o combate sente.

---

## 7. Câmera como feel (o lado de controle) 🎥

> O **shake** e o **zoom-punch** de impacto são juice → [21](21_Juice_FX.md). Aqui: a câmera como **instrumento de controle** — lag, lookahead, framing, follow no juggle. A pergunta é "consigo ler e controlar?", não "que efeito legal?".

A câmera topdown ([06](../systems/06_Camera_TopDown.md)) é **fixa** (pitch -55°, yaw -45°, `bUsePawnControlRotation=false`) — não orbita. Isso é uma decisão de *feel*: a tela é um plano estável, o "pra cima" é sempre o mesmo, o jogador nunca se desorienta. Os knobs de controle:

| Knob | O que faz ao feel | Valor inicial | Prioridade |
|---|---|---|---|
| **Camera lag** (`bEnableCameraLag`, `CameraLagSpeed`) | suaviza o follow — a câmera "respira" atrás do player em vez de colar rígido | **10** ([06 §2](../systems/06_Camera_TopDown.md)) | 🟢 P0 |
| **Lookahead pela velocidade** | a câmera **antecipa** a direção do movimento → você vê mais à frente pra onde vai | offset proporcional à `Velocity` (tune) | 🟡 P1 ([06 §5](../systems/06_Camera_TopDown.md)) |
| **Framing de combate** | leve zoom-in em combate, zoom-out explorando → o jogo "respira" com o ritmo | zoom contextual | 🟡 P1 |
| **Follow no juggle** | leve zoom-out / reframe quando o combo sobe → a ação vertical **cabe na tela** | dolly-out sutil no `Airborne` | 🟡 P1 (essencial pro pilar) |

> 🎚️ **O feel da camera lag:** muito baixa (lag rígido) = câmera "nervosa", cada microajuste do player treme a tela → cansa. Muito alta = câmera "preguiçosa", o player "foge" do centro e some na borda → perde controle. **10** é um ponto de partida sólido; tune sentindo: a câmera deve dar a impressão de *seguir com intenção*, não de estar amarrada nem solta.

> 🪂 **Follow no juggle é knob de FEEL, não só de juice:** quando o combo sobe, se a câmera não reenquadra, o player e o alvo podem **encostar na borda superior** ou sumir — e aí você perde a **leitura de altura** que as sombras tentam dar ([06 §4](../systems/06_Camera_TopDown.md)). O zoom-out no juggle é sobre **conseguir controlar o que você não consegue ver** — por isso é controle (feel), não enfeite. (O *shake* do slam, esse sim, é juice → [21](21_Juice_FX.md).)

> ⚠️ **Lookahead × topdown:** em topdown o lookahead é mais sutil que em side-scroller (você se move em 2 eixos no plano), mas ainda ajuda: deslocar a câmera ~10–20% na direção do movimento dá "espaço pra frente" sem desorientar. Não exagere — lookahead forte demais faz o player sentir que "não está no centro do próprio jogo".

---

## 8. Tabela-mestra de parâmetros de feel 🎛️

> Todos os knobs cinestésicos num lugar. **Valores iniciais — feel se tuna sentindo, não calculando.** Origens nas seções inline (§2–§7) e docs citados. (VFX/SFX/shake/slow-mo **não** estão aqui → [21](21_Juice_FX.md).)

| Categoria | Knob | Valor inicial | Efeito sentido |
|---|---|---|---|
| **Movimento** (CMC) | `RunSpeed` (base) | **500** | velocidade default (Run, não Walk) |
| | `SprintSpeed` | **750** | burst de reposicionamento |
| | `MaxAcceleration` | **2048** (1500–2500) | arranque (alto = sem patinar) |
| | `BrakingDecelerationWalking` | **2048** | crava ao parar (sem deslizar) |
| | `RotationRate` (Yaw) | **720 °/s** | vira pra onde anda (alto = snap) |
| **Vertical / ar** (CMC) | `JumpZVelocity` | **600** (600–750) | altura do pulo |
| | `GravityScale` | **1.75** (1.5–2.0) | peso da queda (>1 = melhor) |
| | Gravidade na descida | **×1.2–1.5** | apex flutuante + queda pesada |
| | `AirControl` | **0.4** (0.3–0.5) | manobra no ar |
| | Landing retain / brake | **0.4 / ~4096** | anti-slide no pouso |
| **Verbos** | Dash distância | **500–650 cm** | reposiciona sem perder precisão |
| | Dash duração | **0.20–0.25 s** | snappy, não "voo" |
| | Dash i-frames | **0.20–0.30 s** (frente-carreg.) | "confio no botão" |
| | Dash cooldown | **~0.5–0.8 s** ([19](../combat/19_Abilities_Deep.md)) | impede spam, sente "sempre lá" |
| | Coyote time | **~0.10 s** | perdão de borda |
| | Jump buffer | **~0.10 s** | pulo encadeado "sai sozinho" |
| **Buffer / resp.** | Latência-alvo (verbos) | **≤ 50 ms** | "instantâneo" |
| | Buffer de combo | **0.20–0.35 s** | combo nunca engasga |
| | Janela de combo (montage) | **0.30–0.50 s** | tempo pra encadear |
| | Dash buffer (recovery) | **~0.15 s** | enfileira o dash-cancel |
| **Ar de combate** | AltitudeAlvo (pop launcher) | **250–350 cm** | o "salto" inicial, dirigido |
| | Hang-time por hit | **~0.35 s** | o ar "pegajoso" / controlável |
| | PopHeight por hit | **120–180 cm** | re-float de cada pancada |
| | decayFactor | **0.85 / hit** | arco natural de descida |
| | Hitstop chão / slam | **2–3 / 3–5 frames** ([21](21_Juice_FX.md)) | peso do impacto / clímax |
| **Câmera** | `CameraLagSpeed` | **10** | follow "com intenção" |
| | Lookahead (offset) | ~10–20% da velocidade | "espaço pra frente" |
| | Zoom-out no juggle | dolly sutil em `Airborne` | cabe a ação vertical |

---

## 9. Como TESTAR feel 🧪

Feel não se prova lendo a tabela — se prova **com a mão no controle**. Métodos:

### 9.1 O teste dos "3 estranhos" 👥

Dê o controle a **3 pessoas que nunca jogaram**, sem explicar nada, e **observe calado**:

- Eles **andam, param e viram** com confiança nos primeiros 10 segundos? (movimento §3) — se hesitam, accel/rotation estão errados.
- Eles **descobrem o dash sozinhos** e voltam a usá-lo? (se usam uma vez e abandonam → i-frames mesquinhos ou cooldown-feel ruim, §4.1).
- No combo aéreo, eles **entendem que o inimigo subiu**? (leitura §5/§7) — se perguntam "o que aconteceu?", o follow/sombra falhou.
- Eles dizem **"de novo"** sem você pedir? Esse é o sinal verde do feel.

> 🗣️ **Regra de ouro:** se você precisa *explicar* por que o movimento é bom, ele não é. Feel é pré-verbal — ou a mão entende, ou não.

### 9.2 Checklist de feel (o "smell test")

- [ ] **Andar:** começa sem patinar, para onde solto o stick (accel/braking 2048).
- [ ] **Virar:** olha pra onde ando sem "arco de barco" (RotationRate 720).
- [ ] **Latência:** todo verbo (dash/attack/jump) reage em ≤ 50ms — o **primeiro frame** sai no input.
- [ ] **Buffer:** apertar o combo um tico cedo **funciona** (não engasga) — buffer 0.2–0.35s.
- [ ] **Dash:** arranca (não acelera), salva (i-frames generosos), sempre disponível.
- [ ] **Pulo:** sobe leve, cai pesado (gravidade assimétrica); tap ≠ hold (variable height).
- [ ] **Cancelamento:** dash corta qualquer golpe a qualquer momento (sem "travar").
- [ ] **Ar:** o alvo **paira** (hang ~0.35s), não despenca nem flutua eterno; cada hit **pesa** (hitstop global).
- [ ] **Slam:** desce com força, o impacto sente o **fim** do combo (peso, comprometimento).
- [ ] **Câmera:** segue com intenção (lag 10), não treme nem foge; no juggle, a altura é **legível**.
- [ ] **Follow launch:** depois de lançar, sinto que **estou no combo**, não assistindo (§5.3 — questão do spike).

### 9.3 O que "bom" sente 🟢

Quando o feel está certo, o jogador relata (ou demonstra) estas sensações — use-as como alvo qualitativo:

| Sensação | Sinal de que está certo |
|---|---|
| **"Cola na minha mão"** | resposta ≤50ms, accel alta — zero atraso percebido |
| **"Flui"** | dash-cancel + buffer — o loop nunca trava entre ações |
| **"Pesa na hora certa"** | hitstop global — impacto e slam têm peso, o resto é leve |
| **"Confio no botão"** | i-frames generosos, buffer perdoador — sem medo de apertar |
| **"De novo"** | a soma de tudo — o loop é gostoso o bastante pra repetir sem recompensa externa |

> 🎯 **A meta cinestésica do MVP** (após o spike, [16 §10](../combat/16_Aerial_Combos.md)): com **1 launcher + 1 hit aéreo + sombra**, o combo aéreo já sente **divertido e legível**. Se sente, *o jogo tem alma* — e o feel está no caminho. Se não, nenhum knob de juice salva: volte pra esta mesa.

---

## 10. Troubleshooting 🔧

| Sintoma (o que a mão sente) | Causa provável | Onde olhar |
|---|---|---|
| "Patina" pra começar a andar | `MaxAcceleration` baixa | §3.1 — suba p/ 2048 |
| Desliza ao parar | `BrakingDecelerationWalking` baixo | §3.1 / [09 §7](../locomotion/09_Gaits.md) |
| Vira como um "barco" | `RotationRate` baixo | §3.1 — 720 ([06 §3](../systems/06_Camera_TopDown.md)) |
| Verbos sentem "atrasados" | input em `Triggered`/`Hold`, ou delay antes da montage | §2.1 / [07 §8](../systems/07_Input.md) — use `Started`, front-load |
| Combo "engasga", exige timing perfeito | sem buffer ou buffer curto | §2.2 / [07 §5](../systems/07_Input.md) |
| Combate sente "preso", medo de atacar | sem dash-cancel universal | §6 / [15 §6](../combat/15_Combat_Overview.md) |
| Pulo "lunar", sem autoridade | `GravityScale` ≤1, sem assimetria | §4.2 / [13 §3](../locomotion/13_Jump_Fall_Landing.md) |
| Dash sentido inútil, jogador abandona | i-frames mesquinhos / curtos | §4.1 — alargue, frente-carregados |
| Alvo despenca, não dá pra combar | hang-time/PopHeight baixos OU hitstop não-global (vaza) | §5.1/§5.2 / [16 §3](../combat/16_Aerial_Combos.md) |
| Impacto "molhado", sem peso | hitstop pausando só a montage (CMC desliza) | §5.2 / [15 §5](../combat/15_Combat_Overview.md) — freeze global |
| Combo aéreo sente "remoto" | sem follow launch (player fica no chão) | §5.3 — testar no spike |
| Câmera "nervosa" ou "preguiçosa" | `CameraLagSpeed` mal tunado | §7 — ~10, sentir |
| Altura do combo ilegível (perco controle no ar) | sem zoom-out/follow no juggle | §7 / [06 §4](../systems/06_Camera_TopDown.md) |

---

## 11. Próximo passo

→ [feel/21 — Juice & FX](21_Juice_FX.md): o **eco** — VFX, SFX, screenshake, hitstop-como-espetáculo, time-dilation, rumble, damage numbers. (Este doc deu o **tato**; o 21 dá o que os sentidos recebem de volta.)
→ [combat/18 — Combat System Deep](../combat/18_Combat_System_Deep.md): as **regras** de cancelamento e frame data (a tabela que este doc só *sentiu*).
→ Cross-refs de base: [06 — Câmera](../systems/06_Camera_TopDown.md) · [07 — Input/Buffer](../systems/07_Input.md) · [13 — Jump/Fall/Landing](../locomotion/13_Jump_Fall_Landing.md) · [16 — Combos Aéreos](../combat/16_Aerial_Combos.md).
