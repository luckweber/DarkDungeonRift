# 60 — M2 Editor Setup (passo a passo) · 🟢 P0 ★ O PILAR

> **O que fazer no Unreal Editor** depois de compilar o C++ do **M2 — VOAR** (launcher → juggle → slam). O código entrega tudo; **você** cria 3 montages, 2 inputs, BPs filhos e a **sombra no chão**. Inclui o **run-attack** e a resposta sobre **editor de combo com nodes** (§8).

> **Pré-req:** M1 completo ([55](55_M1_Editor_Setup.md)). **Classes C++ novas:** `UGA_Launcher` · `UGA_AirAttack` · `UGA_AirSlam` · `UGE_DDRCooldownLauncher` + API airborne em `ADDRCharacterBase` (StartAirborne/ApplyAirPop/EndAirborne) + estado aéreo no `UDDRCombatComponent` (EnterAirCombat/RefreshAirHold/ExitAirCombat) + flags novos no `ANS_DDRHitbox` (`bLaunchTargets`/`bAirPop`/`bAoEAtOwner`/`bSlamDownTargets`) + **run-attack opener** no `GA_AttackLight` + **soft-lock + Motion Warping** (`UMotionWarpingComponent`, `FaceAndSetupMotionWarp`, perfis por ability).

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

> 🎯 **Regra do código:** o player só **entra no ar** se o swing **lançou alguém** (whiff = termina no chão, sem voar à toa). O alvo sobe **300cm dirigido** e segura ~1.1s (renovado por hit). `Attack_Up_01_Seq` (uppercut que NÃO sobe) fica como launcher alternativo P1.

---

## 3. `AM_AirCombo` — o juggle

**Clips:** `Attack_Up_Air_To_Air_03_Seq` + um dos `Combo_Attack_Air_06/Wave_07`.

1. Montage **`AM_AirCombo`** com **2 seções**: `Air1` (Air_To_Air) e `Air2` (Combo_Air). ⚠️ **Montage Sections → Clear!**
2. **`EnableRootMotion = ❌ FALSE`** nos clips aéreos — o player está **segurado em Flying** pelo código; RM aqui só causaria drift.
3. Por seção: **`ANS_DDRHitbox`** com **`bAirPop = ✅ TRUE`** (re-flutua o alvo: 150cm × 0.85^hits, cap 7) + **`ANS_DDRComboWindow`** (encadeia Air1→Air2, mesmo padrão do chão).

> O `GA_AirAttack` **herda** toda a máquina do combo de chão (seções/janela/buffer) — e cada golpe **renova o hold aéreo** do player (sem atacar por 1.4s → cai sozinho).

---

## 4. `AM_AirSlam` — o finisher

**Clips:** `Attack_Air_To_Floor_Start_01_Seq` + `Attack_Air_To_Floor_End_01_Seq` (o `_Loop_01` fica de fora no MVP — a descida é rápida; vira seção "Loop" P1 pra quedas altas).

1. Montage **`AM_AirSlam`** com **2 seções**: `Start` e `End`. **Clear!**
2. **`EnableRootMotion = ❌ FALSE`** — a descida real é o código (velocity -3500).
3. **NÃO** precisa de hitbox notify: o **AoE do impacto é o código** (raio 250, dano 25, hit-stop 6 frames, derruba todos os Airborne) — dispara no **pouso** (LandedDelegate → pula pra seção `End` + `GameplayCue.Slam`).

---

## 5. BPs filhos + Startup Abilities

| BP (parent) | Campo | Asset |
|---|---|---|
| `BP_GA_Launcher` (← `GA_Launcher`) | Launcher Montage | `AM_Launcher` |
| `BP_GA_AirAttack` (← `GA_AirAttack`) | Combo Montage | `AM_AirCombo` |
| `BP_GA_AirSlam` (← `GA_AirSlam`) | Slam Montage | `AM_AirSlam` |
| `BP_GA_AttackLight` (já existe) | **Run Attack Montage** | `AM_RunAttack` (§6) |

No `BP_DDRPlayer` → **Startup Abilities** (5 entradas): troque as classes C++ pelos BPs:
`BP_GA_AttackLight`(Attack) · `BP_GA_AirAttack`(**Attack** — mesmo botão!) · `BP_GA_Dash`(Dash) · `BP_GA_Launcher`(Launcher) · `BP_GA_AirSlam`(AirSlam).

> 🔑 **Mesmo botão, ability certa:** `GA_AttackLight` agora tem `Blocked: InAir` e `GA_AirAttack` tem `Required: InAir` — o LMB ativa o certo pelo **estado**, sem `if` no input ([19 §roteamento](../combat/19_Abilities_Deep.md)).

---

## 6. ⚔️ Run-Attack (sua pergunta: SIM, é barato!)

**Clip:** `Run_Attack_01_Seq` (pasta `12_Run_Attact`).

1. Montage **`AM_RunAttack`** (sem seções; `EnableRootMotion = TRUE` — o avanço da estocada é autorado).
2. Notifies: **`ANS_DDRHitbox`** normal (Damage ~14) + **`ANS_DDRComboWindow`** na recovery.
3. Assign no `BP_GA_AttackLight` → **Run Attack Montage** (threshold: `Run Attack Min Speed = 450`).

**Como funciona (código):** apertou ataque **correndo ≥450 u/s** → toca o run-attack como **OPENER**; input na janela dele → **encadeia direto no Atk1** do combo normal. Parado/andando → combo normal direto. Zero botão novo.

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
| Run-attack opener | **RunAttack** | 200 cm | mesma lógica, alcance maior na estocada |
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

Em **cada** montage que avança o corpo (combo chão, run-attack, launcher — **não** nos clips aéreos puros):

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
3. Configure:
   - **Warp Target Name** = `AttackWarp` ← **exato**
   - **Warp Translation** = ✅
   - **Ignore Z Axis** = ✅ (ground/run/launcher — Z vem do RM do clip no launcher)
   - **Warp Rotation** = ✅ (opcional; o código já encara no startup)

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
| **R no ar** | desce em slam → **AoE no impacto** (dano + hit-stop pesado) + dummy **despenca** |
| 7+ hits aéreos | dummy cai (cap do juggle) |
| LMB **correndo** (≥450) | **run-attack** avança; janela → encadeia no combo |
| `ddr.CombatDebug 1` | sweeps + soft-lock (ciano) + warp point (magenta) + AoE slam |
| Acertar dummy ~2m de frente | soft-lock + warp | §7 — sem isso = swing no vazio |

**✅ M2 pronto quando:** lançar → combar → slam **é divertido e você LÊ a altura** (sombra!). Mostre pra alguém sem explicar — se entender "o inimigo subiu", o pilar está provado ([17 §4](../17_Implementation_Roadmap.md)). 🎯

---

## 11. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| **Editor crasha ao abrir** (stack em `GE_DDRCooldownLauncher` / `FindOrAddComponent`) | `FindOrAddComponent` no **construtor** do GE | Recompile — tags vão em `PostInitProperties` ([05 §5](05_GAS_Architecture.md)) |
| Player **não sobe** no launcher | acertou? (whiff = by design) · clip sem `EnableRootMotion` | §2 — TRUE no `Attack_Up_Floor_To_Air` |
| **"Pulinho curto e trava"** (parece root lock) | `MOVE_Walking` descarta o **Z** do root motion | corrigido no código (Flying durante a montage) — **recompile** |
| Player flutua após whiff/cancel do launcher | build antiga (sem restauração de Falling) | recompile — `EndAbility` restaura |
| Alvo não sobe | `bLaunchTargets` desmarcado no notify | §2 passo 3 |
| Alvo cai rápido demais | hold/pop baixos | `TargetAirborneHoldSeconds`/`AirPopHeightBase` no CombatComponent (BP) |
| Player desliza no ar | clips aéreos com RM | §3 — `EnableRootMotion=FALSE` nos air clips |
| LMB no chão não ataca | recompile (Blocked InAir entrou no AttackLight) | rebuild + restart editor |
| Slam não muda pra "End" | nomes das seções (`Start`/`End` exatos) | §4 |
| Slam sem dano | — (AoE é código; confira `ddr.CombatDebug 1`) | raio 250 alcança? |
| Air combo toca as 2 seções de uma vez | seções linkadas | **Clear** ([57 §0](../combat/57_M1_Combo_Editor_Setup.md)) |
| Não leio a altura | sem sombra blob | §7 — P0! |
| Run-attack nunca sai | velocidade < 450 / montage não setada | §6 |
| **Bate no vazio** perto do dummy | sem Motion Warp / warp name errado | §7.3 — target `AttackWarp` |
| Personagem não avança no swing | falta notify Motion Warping na montage | §7.3 |
| Warp "teleporta" demais | `MaxWarpDistance` alto demais | §7.2 — cap 200cm ground |
| Olha pro inimigo mas não alcança | só soft-lock, sem warp | §7 — as 4 camadas |
| **Atk1 alcança, Atk2+ bate no vazio** | warp só na 1ª seção | §7.3 — **replique** o notify em Atk2/3/4 |
| Air combo drifta | RM ligado nos clips aéreos | §3 — `EnableRootMotion=FALSE` |

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
