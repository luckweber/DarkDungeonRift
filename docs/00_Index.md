# Dark Dungeon Rift — Documentação do Projeto

> **Gênero:** ARPG **topdown** · **Roguelike** · **Hack'n'Slash** com **combos aéreos**.
> **Engine:** Unreal Engine **5.4** · **GAS** (Gameplay Ability System).
> **Foco desta doc:** **jogabilidade primeiro** — o MVP precisa ser *jogável e gostoso*, não bonito. Tudo aqui é priorizado por esse princípio.

Este é o índice mestre. Cada doc é autocontido e segue o mesmo formato: **o que é → por que importa no MVP → como montar no UE 5.4 → checklist → troubleshooting**.

---

## Como ler (ordem recomendada)

1. **Visão & escopo** — entenda o jogo e o que entra no MVP antes de codar.
2. **Sistemas base** — projeto, GAS, câmera, input. É o esqueleto.
3. **Locomoção** — os 11 recursos (Turn In Place removido), priorizados para topdown.
4. **Combate** — o coração do hack'n'slash + combos aéreos.
5. **Roadmap** — a ordem real de implementação, fatiada em milestones jogáveis.

---

## Legenda de prioridade (MVP topdown)

| Tag | Significado | Regra |
|---|---|---|
| 🟢 **P0** | Núcleo do MVP. Sem isso não há "jogo". | Faça primeiro, faça funcionar. |
| 🟡 **P1** | Polish do MVP. Faz o jogo *sentir* bom. | Só depois do P0 jogável de ponta a ponta. |
| 🔵 **P2** | Pós-MVP. Em **topdown** a câmera esconde boa parte disto. | Não gaste tempo agora. |

> ⚠️ **Princípio topdown:** a câmera fica longe e de cima. Detalhe de pé, transição de passada e turn-in-place **quase não aparecem**. Invista o orçamento de animação em **leitura de combate** (telegrafe de ataque, hit-stop, impacto) — é isso que o jogador vê.

---

## Mapa de documentos

### 🎯 Design & Escopo
| # | Doc | Conteúdo |
|---|---|---|
| 01 | [Visão do Jogo](design/01_Game_Vision.md) | Pilares, fantasia, referências (Hades, DMD, Diablo 4, POE2, DMC), identidade |
| 02 | [Escopo do MVP](design/02_MVP_Scope.md) | O que entra/fica de fora, milestones (incl. **M⁻¹ spike**), definição de "pronto" |
| 03 | [Core Loop Roguelike](design/03_Core_Loop_Roguelike.md) | Estrutura da run, salas, recompensas, meta-progressão |
| 03b | [Sistema de Recompensa / Ecos](design/03b_Reward_System.md) 🆕 | Pool, famílias, sinergias, raridade — *"Eco transforma o combo"* |
| 40 | [Catálogo de Ecos (conteúdo)](design/40_Eco_Pool_Catalog.md) 🆕 | **18 Ecos** MVP: nome, GAS, raridade, sinergias → `UDDREcoData` |
| 45 | [Mundo & Lore](design/45_World_Lore.md) 🆕 | A **Fenda**, Ecos = ecos dos caídos, o Limiar, morte diegética, **combo aéreo diegético** |
| 46 | [Profundidade & Replay](design/46_Depth_Replayability.md) 🆕 | **Auditoria da run** + mecânicas que viciam: tipos de sala, preview de porta, **Pacto/Heat**, drip narrativo |
| 47 | [Tipos de Sala & Rota](design/47_Room_Types_Routing.md) 🆕 | Variedade de salas (elite/tesouro/loja/santuário) + **preview de porta** (rota com escolha) |
| 48 | [Pacto / Heat](design/48_Pact_Heat.md) 🆕 | Dificuldade customizada (Calor → recompensa/desbloqueio) — **o rei da retenção** |
| 49 | [Codex & Vozes do Limiar](design/49_Codex_Limiar.md) 🆕 | Drip narrativo: a Voz da Fenda, codex dos caídos, reações de NPC, eventos |
| 53 | [Economia In-Run / Loja](design/53_In_Run_Economy_Shop.md) 🆕 | Fragmentos (ouro da run), `DT_ShopInventory`, sala Loja — mecânica C do [46](design/46_Depth_Replayability.md) |
| — | [**Revisão de Design 2026-06**](design/Design_Review_2026-06.md) | Mesa-redonda macro (4 especialistas) → veredito que atualizou o plano |
| — | [**Revisão de Combate 2026-06**](design/Design_Review_Combat_2026-06.md) 🆕 | Roundtable combate/feel/juice → perfect-dodge (M1), parry P1, 1 esquiva, launcher no spike |
| — | [**Revisão de Corpus 2026-06**](design/Design_Review_Corpus_2026-06.md) 🆕 | Auditoria dos 53 docs (4 revisores) → consistência aplicada: física do juggle, atributos GAS, cap=7, golpe Heavy, etc. |

### 🧱 Sistemas Base
| # | Doc | Prioridade | Conteúdo |
|---|---|---|---|
| 04 | [Setup do Projeto](systems/04_Project_Setup.md) | 🟢 P0 | Plugins, módulos, `Build.cs`, organização de pastas |
| 05 | [Arquitetura GAS](systems/05_GAS_Architecture.md) | 🟢 P0 | ASC, Attributes, Abilities, Effects, Gameplay Tags |
| 06 | [Câmera Topdown](systems/06_Camera_TopDown.md) | 🟢 P0 | Spring arm, framing, zoom, foco em alvo |
| 07 | [Input (Enhanced Input)](systems/07_Input.md) | 🟢 P0 | Mapeamento, buffer de input, ativação de habilidade |
| 39 | [Controles (Teclado & Gamepad)](systems/39_Controls.md) | 🟢 P0 | **Mapa canônico** de teclas/botões: combate, UI, gamepad, símbolos do HUD |
| 41 | [Gameplay Tags (catálogo)](systems/41_Gameplay_Tags.md) 🆕 | 🟢 P0 | Referência única: `Ability.*`, `State.*`, `Eco.*`, `Event.*`, slots GAS |
| 42 | [Run & Room Manager](systems/42_Run_Room_Manager.md) 🆕 | 🟢 P0 | `DDRRunManager`, fluxo sala→onda→Eco, `DT_RunLayout` |
| 44 | [Arquitetura Data-Driven](systems/44_Data_Driven.md) 🆕 | 🟢 P0 | **DataTables vs DataAssets**, catálogo de dados, pastas, workflow designer |

### 🏃 Locomoção (os 11 recursos)
| # | Doc | Recursos cobertos | Prioridade topdown |
|---|---|---|---|
| 08 | [Visão Geral da Locomoção](locomotion/08_Locomotion_Overview.md) | **Clean Extendable Logic** + arquitetura + decisão GASP vs bespoke | 🟢 P0 |
| 09 | [Gaits](locomotion/09_Gaits.md) | **Multiple Gaits** · **Sprinting & Crouching** · **Gait Switch Transition States** | 🟢 P0 (sprint) / 🔵 P2 (crouch) |
| 10 | [Start/Stop & Distance Match](locomotion/10_Start_Stop_DistanceMatch.md) | **Start & Stop Transitions** · **Distance Match Stops** | 🟡 P1 |
| 11 | [Warping de Passada](locomotion/11_Warping.md) | **Stride Warping** (+ Orientation/Slope) | 🟡 P1 / 🔵 P2 |
| 13 | [Jump / Fall / Landing](locomotion/13_Jump_Fall_Landing.md) | **Jumping** · **Gait-Selected Jumping** · **Height-Based Fall to Landing** | 🟢 P0 (base p/ combo aéreo) |
| 14 | [Foot & Leg IK](locomotion/14_Foot_Leg_IK.md) | **Foot & Leg IK** | 🔵 P2 (só vale em rampas/escadas) |

### ⚔️ Combate (Hack'n'Slash)
| # | Doc | Prioridade | Conteúdo |
|---|---|---|---|
| 15 | [Visão Geral do Combate](combat/15_Combat_Overview.md) | 🟢 P0 | Sistema de combo melee data-driven via GAS, hit detection, hit-stop |
| 16 | [Combos Aéreos](combat/16_Aerial_Combos.md) | 🟢 P0 | Launcher, juggle, gravidade em ar, slam, finisher |
| 50 | [Armas & Arsenal](combat/50_Weapons_Arsenal.md) 🆕 | **Arma = identidade:** habilidades + **locomoção (Linked Anim Layers)** + ataques próprios; 1 arma no MVP, arsenal pós-MVP |
| 51 | [Combate Defensivo](combat/51_Defensive_Combat.md) 🆕 | Perfect-dodge (M1), parry ofensivo (P1), dodge-offset — consolida Review de Combate |

### 🔬 Deep Dives — Combate · Habilidades · Feel · Juice 🆕
> Aprofundamentos escritos pela *writers' room* (4 agentes-autores). Leia após os overviews (15/16). Jogo **single-player** (local-authoritative) → combate agressivo, slow-mo livre.

| # | Doc | Prioridade | Conteúdo |
|---|---|---|---|
| 18 | [Combate Profundo](combat/18_Combat_System_Deep.md) | 🟢 P0 | **As REGRAS:** `FDDRAttackData` + frame data, hit detection, fórmula de dano, máquina de combo/cancelamento, poise/stagger, soft-lock |
| 19 | [Habilidades (GAS) Profundo](combat/19_Abilities_Deep.md) | 🟢 P0 | **O roster:** fichas das abilities, base `UDDRGameplayAbility`, taxonomia de tags, custo/cooldown, runtime granting de Ecos |
| 20 | [Game Feel](feel/20_Game_Feel.md) | 🟢 P0 | **O TATO** (você→jogo): responsividade, peso, feel de dash/pulo/sprint/aéreo, cancelamento sentido, tabela-mestra de knobs |
| 21 | [Juice & FX](feel/21_Juice_FX.md) | 🟢 P0 | **O ECO** (jogo→você): hit-stop, shake, slow-mo, VFX/SFX, catálogo de GameplayCues, escala de juice por golpe |

### 🗺️ Execução
| # | Doc | Conteúdo |
|---|---|---|
| 17 | [Roadmap de Implementação](17_Implementation_Roadmap.md) | Plano em fases, MVP-first, com critérios de aceite por milestone |
| 43 | [Spike M⁻¹ (playbook)](43_Spike_Minus1.md) 🆕 | Protótipo descartável: cubos, co-altitude, `DT_SpikeTuning`, go/no-go |

### 🌍 World & Level Design 🆕
| # | Doc | Conteúdo |
|---|---|---|
| 52 | [Arena & Level Design](world/52_Arena_Level_Design.md) 🆕 | Playbook: 6 arenas + boss, spawn points, `DT_RunLayout`, NavMesh, streaming |

### 🎬 Produção & Assets 🆕
| # | Doc | Conteúdo |
|---|---|---|
| 22 | [Inventário de Animação](22_Animation_Inventory.md) | Mapa do set (~334 clips) → sistemas → prioridade MVP; **gaps** (launcher!) e **over-scope** (parry/charge/buff/execution/block) |

### 🖥️ UI & Menus 🆕
> **UI funcional, não bonita** no MVP ([02 §2](design/02_MVP_Scope.md)). As mais importantes são as que o jogador usa *durante o fun*: **HUD** (28) e **Seleção de Eco** (29) — não os menus de borda.

| # | Doc | Prioridade | Conteúdo |
|---|---|---|---|
| 23 | [UI: Visão Geral](ui/23_UI_Overview.md) | 🟢 P0 | Arquitetura: UMG/CommonUI, camadas, input modes, navegação gamepad, UI Manager |
| 24 | [Main Menu](ui/24_MainMenu.md) | 🟡 P1 | Layout, botões, fluxo (Nova Run / Continuar / Ajustes / Sair) |
| 25 | [Save/Load & Slots](ui/25_Save_Load_Slots.md) | 🟡 P1 | O que persiste (meta vs run), `USaveGame`+versão, autosave, slots |
| 26 | [Loading](ui/26_Loading_System.md) | 🟡 P1 | Tela + load assíncrono, streaming entre salas, preload anti-hitch |
| 27 | [Settings](ui/27_Settings.md) | 🟡 P1 | Vídeo/áudio/controles/**acessibilidade** (liga `ddr.ShakeScale`/`HitStopScale`) |
| 28 | [HUD + Diagrama](ui/28_HUD.md) | 🟢 P0 | HP/abilities/Ecos/combo; **diagrama de posição** (centro = zona sagrada) |
| 29 | [Run-Flow Menus](ui/29_Run_Flow_Menus.md) | 🟢/🟡 | Pause, **Seleção de Eco** (P0), morte/vitória, hub |

### 👹 Inimigos & IA 🆕
> Preenche o gap que a [Revisão de Design (Sofia)](design/Design_Review_2026-06.md) apontou no **M3**. IA via GAS, **pausa no juggle**, telegrafe = leitura topdown P0.

| # | Doc | Prioridade | Conteúdo |
|---|---|---|---|
| 30 | [IA: Visão Geral](enemies/30_AI_Overview.md) | 🟢 P0 | AIController, BT vs StateTree, percepção, GAS, **pausa no `Airborne`** (gancho do juggle) |
| 31 | [Arquétipos](enemies/31_Enemy_Archetypes.md) | 🟢 P0 | Base + `UDDREnemyData`, grunt/ranged/elite, eixos de variedade, "canvas do pilar" |
| 32 | [Comportamento de Combate](enemies/32_Enemy_Combat_Behavior.md) | 🟢 P0 | **Telegrafe (decal)**, frame data, hyperarmor, poise→launch, token de ataque |
| 33 | [Spawning & Encontros](enemies/33_Spawning_Encounters.md) | 🟢 P0 | Encounter manager, ondas, colocação, escalada, token director |
| 34 | [Mini-Boss](enemies/34_MiniBoss.md) | 🟡 P1 | Fases, ataques-assinatura, **janela de vulnerabilidade** (combar o boss) |
| 51 | [Catálogo Inimigos MVP](enemies/51_Enemy_Catalog_MVP.md) 🆕 | 🟢 P0 | **4 inimigos** concretos: stats, abilities, frame data, encontros → `UDDREnemyData` |

### 🔊 Áudio · ✨ VFX · 🎯 Projéteis 🆕
> Camada sensorial e ranged do M3. Áudio/VFX aprofundam [21 — Juice](feel/21_Juice_FX.md); projéteis são **P0** pro Atirador.

| # | Doc | Prioridade | Conteúdo |
|---|---|---|---|
| 35 | [Áudio & Música](audio/35_Audio_Music.md) | 🟡 P1 (núcleo 🟢) | SoundClass/Mix, MetaSound, combate por peso, música, tells, projéteis, Settings |
| 36 | [VFX & Niagara (detalhado)](feel/36_VFX_Niagara.md) | 🟡 P1 (núcleo 🟢) | Spec por sistema, User Params, telegrafe, pooling, projétil, performance horda |
| 37 | [Projéteis (Atirador)](combat/37_Projectiles.md) | 🟢 P0 (M3) | `ADDProjectileBase`, DataAsset, telegrafe de linha, GAS, pool, leitura topdown |

### 🌐 Localização 🆕
> Suporte a vários idiomas desde o código (`FText`); tradução PT-BR + EN no P1.

| # | Doc | Prioridade | Conteúdo |
|---|---|---|---|
| 38 | [Localização & Idiomas](systems/38_Localization.md) | 🟡 P1 | String Tables, `UDDRLocalizationSubsystem`, Settings, pipeline de tradução |

---

## Decisões de projeto já tomadas (premissas)

Estas premissas guiam toda a doc. Mude aqui se discordar — o resto se ajusta.

| Decisão | Escolha | Por quê |
|---|---|---|
| **Câmera** | Topdown 3/4 fixa (pitch ~-55°), spring arm longo | Identidade ARPG (Diablo 4/POE2/Hades). Define o que vale animar. |
| **Rotação do personagem** | Orient-to-movement **ou** orient-to-cursor (ver doc 06) | ARPG topdown padrão; **Turn In Place removido do escopo**. |
| **Locomoção** | **Foundation bespoke** (state machine + distance match), Motion Matching como spike opcional | Menos assets p/ MVP; você já domina esse padrão no DungeonForged. |
| **Modo / Rede** | 🔒 **Single-player** (local-authoritative) | Decisão 2026-06. Sem replicação/predição → combate **agressivo**: CMC direto, `CustomTimeDilation`/slow-mo global, hitstop local, RootMotion sem reconciliação. Resolve a dívida de networking do juggle. |
| **Combate** | **GAS-driven** (abilities + montages + tags) | Escalável, data-driven, **local-authoritative**. É o coração do jogo. |
| **Dados** | **DataTables + DataAssets** ([44](systems/44_Data_Driven.md)) | Ataques, Ecos, inimigos, encontros, run — tuning sem recompile. |
| **Aéreo** | **`RootMotionSource` ancorado** (não `LaunchCharacter`) + state machine própria do alvo | Pilar de design; co-altitude previsível (rev. 2026-06, doc 16 §2). |
| **Idioma da doc** | PT-BR | Padrão dos seus projetos. |

---

## Status

📝 Documentação inicial criada e **revisada por mesa-redonda de design** ([Revisão 2026-06](design/Design_Review_2026-06.md)). Código do jogo ainda é o **template Third Person** (`ADarkDungeonRiftCharacter`).

🔬 **Próximo passo concreto:** **M⁻¹ — spike de validação** ([43 — playbook](43_Spike_Minus1.md), [Roadmap §1.5](17_Implementation_Roadmap.md)) **antes** do M0. Conteúdo e arquitetura data-driven já estão no papel: [40 Ecos](design/40_Eco_Pool_Catalog.md) · [41 Tags](systems/41_Gameplay_Tags.md) · [42 Run Manager](systems/42_Run_Room_Manager.md) · [44 Data-Driven](systems/44_Data_Driven.md). Depois do spike → [04 Setup](systems/04_Project_Setup.md) → [17 Roadmap](17_Implementation_Roadmap.md).
