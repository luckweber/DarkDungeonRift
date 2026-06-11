# 60 — M2 Editor Setup (passo a passo) · 🟢 P0 ★ O PILAR

> **O que fazer no Unreal Editor** depois de compilar o C++ do **M2 — VOAR** (launcher → juggle → slam). O código entrega tudo; **você** cria 3 montages, 2 inputs, BPs filhos e a **sombra no chão**. Inclui o **dash-attack** (estilo Hades) e a resposta sobre **editor de combo com nodes** (§8).

> **Pré-req:** M1 completo ([55](55_M1_Editor_Setup.md)). **Classes C++ novas:** `UGA_Launcher` · `UGA_AirAttack` · `UGA_AirSlam` · `UGE_DDRCooldownLauncher` + API airborne em `ADDRCharacterBase` (StartAirborne/ApplyAirPop/EndAirborne) + estado aéreo no `UDDRCombatComponent` (EnterAirCombat/RefreshAirHold/ExitAirCombat) + flags novos no `ANS_DDRHitbox` (`bLaunchTargets`/`bAirPop`/`bAoEAtOwner`/`bSlamDownTargets`) + **dash-attack opener** (estilo Hades) no `GA_AttackLight` + **soft-lock + Motion Warping** (`UMotionWarpingComponent`, `FaceAndSetupMotionWarp`, perfis por ability).

> **Docs:** [16 Combos Aéreos](../combat/16_Aerial_Combos.md) · [18 Combate Profundo §6](../combat/18_Combat_System_Deep.md) · [19 Abilities](../combat/19_Abilities_Deep.md) · [17 Roadmap M2](../17_Implementation_Roadmap.md)

---

## 0. Checklist rápido

- [ ] Compila com M2 C++
- [ ] `IA_Launcher` + `IA_AirSlam` no IMC (§1)
- [ ] `AM_Launcher` (§2) · `AM_AirCombo` (§3) · `AM_AirSlam` (§4) · `AM_RunAttack` (§6)
- [ ] BPs filhos com montages + Startup Abilities trocadas (§5)
- [ ] **Sombra blob** no player e no dummy (§7) — leitura do pilar!
- [ ] **Motion Warping** nas montages de ataque (§7.3) — fecha o gap pro alvo!
- [ ] PIE: lançar → combar no ar → slam (§9)

---

## 1. Inputs novos

| Asset | Tecla ([39](39_Controls.md)) | Trigger |
|---|---|---|
| `IA_Launcher` | **E** / Gamepad B | Started |
| `IA_AirSlam` | **R** / Gamepad Y | Started |

Adicione ao `IMC_DDR_Default` e assigne nos slots **Launcher Action** / **Air Slam Action** do `BP_DDRPlayer`. *(LMB continua sendo o ataque — no ar ele vira air-combo sozinho, gate por tag.)*

---

## 2. `AM_Launcher` — o uppercut que SOBE o player

**Clip:** `Attack_Up_Floor_To_Air_02_Seq` (o nome diz tudo: floor→air — o **follow launch já está autorado** no root motion!).

1. Crie a montage **`AM_Launcher`** com esse clip (sem seções). Slot `DefaultSlot`.
2. **`EnableRootMotion = ✅ TRUE`** na sequence — é o RM que sobe o player (Root Lock: *Ref Pose* default ok).

> ⚙️ **Por que o código põe o player em `MOVE_Flying` durante a montage:** em **`MOVE_Walking` o CMC DESCARTA o Z do root motion** (gruda o personagem no chão) — sintoma clássico: *"pulinho curto e trava, como root lock"*. O `GA_Launcher` troca pra Flying ao ativar e restaura `Falling` no fim **se não entrou no ar** (whiff/cancel caem de volta com a gravidade). Você não configura nada — só recompile.
3. Adicione **`ANS_DDRHitbox`** no swing com: `Base Damage 15` · `Hit Stop Frames 3` · **`bLaunchTargets = ✅ TRUE`** ← é isso que lança o alvo.

> 🎯 **Regra do código:** o player só **entra no ar** se o swing **lançou alguém** (whiff = termina no chão, sem voar à toa). O alvo sobe **dirigido** (`LaunchRiseHeight`, default 300 cm) e segura ~1.1s (renovado por hit). `Attack_Up_01_Seq` (uppercut que NÃO sobe) fica como launcher alternativo P1.

> 📐 **Tuning launcher (altura player × inimigo):** o **pulo do player** vem do **root motion** do clip `Attack_Up_Floor_To_Air` — o C++ **não** calcula isso. No fim da montage, `EnterAirCombat` ancora o player na **altura atual do RM** (`AirAnchorZ`) e **alinha o inimigo** ~`JuggleTargetHeightAbovePlayer` acima — o player **nunca** é puxado para baixo.
>
> **Onde tunar (preferido):**
> | Ability BP | Campo | Uso |
> |---|---|---|
> | `BP_GA_Launcher` | **Launch Rise Height** | altura do inimigo no uppercut (250–350) |
> | `BP_GA_Launcher` | **Juggle Target Height Above Player** | co-altitude ao *entrar* no ar — **negativo = alvo abaixo do player** |
> | `BP_GA_AirAttack` | **Juggle Target Height Above Player** | co-altitude a cada hit (`bAirPop`) — negativo (~-30 a -60) "assenta" o alvo na lâmina dos swings descendentes |
> | `BP_GA_AirAttack` | **Air Pop Vertical Nudge Scale** | nudge extra por hit (~0.15) |
>
> O **Combat Component** no player guarda só **fallback** global — as abilities copiam os valores ao ativar.

---

## 3. `AM_AirCombo` — o juggle

**Clips:** `Attack_Up_Air_To_Air_03_Seq` + um dos `Combo_Attack_Air_06/Wave_07`.

1. Montage **`AM_AirCombo`** com seções `Air1`–`Air4` (ou 2 no MVP). ⚠️ **Montage Sections → Clear!**
2. **Root Motion nos clips aéreos** (escolha **uma**):
   - **✅ Recomendado:** `EnableRootMotion = TRUE` — o swing **sincroniza** cápsula ↔ mesh (sem slide visual). O C++ **fixa o Z** (`AirAnchorZ` + `MaintainAirAltitude`) — você **não cai** mesmo com RM ligado.
   - **Alternativa:** `EnableRootMotion = FALSE` — só se o clip for **100% in-place** (zero translação no root). Senão a mesh “desliza” e a posição parece bugada.
   - **Root Motion Root Lock:** `Ref Pose` (default) · **Force Root Lock** OFF.
3. **Sem** notify Motion Warping no air combo (drift indesejado) — ver §7.3.
4. Por seção: **`ANS_DDRHitbox`** com **`bAirPop = ✅ TRUE`** + **`ANS_DDRComboWindow`**.
5. **Carry do alvo (set 06 vs 07):**

| Set de clips | Comportamento no ar | `bCarryAirborneTargets` no `ANS_DDRHitbox` |
|---|---|---|
| **`06_Combo_Attack_Air_06`** | player **avança** (RM horizontal) | ✅ **ON** — inimigo segue no XY (~`AirCarryForwardOffset` 90 cm) |
| **`07_Combo_Attack_Air_Wave_07`** | player **parado** no ar | ❌ **OFF** — alvo fica na posição do launch |

> 🔑 **`bCarryAirborneTargets`** no hitbox: enquanto o notify está ativo (e depois do 1º hit), o alvo `Airborne` **interpola** XY até ficar à frente do player — o juggle não “fica pra trás” quando o combo avança.

> O `GA_AirAttack` **herda** toda a máquina do combo de chão (seções/janela/buffer) — e cada golpe **renova o hold aéreo** do player (sem atacar por 1.4s → cai sozinho).

---

## 4. `AM_AirSlam` — o finisher (3 fases: Start → Loop → End)

**Clips:** `Attack_Air_To_Floor_Start_01_Seq` + `Attack_Air_To_Floor_Loop_01_Seq` + `Attack_Air_To_Floor_End_01_Seq` (pasta `08_Attack_Air_to_Floor_01`).

1. Montage **`AM_AirSlam`** com **3 seções**: `Start`, `Loop`, `End` (nomes **exatos**). ⚠️ **Montage Sections → Clear** como sempre — quem encadeia é o **C++**: na ativação ele liga `Start→Loop` e **self-loopa** o `Loop` (`Montage_SetNextSection`); o **pouso** (`LandedDelegate`) pula pra `End`. A queda dura o que durar — o Loop dá quantas voltas precisar.
2. **Root Motion:** `Start`/`Loop` = **❌ OFF** (clareza) — mas o código agora **força `IgnoreRootMotion` durante o slam** (restaurado no fim): mesmo com RM ON no clip, a descida é sempre a velocity -3500. *(Antes, RM ON fazia a anim dirigir a cápsula e matar a queda — player "boiando".)*
3. **NÃO** precisa de hitbox notify: o **AoE do impacto é o código** — uma **COLUNA vertical** (raio 250 × altura `Slam Vertical Reach` 450, tunável no `BP_GA_AirSlam`): dano 25, hit-stop 6 frames, **derruba todos os Airborne** — inclusive o alvo juggleado lá no alto — + `GameplayCue.Slam`.
4. **Alvo seguro + homing (código, nada a configurar):** ao ativar, o slam **(a)** estende o hold do alvo juggleado (`Slam Target Hold Seconds` = 2s no Combat Component); **(b)** **mira o XY do alvo** na descida (`bHomeToTarget`; cap `Slam Homing Max Distance` 350cm = whiff honesto; vel. máx 1800) — a coluna **sempre conecta** no alvo perseguido. ⚠️ Motion warping de engine **não** serve aqui: warp é *root motion*, e a descida é *velocity*.
5. **Coreografia do impacto (feel, código):** o golpe do **apex** (frame da ativação, o swing do `Start`) **já arremessa o alvo** — ele cai a `SlammedFallVelocity` **-4500** (na base do personagem; mais rápido que o player a -3500) e **esmaga no chão PRIMEIRO**. Beats de juice: **hit-stop 2 frames no apex** (`Slam Grab Hit Stop Frames`) + **hit-stop 6 no impacto** (AoE) + `GameplayCue.Hit.Light` no apex e `GameplayCue.Slam` no pouso — os cues são os **ganchos pro juice visual** (shake/poeira/trail = BP, [21](../feel/21_Juice_FX.md)). Toggle: `bSlamClaimedTargetOnActivate` (OFF = alvo só cai com o AoE do pouso, comportamento antigo).

> 🎮 **"Segurar R pra loopar?" NÃO.** O `Loop` existe pra **distância de queda variável** (anchor mais alto, desnível de arena) — quem decide quantas voltas ele dá é o **tempo de queda**, não o input. Slam é **comprometido** ([15 §6](../combat/15_Combat_Overview.md)): apertou R, desce até o impacto. Pairar segurando botão conflitaria com o auto-drop do juggle e tiraria o peso do finisher.

> 🧷 **MVP 2 seções ainda vale:** sem a seção `Loop`, se o clip `Start` acabar antes do pouso o código **segura a ability** até o `LandedDelegate` — o AoE nunca se perde (build antiga encerrava cedo e o slam "secava").

---

## 5. BPs filhos + Startup Abilities

| BP (parent) | Campo | Asset / nota |
|---|---|---|
| `BP_GA_Launcher` (← `GA_Launcher`) | Launcher Montage | `AM_Launcher` |
| `BP_GA_Launcher` | **Launch Rise Height** · **Juggle Target Height Above Player** | tuning aéreo (§2) |
| `BP_GA_AirAttack` (← `GA_AirAttack`) | Combo Montage | `AM_AirCombo` |
| `BP_GA_AirAttack` | **Juggle Target Height Above Player** · **Air Pop Vertical Nudge Scale** | tuning do juggle (§3) |
| `BP_GA_AirSlam` (← `GA_AirSlam`) | Slam Montage | `AM_AirSlam` |
| `BP_GA_AttackLight` (já existe) | **Dash Attack Montage** | `AM_RunAttack` (§6 — dash-attack) |

No `BP_DDRPlayer` → **Startup Abilities** (5 entradas): troque as classes C++ pelos BPs:
`BP_GA_AttackLight`(Attack) · `BP_GA_AirAttack`(**Attack** — mesmo botão!) · `BP_GA_Dash`(Dash) · `BP_GA_Launcher`(Launcher) · `BP_GA_AirSlam`(AirSlam).

> 🔑 **Mesmo botão, ability certa:** `GA_AttackLight` bloqueia `State.Combat.InAir`; `GA_AirAttack` exige essa tag. A tag entra **no hit do launcher** (`bLaunchTargets`) — não é só "pular" ou estar em Falling. O ASC prioriza `GA_AirAttack` quando a tag está ativa ([19 §roteamento](../combat/19_Abilities_Deep.md)).

---

## 6. ⚔️ Dash-Attack (estilo Hades)

**Clip:** `Run_Attack_01_Seq` (pasta `12_Run_Attact`) — a estocada já parece "dash + golpe": é exatamente o que ela vira aqui.

1. Montage **`AM_RunAttack`** (sem seções; `EnableRootMotion = TRUE` — o avanço da estocada é autorado).
2. Notifies: **`ANS_DDRHitbox`** normal (Damage ~14) + **`ANS_DDRComboWindow`** na recovery.
3. Assign no `BP_GA_AttackLight` → **Dash Attack Montage** (grace: `Dash Attack Grace Seconds = 0.15`).

**Como funciona (código):** apertou ataque **durante o dash** (ou até **0.15s** depois — grace) → o código **cancela o dodge** e toca a estocada como **OPENER**; input na janela dela → **encadeia direto no Atk1** do combo normal. Parado/andando/correndo → combo normal direto. Zero botão novo.

> 🎯 **Por que dash+attack (e não correr+attack):** o gatilho por velocidade (versão antiga, ≥450 u/s) disparava sozinho em qualquer ataque em movimento — o jogador não *pedia* a estocada. **Dash→LMB é input deliberado** (Hades): o dash já orientou o corpo, a estocada completa o gap. Detecção: **ability do dash ativa** (cobre a recovery do dodge) — *não* a tag `State.Movement.Dashing`, que é a janela de **i-frame** (0.25s do GE) e tem outra semântica de propósito.

---

## 7. 🎯 Soft-lock + Motion Warping (P0 feel — acertar o inimigo)

> **Por que isto existe:** com **orient-to-movement**, o golpe sai na direção do stick, não do inimigo. O pipeline canônico ([18 §6](../combat/18_Combat_System_Deep.md)) é **4 camadas**:

```
1. Soft-lock   → escolhe o alvo óbvio (cone + distância)
2. Face        → gira o yaw no STARTUP (antes do swing)
3. Motion Warp → avança até ~90cm do alvo (cap de distância)
4. Sweep       → ANS_DDRHitbox confirma o hit
```

**Já no C++** (`UDDRCombatComponent` + `UMotionWarpingComponent` no personagem). **Você** configura as janelas nas montages.

### 7.1 Soft-lock (automático — tune no BP)

No `BP_DDRPlayer` → **Combat Component**:

| Campo | Default | Nota |
|---|---|---|
| `bSoftLockEnabled` | ✅ | desliga = ataque "reto" sem assist |
| `SoftLockRadius` | 600 | raio de busca (cm) |
| `SoftLockHalfAngleDegrees` | 75 | cone na direção do input |

| Ability | `bPreferAirborne` | Comportamento |
|---|---|---|
| Ground / Run / Launcher | false | prioriza inimigo no cone |
| Air / Slam | true | **+1000 score** pro alvo com tag `Airborne` |

Debug: `ddr.CombatDebug 1` → linha ciano owner→alvo.

### 7.2 Motion Warp — perfis por ability (C++)

Warp target name canônico: **`AttackWarp`** (não mude).

| Ability | Perfil | Max warp | O que faz |
|---|---|---|---|
| `GA_AttackLight` | **Ground** | 200 cm | lunge horizontal no swing |
| Dash-attack opener | **RunAttack** | 200 cm | mesma lógica, alcance maior na estocada |
| `GA_AirAttack` | **Air** | 120 cm | ajuste fino — alvo já está perto no juggle |
| `GA_Launcher` | **Launcher** | 180 cm | fecha gap **horizontal**; subida = RM do clip |
| `GA_AirSlam` | **Slam** | — | só **encara** o alvo; descida = velocity do código |

Tune no **Combat Component** do `BP_DDRPlayer`:

| Campo | Default |
|---|---|
| `bMotionWarpEnabled` | ✅ |
| `IdealHitDistance` | 90 cm |
| `MaxWarpDistance` | 200 cm |
| `MaxWarpDistanceAir` | 120 cm |
| `MaxWarpDistanceLauncher` | 180 cm |

> ⚠️ **Honestidade:** além do `MaxWarp*` o código **não warpa** — whiff real, não teleporte. Fora do cap = swing no vazio (valor de skill).

### 7.3 Montage — adicionar janela Motion Warping (EDITOR)

Em **cada** montage que avança o corpo (combo chão, dash-attack, launcher — **não** nos clips aéreos puros):

#### Combo seccionado (`AM_Combo`) — **uma janela por seção**

Montages com seções (`Atk1`–`Atk4`) precisam de **uma notify `Motion Warping` em cada seção**, não só na primeira. São **duas camadas** que trabalham juntas:

| Camada | Quem faz | Quando |
|---|---|---|
| **C++** | `FaceAndSetupMotionWarp` em `UGA_AttackLight` | No startup de **cada** golpe — Atk1 **e** ao `MontageJumpToSection` (Atk2/3/4) |
| **Montage** | Notify `Motion Warping` na timeline da seção | Só durante o **swing daquela seção** — a engine aplica o lunge |

- Só **Atk1** com warp → Atk1 alcança; Atk2+ **encaram** o alvo (soft-lock) mas **não avançam** = combo inconsistente.
- **Workflow:** comece com warp só em **Atk1** (§7.4) → valide no PIE → **replique** o mesmo notify em Atk2, Atk3 e Atk4 (mesmos pins: `AttackWarp`, Skew Warp, Ignore Z).
- Detalhe do combo no editor: [57 §2b](../combat/57_M1_Combo_Editor_Setup.md).

1. Na timeline, no **startup/swing** (antes do hitbox), adicione notify state: **`Motion Warping`** (engine).
2. No notify → **Root Motion Modifier** = **`Skew Warp`** (ou `Warp`).
3. Configure os **pins do notify** (mesmos valores em todas as seções/montages):

| Campo (Details) | Valor DDR | Nota |
|---|---|---|
| **Warp Target Name** | **`AttackWarp`** | ⚠️ **Nunca `None`** — tem que bater com `DDRMotionWarpNames::AttackWarp` no C++ |
| **Warp Translation** | ✅ ON | é o lunge horizontal |
| **Ignore Z Axis** | ✅ ON | chão/run/launcher — Z vem do chão ou do RM vertical do clip |
| **Mode** | `Linear` (default) | ok |
| **Warp Rotation** | ❌ **OFF** | **recomendado** — o C++ já encara no startup (`FaceSoftLockTarget`); ligar = risco de rotação dupla / "homing slash" ([18 §6.2](18_Combat_System_Deep.md)) |
| *Se Warp Rotation ON* | Rotation Type = **`Default`** | usa o yaw que o C++ passa em `AddOrUpdateWarpTargetFromLocationAndRotation`; **não** use `Facing` |
| *Se Warp Rotation ON* | Rotation Method = `Slerp`, Multiplier = `1.0` | defaults ok |

> 🔑 **Rotação = C++. Translation = montage.** Soft-lock vira o personagem **antes** do swing; o notify só **fecha o gap** (translation). Por isso `Warp Rotation` fica desligado no setup canônico.

| Montage | Janela Motion Warp? | Ignore Z? |
|---|---|---|
| `AM_Combo` (Atk1–4) | ✅ em cada seção, no swing | ✅ |
| `AM_RunAttack` | ✅ na estocada | ✅ |
| `AM_Launcher` | ✅ no passo antes do uppercut | ✅ |
| `AM_AirCombo` | ❌ (player em Flying; drift) | — |
| `AM_AirSlam` | ❌ (descida = código) | — |

4. **Personagem:** `MotionWarpingComponent` já vem no C++ — confira no `BP_DDRPlayer` (componente visível, `bSearchForWindowsInAnimsWithinMontages = true`).

Debug warp: `ddr.CombatDebug 1` → esfera magenta no warp point + log `MotionWarp profile=...`.

Console extra (engine): `MotionWarping.Debug 1`.

### 7.4 Ordem de setup recomendada

1. Tune soft-lock (§7.1) até o personagem **olhar** pro dummy.
2. Adicione Motion Warp em **Atk1** só → teste 1 golpe.
3. Replique pras outras seções / montages.
4. Só depois monte o juggle (§2–4).

---

## 8. 🪂 Sombra no chão (P0 do pilar — leitura de altura!)

Sem isto o juggle é **ilegível** em topdown ([16 §6](../combat/16_Aerial_Combos.md)):

1. Crie um material **`M_BlobShadow`**: domínio **Deferred Decal**, blend **Translucent**; um círculo radial preto (RadialGradientExponential → Opacity ~0.5).
2. No `BP_DDRPlayer` **e** no `BP_TrainingDummy`: **Add Component → Decal** → material `M_BlobShadow`, rotação (-90,0,0), tamanho ~(80, 80, 200).
3. O decal **projeta no chão** mesmo com o corpo no ar → o **gap sombra↔corpo = altura**. Teste: lance o dummy e veja se VOCÊ lê a altura sem pensar.

---

## 9. 🧩 "Editor de combo com nodes?" — a resposta honesta

Sua intuição (grafo com conectores + regras "se está no ar") está **correta** — e a boa notícia: **você JÁ TEM esse grafo**, só que expresso em **tags**:

| Conceito do "node editor" | Como já existe (GAS) |
|---|---|
| Nó (golpe) | uma `GameplayAbility` / seção de montage |
| Conector "se está no ar" | `ActivationRequiredTags: State.Combat.InAir` |
| Conector "cancela X" | `CancelAbilitiesWithTag` |
| Janela de dependência | `ANS_DDRComboWindow` + buffer |
| Debug visual | `showdebug abilitysystem` no console |

**Os 3 níveis (decisão registrada):**

| Nível | O quê | Quando | Custo |
|---|---|---|---|
| **1. Tags + abilities** (atual) | o grafo implícito acima | ✅ **MVP** — escala até ~10 moves tranquilo | zero |
| **2. Grafo data-driven** | `UDDRComboGraphData` (DataAsset: moves + transições {input, janela, requiredTags, nextMove}) lido por UMA ability genérica | 🟡 P1/P2 — quando vier a **2ª arma/aspectos** ([50](../combat/50_Weapons_Arsenal.md)) e o roster crescer | dias |
| **3. Node editor custom** (EdGraph/Slate) | UI visual de nós e setas | 🔵 P2+ — só com **dezenas de moves** ou designers no time | **semanas** de tooling |

> 🎯 **Veredito:** não construa o node editor agora — é a armadilha clássica de tooling antes de conteúdo. O nível 2 (data-driven) é o upgrade natural quando o doc 50 (armas) sair do papel: pega 80% do benefício por 5% do custo. Registrado também no [18 §4](../combat/18_Combat_System_Deep.md) (o grafo como dado).

---

## 10. Teste PIE (critério do M2 — o momento da verdade)

| Teste | Esperado |
|---|---|
| **E** no vazio (whiff) | uppercut toca, player **fica no chão** |
| **E** acertando o dummy | dummy sobe 300cm e **segura**; player **sobe junto** (RM do clip) e **trava no ar** |
| **LMB no ar** | air combo (Air1→Air2); cada hit **re-flutua** o dummy (cada vez menos — decay) |
| Parar de atacar no ar ~1.4s | player **cai** sozinho (auto-drop) |
| **R no ar** | desce em slam (Start→Loop) → no pouso pula pra End + **AoE coluna** (dano + hit-stop pesado) + dummy juggleado **despenca** |
| 7+ hits aéreos | dummy cai (cap do juggle) |
| **Dash → LMB** (durante o dodge ou ≤0.15s após) | dash cancela → **estocada** avança; janela → encadeia no combo |
| `ddr.CombatDebug 1` | sweeps + soft-lock (ciano) + warp point (magenta) + AoE slam |
| Acertar dummy ~2m de frente | soft-lock + warp | §7 — sem isso = swing no vazio |

**✅ M2 pronto quando:** lançar → combar → slam **é divertido e você LÊ a altura** (sombra!). Mostre pra alguém sem explicar — se entender "o inimigo subiu", o pilar está provado ([17 §4](../17_Implementation_Roadmap.md)). 🎯

### 10b. 🔬 Logs de refino (Output Log, filtre `LogDDR`)

Toda a cadeia de combate loga com prefixo + timestamp — é a régua pra tunar feel (cadência, janela, buffer):

| Prefixo | Cobre | Use para tunar |
|---|---|---|
| `[ATK]` | ativação (combo vs dash-attack), janela ABRE/FECHA, input na janela/buffer, buffer consumido, avanço de seção, fim/interrupção | **janela de combo** (notify cedo/tarde), `InputBufferSeconds`, cadência Atk1→Atk4 (Δt entre "avanca") |
| `[ATK-AIR]` | roteamento InAir, tuning do juggle, hold renovado | co-altitude/nudge, auto-drop |
| `[DASH]` | direção, seção 8-way, modo (RM/força), fim (cancelled=1 = dash-attack cortou) | `DashDistance/Duration`, grace do dash-attack |
| `[HIT]` | cada hit: alvo, seção, dano, blade sweep | dano por seção, hits fantasma |
| `[WARP]` | perfil, alvo, aplicado ou "fora do cap" | `MaxWarpDistance*`, soft-lock errando alvo |
| `[SLAM]` / `[AIRBORNE]` | ciclo do slam e do juggle do alvo | ver §4 |

**Como medir a cadência:** os `t=` são segundos de jogo — Δt entre `janela ABRE` e o `input na JANELA` mostra quão apertada a janela está; Δt entre dois `[ATK] avanca` é o ritmo real do combo.

---

## 11. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| **Editor crasha ao abrir** (stack em `GE_DDRCooldownLauncher` / `FindOrAddComponent`) | `FindOrAddComponent` no **construtor** do GE | Recompile — tags vão em `PostInitProperties` ([05 §5](05_GAS_Architecture.md)) |
| Player **não sobe** no launcher | acertou? (whiff = by design) · clip sem `EnableRootMotion` | §2 — TRUE no `Attack_Up_Floor_To_Air` |
| **"Pulinho curto e trava"** (parece root lock) | `MOVE_Walking` descarta o **Z** do root motion | corrigido no código (Flying durante a montage) — **recompile** |
| Player flutua após whiff/cancel do launcher | build antiga (sem restauração de Falling) | recompile — `EndAbility` restaura |
| Alvo não sobe | `bLaunchTargets` desmarcado no notify | §2 passo 3 |
| Alvo cai rápido demais | hold/pop baixos | `TargetAirborneHoldSeconds` no Combat Component (BP) |
| **Inimigo sobe muito acima do player** (no combo) | `AirPop` stackava +150cm/hit | tune **`BP_GA_AirAttack`** → Juggle Target Height (~60; **aceita negativo** = alvo abaixo do player) + Air Pop Nudge (~0.15) |
| **Soft-lock no inimigo no céu** (você no chão) | sem filtro de ΔZ | `SoftLockMaxVerticalGap` (~220) no Combat Component (fallback global) |
| **Player teleporta para baixo** após launcher | `AirAnchorZ` puxava pro Z do inimigo | recompile — ancora no **pulo do player**; inimigo alinha depois (§2) |
| Launcher: inimigo sobe pouco / player muito alto | `LaunchRiseHeight` ≠ ΔZ do clip RM | tune **Launch Rise Height** no `BP_GA_Launcher` (§2) |
| Player **cai** no air combo | RM com componente Z nos clips | recompile (`AirAnchorZ`) · ou RM OFF só em clips in-place |
| Mesh **desliza** / posição bugada | RM **OFF** em clip com translação | §3 — `EnableRootMotion=TRUE`; C++ fixa o Z |
| Drift horizontal no ar | motion warp no `AM_AirCombo` | §7.3 — sem warp aéreo |
| LMB no chão não ataca | recompile (Blocked InAir entrou no AttackLight) | rebuild + restart editor |
| **No ar, LMB ainda toca `AM_Combo`** (light) | pulo comum ≠ juggle · tag `InAir` só após **lançar** alguém | §2 — `bLaunchTargets` no notify do launcher; recompile (tag no hit) |
| AirAttack não ativa | `BP_GA_AirAttack` sem **Combo Montage** = `AM_AirCombo` | §5 · seções `Air1`–`Air4` |
| AirAttack não ativa | falta `BP_GA_AirAttack` no Startup Abilities (Input **Attack**) | §5 — **duas** abilities no mesmo InputID |
| **Crash em `GetDDRCombatComponent` / air combo** | build antiga chamava `InputPressed` manual no ASC | recompile — roteamento InAir usa só `TryActivate` + `Super` |
| Slam não muda pra "End" | nomes das seções (`Start`/`Loop`/`End` exatos) | §4 |
| **Dummy fica PENDURADO no ar pra sempre** (slam e hold expirado não derrubam) | dummy **sem Controller**: o CMC pula `PerformMovement` (gate `bRunPhysicsWithNoController`, default false) — `EndAirborne` setava Falling -3000 e **nada acontecia**. A subida funcionava (é `SetActorLocation` da base), só a QUEDA dependia do CMC | recompile — `bRunPhysicsWithNoController = true` no `ADDRTrainingDummy`. ⚠️ Vale pra **qualquer** alvo juggleável sem AIController |
| **Slam: dummy juggleado não cai nem toma dano** | (a) AoE antigo era **esfera no pouso** — alvo a ~300cm ficava fora · (b) hold do alvo (1.1s) **expirava na descida** e ele já estava no chão no impacto | recompile — AoE virou **coluna** (250 × 450) + slam **estende o hold do alvo** (2s); confira com `ddr.CombatDebug 1` (esferas vermelhas = hit) |
| Player desce devagar / "boiando" no slam | **RM ON** no clip `Start`/`Loop` dirigia a cápsula e anulava a velocity -3500 | recompile — código força `IgnoreRootMotion` no slam (§4); mantenha RM OFF por clareza |
| Slam desce reto e erra alvo distante | além do cap de homing (350cm) | design (whiff honesto) — tune `Slam Homing Max Distance` no `BP_GA_AirSlam` |
| **Slam "boia" num arco longo preso no Loop** (dummy já no chão) | **R durante a montage do launcher**: a tag `InAir` entra no hit mas o hold aéreo só começa no fim da montage → `ExitAirCombat` era no-op e sobrava a **velocity de SUBIDA** do launcher (log: `velZ` positivo + `ExitAirCombat IGNORADO`) | recompile — a descida virou **incondicional** na ativação (`Slam Fall Velocity` -3500 no `BP_GA_AirSlam`) |
| Slam "seca" (sem AoE) em queda alta | montage sem `Loop` acabava antes do pouso (build antiga encerrava a ability cedo) | recompile · adicione a seção `Loop` (§4) |
| Air combo toca as 2 seções de uma vez | seções linkadas | **Clear** ([57 §0](../combat/57_M1_Combo_Editor_Setup.md)) |
| Não leio a altura | sem sombra blob | §7 — P0! |
| Dash-attack nunca sai | LMB fora do dash/grace (0.15s) / montage não setada no slot **Dash Attack Montage** | §6 |
| **Depois do 1º juggle:** LMB no chão toca air combo / E não lança mais | tag `InAir` presa (count duplo — build antiga) | recompile — `SetLooseGameplayTagCount` zera no exit/whiff |
| **Bate no vazio** perto do dummy | sem Motion Warp / warp name errado | §7.3 — target **`AttackWarp`** (não `None`) |
| Personagem não avança no swing | falta notify Motion Warping na montage | §7.3 |
| Olha pro alvo, esfera magenta, **zero lunge** | `Warp Target Name` = `None` ou typo | §7.3 — digite **`AttackWarp`** exato |
| Golpe **gira estranho** / homing durante swing | `Warp Rotation` ligado | §7.3 — **desligue** Warp Rotation; face já vem do C++ |
| Warp "teleporta" demais | `MaxWarpDistance` alto demais | §7.2 — cap 200cm ground |
| Olha pro inimigo mas não alcança | só soft-lock, sem warp | §7 — as 4 camadas |
| **Atk1 alcança, Atk2+ bate no vazio** | warp só na 1ª seção | §7.3 — **replique** o notify em Atk2/3/4 |
| Air combo drifta (cai) | RM Z sem pin de altitude (build antiga) | recompile + §3 — RM ON ok com `AirAnchorZ` |

---

## 12. Mapa C++ ↔ Editor (soft-lock + warp)

| Feature | C++ | Editor |
|---|---|---|
| Soft-lock | `FindSoftLockTarget` / `FaceSoftLockTarget` | tune Combat Component |
| Motion warp | `FaceAndSetupMotionWarp` + `UMotionWarpingComponent` | notify `Motion Warping` → `AttackWarp` |
| Perfil ground | `GA_AttackLight::GetMotionWarpProfile` | `AM_Combo` warp windows |
| Perfil air | `GA_AirAttack` override | cap menor; sem RM nos clips |
| Perfil launcher | `GA_Launcher` | `AM_Launcher` warp + RM vertical do clip |
| Perfil slam | `GA_AirSlam` | só face; descida no código |
| Whiff honesto | `MaxWarpDistance*` | não aumente sem limite |

---

## 13. Próximo passo

→ **M3 — Lutar:** inimigos com IA ([30-33](../enemies/30_AI_Overview.md)) + o atirador ([37](../combat/37_Projectiles.md)). O dummy já "herda" o juggle — os inimigos do M3 também (API na base).
