# 17 — Roadmap de Implementação

> A **ordem real** de construir o Dark Dungeon Rift, fatiada em milestones que terminam **jogáveis**. Cada fase tem critério de aceite ("pronto quando..."). MVP-first, jogabilidade primeiro.

> Princípio que guia tudo: **chegue ao combo aéreo (M2) o mais rápido possível.** É lá que você descobre se o jogo é divertido. Não perfeccione nada antes disso.

---

> 🔬 **Atualizado pela [Revisão de Design 2026-06](design/Design_Review_2026-06.md)** (mesa-redonda de 4 especialistas). Mudança principal: foi adicionada a fase **M⁻¹ (spike de validação)** antes do M0 — consenso unânime de que o pilar (combo aéreo) precisa ser validado *antes* de construir a infraestrutura.

## 1. Visão dos milestones

```
M⁻¹ SPIKE ─→ M0 Esqueleto ─→ M1 Bater ─→ M2 VOAR ─→ M3 Lutar ─→ M4 Run ─→ M5 Loop
 valida o     movimento       combo        combo       IA+arena   salas+    morte→meta
 PILAR ⚡      + câmera        de chão      AÉREO ★     ondas      recompensa →polish
 (descartável)                             (o pilar)
 └─ derisk ─┘ └──────────── prova técnica ──────────┘└────────── prova de jogo ──────────┘
```

★ **M2 é o momento da verdade.** Se o combo aéreo for divertido e legível, o projeto tem alma. **Mas o M⁻¹ derisca o M2** — você não quer descobrir no M2 (com toda a infra construída) que o pilar não funciona. Valide a alma antes de construir o corpo.

---

## 1.5. ⚡ M⁻¹ — Spike de Validação (FAÇA ANTES DO M0)

**Meta:** num protótipo **descartável**, sobre o template Third Person atual, **sem GAS, sem arte, sem roadmap**, responder a pergunta existencial do projeto: *o combo aéreo em topdown é divertido E legível?*

> 🧠 **Por que isto vem ANTES do M0** (consenso Davi + Sofia, apoiado por Kael): o plano inteiro repousa numa suposição **não testada** de fun/legibilidade. Docs lindos não são evidência. Uma tarde de spike vale mais que uma semana de infra construída sobre fé.

| Tarefa | Responde a... |
|---|---|
| 1 botão → `RootMotionSource` leva um **cubo** (alvo) à altura X e o segura | viabilidade técnica do hang |
| Player sobe junto via `RootMotionSource` (mesma altura) | **co-altitude** sem `LaunchCharacter` (Davi, risco ALTA) |
| **Sombra de decal** no chão sob player e cubo | leitura de altura em topdown |
| 1 "hit" que dá pop + decay no cubo | o juggle *segura*? (modelo numérico, doc 16 §3) |
| Testar **follow launch total vs parcial vs só-o-alvo-sobe** | a "pergunta de um milhão" (doc 16 §6) |
| **Launcher tosco de verdade** (`IA_Launcher`, não cubo mudo) → dispara o pop | provar *fun* do pilar, não só co-altitude (Kael) |

> 🥊 **Atualização (Review de Combate):** o **launcher entrou no spike**. Cubos provam a co-altitude *técnica*; só um **launcher real + air-hit + sombra** prova o *fun*. Falta asset? Sourcing de 1 uppercut tosco ([22 §4](22_Animation_Inventory.md)). Origem: [Review de Combate](design/Design_Review_Combat_2026-06.md).

**✅ Pronto / Go-No-Go quando:** você joga, mostra pra **3 pessoas sem explicar nada** e pergunta *"o que aconteceu?"*. Se elas entendem a altura e acham **empolgante** → 🟢 siga pro M0 com confiança. Se acham confuso ou sem graça → 🔴 **pare e itere o conceito do pilar** (ou reconsidere follow launch / câmera) antes de investir meses. Spike descartado depois — o aprendizado fica.

> 🗑️ **É descartável de propósito.** Não tente "aproveitar o código". O valor é o *aprendizado*, não os arquivos. Jogue fora e construa o M0 limpo.

---

## 2. M0 — Esqueleto (mover numa sala) · 🟢

**Meta:** personagem topdown que anda/sprinta/dasha/pula numa sala vazia, com câmera boa. Sem combate ainda.

| Tarefa | Doc |
|---|---|
| Habilitar plugins P0 + Build.cs + reorganizar pastas + renomear classe → `DDR` | [04](systems/04_Project_Setup.md) |
| Reconfigurar câmera p/ topdown (pitch -55°, yaw fixo, orient-to-movement) | [06](systems/06_Camera_TopDown.md) |
| Enhanced Input: Move (relativo à câmera) + Sprint + Dash + Jump | [07](systems/07_Input.md) |
| CMC: gaits (Run base + Sprint) + pulo (gravidade assimétrica) | [09](locomotion/09_Gaits.md), [13](locomotion/13_Jump_Fall_Landing.md) |
| AnimInstance + `FDDRLocomotionState` + blendspace 2D (Direction×Speed) | [08](locomotion/08_Locomotion_Overview.md), [09](locomotion/09_Gaits.md) |
| State machine de Air (jump/fall/land básico) | [13](locomotion/13_Jump_Fall_Landing.md) |
| Console debug `ddr.LocomotionDebug` | [08](locomotion/08_Locomotion_Overview.md) |

**✅ Pronto quando:** dá pra correr, sprintar, dashar e pular numa arena e **sente responsivo e gostoso de mover**. (Se mover já não é gostoso, conserte antes de seguir.)

---

## 3. M1 — Bater (combo de chão) · 🟢

**Meta:** combo melee de 3-4 golpes num boneco de treino, com hit-stop. GAS ligado.

| Tarefa | Doc |
|---|---|
| GAS: ASC + AttributeSet (HP/AttackPower) + InitAbilityActorInfo | [05](systems/05_GAS_Architecture.md) |
| Input → GAS por InputID + **input buffer** de combo | [07](systems/07_Input.md) |
| `GA_Attack_Light` + `AM_Combo` (seções) + janelas (notifies) | [15](combat/15_Combat_Overview.md) |
| Hit detection (trace na arma) → GE_Damage | [15](combat/15_Combat_Overview.md) |
| **Hit-stop** + screen shake + hit VFX/SFX (GameplayCue) | [15](combat/15_Combat_Overview.md), [06](systems/06_Camera_TopDown.md) |
| `GA_Dash` (cancela ataque; **única esquiva** — Dodge→Dash, **Roll cortado**) | [07](systems/07_Input.md), [19](combat/19_Abilities_Deep.md) |
| **Gramática de cancelamento** (tabela estado×ação×janela) + `ddr.CombatDebug` — *P0, não "depois"* | [15 §6](combat/15_Combat_Overview.md) |
| Hit-stop como **freeze global** (congela CMC, não só montage) | [15 §5](combat/15_Combat_Overview.md) |
| **Perfect-dodge** (Rha): i-frame 2 camadas (generoso + sub-janela *perfect*) → witch-time | [19](combat/19_Abilities_Deep.md), [20 §4.1](feel/20_Game_Feel.md), [21](feel/21_Juice_FX.md) |
| **Player flinch/hitreact** ao tomar hit — pune ganância (usa clips de Hit, doc 22) | [18](combat/18_Combat_System_Deep.md) |
| Boneco de treino com ASC (recebe dano) | [05](systems/05_GAS_Architecture.md) |

**✅ Pronto quando:** dá pra encadear um combo de 3 golpes num boneco, **sente peso** (hit-stop global), o **dash-cancel é universal e instantâneo** (critério de aceite — testado frame-a-frame com `ddr.CombatDebug`, Nø), os **3 cancelamentos-âncora** funcionam, e o **perfect-dodge** dá witch-time no timing certo. O combate de chão é gostoso *e expressivo*.

---

## 4. M2 — VOAR (combo aéreo) · 🟢 ★ MOMENTO DA VERDADE

**Meta:** launcher + combo aéreo + slam num inimigo, **legível em topdown**.

| Tarefa | Doc |
|---|---|
| **Sombra no chão** (player + inimigo) — leitura aérea | [06](systems/06_Camera_TopDown.md), [16](combat/16_Aerial_Combos.md) |
| `GA_Launcher`: sobe alvo **e** player (follow launch) + gravidade baixa no alvo | [16](combat/16_Aerial_Combos.md) |
| Tags `State.Combat.InAir` / `Airborne`; AnimBP usa SM aérea via tag | [05](systems/05_GAS_Architecture.md), [08](locomotion/08_Locomotion_Overview.md) |
| `GA_AirAttack` (requer InAir) + re-float do alvo + encadeamento (reusa M1) | [16](combat/16_Aerial_Combos.md) |
| `GA_AirSlam`: desce + **Hard Land** (AoE + shake) | [16](combat/16_Aerial_Combos.md), [13](locomotion/13_Jump_Fall_Landing.md) |
| Juggle decay (anti-infinito) | [16](combat/16_Aerial_Combos.md) |

**✅ Pronto quando:** você lança um inimigo, comba ele no ar, e fecha com slam — **e um amigo que nunca viu o jogo entende a altura** sem você explicar. **Se isso for divertido, o jogo está provado.** 🎯

> 🛑 **Checkpoint de decisão:** se o M2 *não* ficou divertido, **pare e itere no combate** antes de construir run/IA/loop. Não adianta ter 50 salas de um combate medíocre. O fun do M2 é pré-condição pra todo o resto.

---

## 5. M3 — Lutar (IA + arena) · 🟡

**Meta:** 2 tipos de inimigo com IA básica numa arena com ondas.

> ⚠️ **Orce a IA de verdade (revisão de design — Sofia).** Inimigo que **persegue + telegrafa + recua + pausa no `Airborne`** é mais trabalho do que uma linha de tabela sugere — é pré-requisito do "jogo de verdade". Agora documentado no cluster **[Inimigos & IA (30-34)](enemies/30_AI_Overview.md)**. Reserve tempo de M3.

| Tarefa | Doc |
|---|---|
| Inimigo melee (grunt) — perseguir/telegrafar/recuar (GAS) | [30](enemies/30_AI_Overview.md), [31](enemies/31_Enemy_Archetypes.md) |
| Inimigo ranged (kite + projétil telegrafado) | [31](enemies/31_Enemy_Archetypes.md), [32](enemies/32_Enemy_Combat_Behavior.md), [37](combat/37_Projectiles.md) |
| IA **pausa em `State.Combat.Airborne`** (deixa ser combada) | [30 §6](enemies/30_AI_Overview.md), [16](combat/16_Aerial_Combos.md) |
| **Token de ataque** (1-2 atacam por vez — anti-overwhelm) | [32 §5](enemies/32_Enemy_Combat_Behavior.md), [33 §5](enemies/33_Spawning_Encounters.md) |
| Arena: portas trancam → ondas de spawn → abre ao limpar | [03](design/03_Core_Loop_Roguelike.md) |
| **Telegrafe** legível em topdown (decal no chão) | [32 §1](enemies/32_Enemy_Combat_Behavior.md) |

**✅ Pronto quando:** dá pra limpar uma arena de 4-8 inimigos usando combos de chão e aéreo, com algum risco real. O combate "ao vivo" (não em boneco) é gostoso.

---

## 6. M4 — Run (salas encadeadas) · 🟡

**Meta:** 3-5 salas + mini-boss + recompensa entre salas.

| Tarefa | Doc |
|---|---|
| `DDRRunManager` / `DDRRoomManager`: sequência de salas | [03](design/03_Core_Loop_Roguelike.md) |
| 6-8 arenas à mão, sorteia 4 por run (procedural-sem-procedural) | [03](design/03_Core_Loop_Roguelike.md) |
| Recompensa entre salas: escolher 1 de 2-3 (GE/ability) | [03](design/03_Core_Loop_Roguelike.md), [05](systems/05_GAS_Architecture.md) |
| Mini-boss (2 fases, ataques-assinatura, janela de launch) | [34](enemies/34_MiniBoss.md) |
| HUD funcional (HP, recurso) | — |

**✅ Pronto quando:** dá pra jogar uma run completa (entrar → 4 salas → boss → fim), escolhendo upgrades que mudam a build. Tem progressão dentro da run.

---

## 7. M5 — Loop (morte → meta) · 🟡

**Meta:** o loop roguelike fecha — morrer dá progresso, nova run começa mais forte.

| Tarefa | Doc |
|---|---|
| Morte (HP≤0 → `State.Dead` → fim de run) | [05](systems/05_GAS_Architecture.md) |
| Recurso meta persistente (ex.: Essência) | [03](design/03_Core_Loop_Roguelike.md) |
| Hub: gastar Essência em 3-5 upgrades permanentes leves | [03](design/03_Core_Loop_Roguelike.md) |
| Save mínimo (só o meta) | [02](design/02_MVP_Scope.md) |
| **Tuning de feel** (números, juice, ritmo) — passe final | todos |

**✅ Pronto quando:** morre → "ficou X mais forte" → quer jogar de novo. O MVP está **completo e jogável de ponta a ponta**.

---

## 8. Depois do MVP (P1/P2 — não antes!)

Só toque aqui depois do M5 jogável:

| Item | Doc | Por quê é depois |
|---|---|---|
| Start/Stop transitions + Distance Match | [10](locomotion/10_Start_Stop_DistanceMatch.md) | Polish que topdown esconde |
| Stride Warping | [11](locomotion/11_Warping.md) | Anti-skating invisível de cima |
| Height landing refinado + predictive | [13](locomotion/13_Jump_Fall_Landing.md) | Já tem o básico; refino é P1 |
| Motion warp no ataque | [15](combat/15_Combat_Overview.md) | Melhora feel, não é bloqueante |
| Foot IK (se houver rampas) | [14](locomotion/14_Foot_Leg_IK.md) | Só com desnível |
| Crouch | [09](locomotion/09_Gaits.md) | Pouco uso em ARPG |
| **Parry** (🟡 P1 — *primeiro* pós-core; ofensivo: parry→stagger→launch) | [22 §5](22_Animation_Inventory.md), [19](combat/19_Abilities_Deep.md) | Barato (janela+tag, reusa poise). Review de Combate |
| **Dodge-offset** (esquiva no meio do combo sem perdê-lo) | [18](combat/18_Combat_System_Deep.md) | P1 — janela defensiva que preserva o pilar (Rha) |
| Roll (2ª esquiva), Charge, Buff, Execution, Block | [22](22_Animation_Inventory.md) | Assets existem; camada de combate extra pós-MVP |
| Motion Matching (spike) | [08](locomotion/08_Locomotion_Overview.md) | Exploração, não MVP |
| Procedural de verdade, +armas, +bosses, arte final, áudio | [02](design/02_MVP_Scope.md) | Expansão pós-prova |

---

## 9. Regras de ouro do roadmap

1. **Cada milestone termina jogável.** Nada de "fase só de backend".
2. **Chegue ao M2 rápido.** É o teste de fun. Não perfeccione antes.
3. **Se o M2 não diverte, itere o combate** — não construa em cima de base medíocre.
4. **Locomoção é suporte, combate é protagonista.** Aloque tempo conforme [doc 02 §5](design/02_MVP_Scope.md).
5. **Em dúvida entre polir e avançar:** avance até o loop fechar (M5), depois polia.
6. **Topdown esconde detalhe de pés** — invista o orçamento de anim em leitura de combate.

---

## 10. Tabela mestra: recurso → milestone → doc

| Recurso pedido | Milestone | Doc | Prioridade |
|---|---|---|---|
| Multiple Gaits | M0 | [09](locomotion/09_Gaits.md) | 🟢 P0 |
| Sprinting & Crouching | M0 / cortado | [09](locomotion/09_Gaits.md) | 🟢 / 🔵 |
| Clean Extendable Logic | M0 | [08](locomotion/08_Locomotion_Overview.md) | 🟢 P0 |
| Jumping | M0 | [13](locomotion/13_Jump_Fall_Landing.md) | 🟢 P0 |
| Gait-Selected Jumping | (de graça/P2) | [13](locomotion/13_Jump_Fall_Landing.md) | 🔵 P2 |
| Height-Based Fall to Landing | M2 (slam) | [13](locomotion/13_Jump_Fall_Landing.md) | 🟡 P1 |
| Start & Stop Transitions | pós-MVP | [10](locomotion/10_Start_Stop_DistanceMatch.md) | 🟡 P1 |
| Distance Match Stops | pós-MVP | [10](locomotion/10_Start_Stop_DistanceMatch.md) | 🟡 P1 |
| Stride Warping | pós-MVP | [11](locomotion/11_Warping.md) | 🟡/🔵 |
| Foot & Leg IK | se houver rampa | [14](locomotion/14_Foot_Leg_IK.md) | 🔵 P2 |
| Gait Switch Transitions | de graça (blendspace) | [09](locomotion/09_Gaits.md) | 🔵 P2 |
| **Combo de chão** | **M1** | [15](combat/15_Combat_Overview.md) | 🟢 P0 |
| **Combo aéreo** ★ | **M2** | [16](combat/16_Aerial_Combos.md) | 🟢 P0 |
| Roguelike loop | M4-M5 | [03](design/03_Core_Loop_Roguelike.md) | 🟢 P0 |

---

## 11. Comece agora

→ **M0, tarefa 1:** [04 — Setup do Projeto](systems/04_Project_Setup.md). Habilite os plugins, arrume o Build.cs, renomeie a classe. Depois siga a tabela do §2.

Bom desenvolvimento. 🗡️
