# 62 — Auditoria de Combate M2 (multi-agente) · 📋 2026-06-12

> **Auditoria profunda do kit de combate** rumo a qualidade AAA, feita por 3 agentes de revisão independentes (cada um leu o C++ atual inteiro + docs canônicas): **①** Launcher + Air Combo (juggle) · **②** Slam (pin/ragdoll/queda) · **③** Combo de chão + Dash (dash-attack). Consolidada e revisada pelo auditor-chefe (correções e reclassificações em §7).
>
> **Como usar:** cada achado tem **status** — marque ✅ ao corrigir e anote a data. Achados com 🔴 ALTA são os candidatos imediatos. Este doc é um *snapshot* (2026-06-12): valide o `arquivo:linha` antes de mexer, o código anda rápido.

---

## 0. Veredito executivo

| Sistema | Nota | Resumo |
|---|:---:|---|
| **Launcher + Air Combo** | 🟡 sólido, 2 bugs ALTA | Máquina aérea correta (tags idempotentes, whiff path, decay/cap, holds duplos). Distância pro AAA: divergência Z player↔alvo no frame, **mono-alvo** (M3 quebra), e 3 das 4 âncoras de cancelamento documentadas não existem (jump-cancel!). |
| **Slam** | 🟡 maduro, feel mudo | Máquina de estados coerente, cancel paths protegidos, ragdoll anti-tunneling acima do padrão indie. **Mas o clímax está mudo**: `Cue_Slam` nunca dispara no fluxo canônico (pin), e a queda pós-End a gravidade pura é lenta pro ritmo roguelike. |
| **Combo chão + Dash** | 🟢 quase lá | Espinha dorsal AAA-correta (buffer, janela, dash-cancel, dash-attack com grace). Falta granularidade por golpe: cancel windows individuais, frame data central (`FDDRAttackData`), branching L/H. |

**Tema geral:** a *arquitetura* está AAA; o que falta é (a) ~6 bugs de borda corrigíveis em horas, (b) o **juice do clímax** (slam mudo), (c) **profundidade de expressão** (jump-cancel) e (d) preparo para **multi-inimigo** antes do M3.

---

## 1. 🐞 Bugs consolidados (por severidade)

### 🔴 ALTA

| ID | Sistema | Onde | Sintoma | Fix sugerido | Status |
|---|---|---|---|---|---|
| **A-02** | Launcher/Air | `DDRCombatComponent::RefreshAirHold` | ~~Hold no hit~~ — **revertido**: `RefreshAirHold` só com `bInAirCombat` (T1). Timer/AirAnchor no hit quebrava juggle+slam | manter guard `!bInAirCombat` return; ver §7 T0/T1 | ↩️ revertido 2026-06-12 |
| **A-04** | Launcher/Air | `LaunchTarget` (`ActiveJuggleTarget = TargetActor` incondicional) | **Multi-alvo**: 2º inimigo lançado descarta o 1º — pop/carry/exit operam só no 2º; o 1º fica `Airborne` órfão. Latente no M2, **garantido no M3** | `ActiveJuggleTargets` (cap 4) + `EndAirborne` no evicted | ✅ 2026-06-12 |
| **A-05** | Slam/pin | `EndSlamAirPin` (bloco final) | `if (MOVE_Flying && !IsFalling())` é quase sempre true em Flying → seta **MOVE_Walking no ar** | trocar por `IsMovingOnGround()` (como `ExitAirCombat` já faz) | ✅ 2026-06-12 |
| **S-03** | Slam | `OnSlamLanded` ignora pouso com pin ativo; `bImpactTriggered` nunca seta no fluxo pinado | **`Cue_Slam` NUNCA dispara no caminho canônico** (BeforeGroundProximity + PinInAir) — o clímax é mudo (confirma `impacto=0` nos logs de PIE) | disparar `Cue_Slam` + `bImpactTriggered=true` + hit-stop pesado em `TryJumpToEndSection` (o momento do "contato declarado") | ✅ 2026-06-12 |
| **S-08** | Slam | `ANS_DDRHitbox::NotifyBegin` chama `BeginSlamAirPin` ANTES do `SnapSlamEndToJuggleTarget` | Com `SlamEndTrigger=OnSlamHit` + hitbox no Start com `bJumpToSlamEndSection`: **pin fixa o Z errado** (antes do snap de co-altitude) | NotifyBegin só configura SweepParams; `BeginSlamAirPin` SÓ dentro de `TryJumpToEndSection`, após o snap | ✅ 2026-06-12 |
| **G-01** | Chão/Dash | `GA_AttackLight::PlayRunAttackOpener` | ~~Condição de warp invertida~~ — **revertido**: dash-attack **não** usa warp (estocada reta estilo Hades; snap anti-natural). Avanço = root motion de `AM_RunAttack` | manter `ClearAttackMotionWarp` no opener | ↩️ revertido 2026-06-12 |

### 🟡 MÉDIA

| ID | Sistema | Onde | Sintoma | Fix | Status |
|---|---|---|---|---|---|
| A-01 | Launcher/Air | `ADDRCharacterBase::Tick` (alvo, PrePhysics) vs `MaintainAirAltitude` (player, PostPhysics) | Alvo interpola Z com dados do frame anterior → **micro-flutuação visível** em -55° | Tick do alvo em TG_PostPhysics, ou re-push do `AirborneTargetZ` após fixar `AirAnchorZ` | ✅ 2026-06-12 |
| A-03 | Locks | `UnlockAirHorizontalInput` não checa `bInAirCombat` | Pin solto no meio de um juggle ativo libera WASD no ar | guard `bInAirCombat \|\| bLaunchedTargetThisSwing` + `Unlock(bForce)` no cleanup | ✅ 2026-06-12 |
| A-06 | Ragdoll | `TickGuidedSlamFall` sem checagem de morte | Alvo que morre na queda guiada "levanta e cai de novo" | early-out se `State.Dead`/`bRagdolled` | ✅ 2026-06-12 |
| S-04 | Slam | `ApplySlamPlayerFollow(FollowToGround)` solta o pin "na mão" | Não zera `bSlamPinSweepParamsSet` → janela de estado inconsistente no path não-canônico | rotear por `ReleaseSlamAirPinForLanding` | ✅ 2026-06-12 |
| S-05 | Ragdoll | `TickGuidedSlamFall` sem timeout | Arena com buraco/void → queda guiada **infinita** (tick eterno, ator some) | timeout 3s ou cap de distância → `FinishGuidedSlamFall` | ✅ 2026-06-12 |
| G-03 | Notifies | `ANS_DDRHitbox::NotifyBegin` sem null-check de `GetOwner()` | Crash potencial em preview/ator destruído | mesmo guard do `ANS_DDRComboWindow` | ✅ já estava |

### 🟢 BAIXA (lista curta)

`A-07` SavedMovementMode frágil (documentar) · `A-08` restauração de orient força true em caso legítimo de false · `S-01` early-return de `EndSlamAirPin` não zera `bSlamPinSweepParamsSet` (code smell; coberto por `UnregisterSlamAbility`) · `S-02` log "homing OFF" enganoso com dist2D≈0 · `S-06` velocity -3500 setada 2× (ExitAirCombat + ability) — remover da 1ª · `S-07` LandedDelegate sempre vinculado (protegido, só custo) · `G-04` `BufferAttackInput` reseta timer em vez de max · `G-05` busca dupla do CombatComponent em InputPressed (cosmético).

---

## 2. 🎮 Gaps para AAA (consolidados, priorizados)

1. **🥇 Jump-cancel (âncora nº 3)** — documentado como pilar ([19 §3], [15 §6]) e ausente. É o que transforma "lança → 4 hits → slam" em sistema de expressão (DMC5/Bayonetta: `golpe → JC → golpe → JC → spike`). Implementação mínima: `GA_Jump` aéreo com `ActivationRequiredTags: InAir` + `CancelAbilitiesWithTag: Ability.Attack.Air` + pop vertical no player. Pode reusar `Double_Jump_Combat_*` (doc 59 §6 já previa).
2. **🥈 O clímax mudo do slam** (= bug S-03 + juice): `Cue_Slam` no momento do contato + **camera shake** (perlin, amp ~6, 0.3s) no `GC_Slam` + hit-stop 6-10 frames cirúrgico no frame do contato. 3 linhas de C++ + 1 asset = o maior salto de feel disponível.
3. **🥉 Queda pós-End lenta**: `PostSlamFallVelocity` default **-350** no C++ (`GA_AirSlam`) — tune no `BP_GA_AirSlam` se precisar.
4. **Dano escalando no juggle**: hit 1 = hit 7 hoje. Multiplicador por `AirborneHitCount` (a infra `SetByCaller` + `ComboSectionIndex` já existe) incentiva manter o juggle vivo.
5. **`ddr.JuggleDebug` (doc 16 §3 promete, não existe)**: HUD com altura do alvo, pop atual, decay, hit count, timers de hold — sem isso o tuning do feel é cego.
6. **Feedback de "vai cair"**: holds de 1.4s/1.1s correm silenciosos — alvo deveria "afundar" levemente/piscar perto de expirar (DMC faz; vira leitura, não relógio interno).
7. **Cancel windows por golpe** (`ANS_CancelWindow`, doc 18 §4.2): hoje dash-cancel é "sempre" — sem janelas por-golpe não existe "golpe comprometido" = teto de skill baixo.
8. **Frame data central** (`FDDRAttackData`/DataAsset, doc 18 §1): dano/hit-stop/raio hoje vivem espalhados nos notifies — Ecos e balance vão sofrer.
9. **Perfect-dodge/witch-time** (doc 19 ficha do Dash): i-frame existe (GE 0.25s), a sub-janela "perfect" + time dilation não.
10. **Variações**: re-launch aéreo com leitura própria (nudge de 75cm é sub-perceptível), slam carregado, finisher alternativo — P1, a arquitetura suporta.

---

## 3. 🧨 Riscos para o M3 (multi-inimigo)

| Risco | Detalhe | Ação antes do M3 |
|---|---|---|
| **Mono-alvo no juggle** (= bug A-04) | `ActiveJuggleTarget` único; 2º lançado orfana o 1º | refatorar p/ lista — **o bloqueio real do M3** |
| Soft-lock O(n) | `TActorIterator<ADDRCharacterBase>` por seção de combo | aceitável até ~15; medir antes de otimizar |
| Sweep do pin O(n×frame) | `MaintainSlamAirPin` varre todo frame na End | `HitActorsThisSwing` já deduplica; ok no M3, revisar com hordas |
| `SanitizeGroundLocomotionAfterAir` em todo CombatComponent | 20 inimigos × casts/frame | early-out por flag "esteve no ar" |
| Terreno desnivelado | `AirAnchorZ` absoluto; queda guiada sem chão (S-05) | timeout + teste em arena com rampa |

---

## 4. 🧹 Simplificação (dívida do slam)

- `bSlamClaimedTargetOnActivate` + `SlamGrabHitStopFrames`: **removidos** ✅.
- `SlamDamage/Radius/VerticalReach/HitStopFrames` "(doc only)": dead weight de runtime — virar comentário/preset.
- `FollowToGround`: não-canônico e com bug S-04 — deprecar ou marcar experimental; **caminho canônico = `BeforeGroundProximity` + `PinInAir`** (documentar como o único garantido — S-08 mostra por quê).
- `SlamFollowFallVelocity` lido num path onde o EditCondition o esconde — renomear p/ `SlamResumeVelocityZ` com condição correta.

---

## 5. 📋 Drift código↔doc (consolidado)

| Doc | Diz | Código faz | Ação |
|---|---|---|---|
| 16 §2/§11, 19 §8 | alvo/player por **RootMotionSource**, `GravityScale~0` no alvo | player = RM da montage + Flying; alvo = `FInterpTo` no Tick | drift consciente — **atualizar doc** p/ refletir M2 real (já parcialmente feito; completar §11) |
| 19 §3/§8 | **jump-cancel âncora nº 3** | não existe | implementar (gap #1) |
| 16 §3 | `ddr.JuggleDebug 1` | não existe | implementar (gap #5) |
| 19 ficha Launcher | `InAir` como Owned tag | é loose tag via `SetLooseGameplayTagCount` | corrigir ficha |
| 60 §4 | proximity "~250 cm" | default C++ = 200 | alinhar |
| 60 §4 item 5 | "Pouso: só Cue_Slam" | Cue_Slam nunca dispara no fluxo pinado (S-03) | corrigir código |
| 18 §1/§4.2 | `FDDRAttackData` / `ANS_CancelWindow` | não existem | gaps #7/#8 (P1) |
| 18 §2 | anti-tunneling 3-5 amostras no sweep | 1 sweep/frame | P1 (lâminas rápidas) |

---

## 6. 🎯 Plano de ação recomendado (ordem de ROI)

1. **Pacote de fixes ALTA** (~meio dia): S-03 · A-05 · S-08 · G-03. ~~A-02 (hold no hit)~~ e ~~G-01 (warp opener)~~ **revertidos** — ver §7.
2. **Juice do slam** (gap #2/#3): shake no GC_Slam + `PostSlamFallVelocity=-350` + hit-stop cirúrgico.
3. **A-04 multi-alvo** (lista de juggle targets) — **antes** de escrever a IA do M3.
4. **Jump-cancel** (gap #1) + `ddr.JuggleDebug` (#5) — o salto de profundidade.
5. MÉDIAs restantes + limpeza do slam (§4) + drift de docs (§5).

---

## 7. Contrato de timing (C++ — não violar)

> **Fonte canônica no código:** bloco `TIMELINE CANÔNICA` em `DDRCombatComponent.h` + comentários `AUDITORIA` / `MARCO T1|T3` em `LaunchTarget`, `GA_Launcher`, `GA_AirSlam`, `ANS_DDRHitbox`, `GA_AttackLight`.

| Marco | Quando | O que faz | **Não fazer** |
|-------|--------|-----------|----------------|
| **T0** | Hit do launcher (`LaunchTarget`) | `StartAirborne`, tag `InAir`, lock input | `RefreshAirHold`, `AirAnchorZ`, pin de Z no tick, `ExitAirCombat` |
| **T1** | Fim da montage (`OnMontageCompleted`) | `EnterAirCombat`, timer do player | Antecipar T1 para o hit |
| **T2** | Cada air attack | `RefreshAirHold` | Timer no T0 |
| **T3** | `GA_AirSlam::Activate` | `ExitAirCombat(bSlam=1)`, velocity queda, pin só em `TryJumpToEndSection` | `BeginSlamAirPin` no `ANS_DDRHitbox::NotifyBegin` |
| **Dash-attack** | Opener durante/grace do dash | `ClearAttackMotionWarp` — estocada reta (Hades) | `FaceAndSetupMotionWarp(RunAttack)` |

Regressões já vistas ao violar isto: **A-02** (timer no hit) · **G-01** (warp no dash-attack) · **S-08** (pin no notify errado).

---

## 8. Notas do auditor-chefe (revisões dos relatórios)

- **Hit-stop "talvez não congele" (relatório ③, rec. 3): VERIFICADO — falso alarme.** `UDDRHitStopSubsystem` usa `SetGlobalTimeDilation(0.0001)` com restore por **tempo real** (`FPlatformTime`) — é freeze global correto. O que falta é o *timing* cirúrgico no slam (S-03), não o mecanismo.
- **Filtro de facção e Poise**: relatórios anteriores apontavam ausência — **já implementados** na build atual (`SharesFactionWithOwner`, `GE_DDRPoiseDamage`, `CanLaunchActor`, `TickPoiseRegen`, gate de distância no launcher). O doc 61 §1 foi atualizado.
- S-01 foi **rebaixado para BAIXA** pelo próprio agente após revisão (coberto por `UnregisterSlamAbility`); G-06 do relatório ③ era falso positivo (descartado).
- Os 3 relatórios completos estão no histórico da sessão de 2026-06-12; este doc é a síntese acionável.
