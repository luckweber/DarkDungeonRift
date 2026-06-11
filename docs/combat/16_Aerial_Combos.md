# 16 — Combos Aéreos · 🟢 P0

> A **assinatura do jogo** (doc 01 §4). Lançar inimigos pro alto e mantê-los flutuando com ataques aéreos, fechando com um slam. É o que diferencia o Dark Dungeon Rift de um ARPG topdown comum.

> **Pré-requisitos:** [GAS (05)](../systems/05_GAS_Architecture.md), [Combate melee (15)](15_Combat_Overview.md), [Jump/Fall/Landing (13)](../locomotion/13_Jump_Fall_Landing.md) — o juggle é construído sobre a base vertical do doc 13.

---

## 1. O loop do combo aéreo

```
[Launcher] → inimigo (e player) sobem
   ↓
[Juggle] → ataques aéreos mantêm o alvo flutuando (RootMotionSource dirige a altura — §3)
   ↓
[Air Combo] → 2-3 golpes aéreos encadeados
   ↓
[Slam/Finisher] → golpe descendente arremessa o alvo no chão (hard land + AoE)
```

Cada etapa é uma **GameplayAbility** (doc 05), gated por tags. O `State.Combat.InAir` é o que habilita os ataques aéreos.

---

## 2. Launcher (GA_Launcher)

O golpe que **inicia** o aéreo. Lança o **alvo** pra cima e (opcionalmente) o **player** junto.

> ⚠️ **CONTRATO TÉCNICO (revisão de design — ver [Design Review](../design/Design_Review_2026-06.md), Davi · risco ALTA):** use **`RootMotionSource` ancorado**, **não** `LaunchCharacter`, para subir alvo e player. `LaunchCharacter` é impulso balístico não-determinístico: com gravidades/impulsos independentes, os dois corpos **divergem em altura já na 2ª pancada** e o follow launch quebra. Pior: `LaunchCharacter` não prediz/replica bem (dívida de networking, §9). `RootMotionSource` (vertical force ou MoveToForce com altura-alvo) dá **posição vertical autoritativa e previsível** → os dois ficam co-altitude porque você *dirige* a altura, não a "joga".

```
GA_Launcher (Ability.Attack.Launcher):
  1. Toca AM_Launcher (uppercut)
  2. No notify de impacto → no ALVO:
       - aplica tag State.Combat.Airborne (alvo "juggável")
       - RootMotionSource (vertical) leva o alvo à AltitudeAlvo e o SEGURA lá ("hang")
       - entra na state machine do ALVO (§3.1) — NÃO no bIsFalling normal
  3. No PLAYER (se follow):
       - RootMotionSource leva o player à MESMA AltitudeAlvo + seta tag State.Combat.InAir
       - habilita as abilities aéreas (GA_AirAttack/GA_AirSlam)
```

| Parâmetro | Valor inicial | Nota |
|---|---|---|
| AltitudeAlvo (pop inicial) | 250-350 cm | Altura que o RootMotionSource **dirige** (não "Launch Z balístico") |
| Tempo de hang no topo | ~0.3-0.5 s | Quanto o alvo "segura" antes de poder cair |
| Player sobe junto? | **Sim** (recomendado) | "Follow launch" estilo DMC — mantém player junto do alvo |
| Player AltitudeAlvo | **≈ altura do RM do player** | Co-altitude: o **player** ancora no fim do pulo; o **alvo** alinha ~60 cm acima (não o contrário) |

> ✅ **Implementação M2 (C++ atual):** o design ainda prefere altura *dirigida* (não balística). Na prática:
> - **Player:** root motion de `Attack_Up_Floor_To_Air` em `AM_Launcher` (`GA_Launcher` põe `MOVE_Flying` durante a montage); ao acertar, `EnterAirCombat` fixa `AirAnchorZ` na **altura atual do player** (`MaintainAirAltitude`).
> - **Alvo:** `StartAirborne(LaunchRiseHeight)` no hit (`bLaunchTargets`); ao fim da montage, snap de co-altitude (`JuggleTargetHeightAbovePlayer` acima do `AirAnchorZ`).
> - **Tuning no editor:** `BP_GA_Launcher` → **Launch Rise Height** + **Juggle Target Height Above Player**; fallback global no Combat Component do player — [60 §2/§5](../systems/60_M2_Editor_Setup.md).

> 🎮 **Follow launch** (player sobe junto) é o que faz o combo aéreo funcionar em ação. Sem isso, o inimigo sobe e o player fica no chão olhando. **Mas** ele tem custo de leitura em topdown — ver §6 (é a pergunta que o spike M⁻¹ precisa responder).

> 🔒 **No hold aéreo o stick NÃO move o corpo** (`MaxFlySpeed=0` enquanto `bInAirCombat`/pin do slam — o `MOVE_Flying` aceitaria WASD e dava pra "andar no ar"). Deslocamento no ar vem dos **golpes** (root motion + carry do alvo) e do **air-dash** — input deliberado, não strafe livre.

> 🛡️ **Quem pode ser lançado? (gate de poise — ver [18 §5](18_Combat_System_Deep.md)):** **trash** (lixo) tem poise baixo/zero → **lança fácil**, preservando a fantasia "lanço qualquer um" (o pilar não pode ter atrito no caso comum). **Elites/bosses** exigem **quebrar a guarda/poise** antes de o launcher pegar (senão tomam hyperarmor). Profundidade no inimigo grande, fluidez no pequeno.

> 🧪 **Derisk obrigatório (consenso da mesa):** prove a co-altitude num **spike de cubos** (um cubo lança outro, os dois ficam ancorados no ar ~2s com `RootMotionSource`) **antes** do M2. Ver [Roadmap → M⁻¹](../17_Implementation_Roadmap.md).

---

## 3. Manter o alvo no ar (juggle physics)

O segredo do juggle é o alvo **não cair rápido**:

- **Cada hit aéreo dá um "pop" extra** pra cima (RootMotionSource curto) → reseta a queda, mantém flutuando.
- **Hitstop** em cada hit (doc 15 §5 — **freeze global**, aplica o pop *depois* do freeze) também segura o tempo.
- **Juggle decay:** cada hit dá menos altura (anti-loop infinito) — o alvo eventualmente cai.

> ⚙️ **A gravidade do alvo NÃO é o mecanismo.** Durante `State.Combat.Airborne`, a altura do alvo é **dirigida por RootMotionSource** (pop + hang + decay), não por gravidade balística — o `GravityScale` do alvo fica **~0** e a SM do alvo (§3.1) controla a queda no fim. É o contrato do §2 (você *dirige* a altura, não a *joga*). **Não** mexa em `GravityScale 0.3-0.5` — isso conflita com o RootMotionSource.

### Modelo numérico do juggle *(revisão de design — Kael; tune com debug)*

"Re-flutua a cada hit" sem números vira pudim flutuante (paira pra sempre) ou cai rápido demais (frustrante). Comece com:

| Variável | Valor inicial | Função |
|---|---|---|
| **Hang-time alvo por hit** | ~1.1 s (`TargetAirborneHoldSeconds`) | Timer renovado a cada hit |
| **Co-altitude por hit** | ~60 cm (`JuggleTargetHeightAbovePlayer`) — **aceita negativo** (alvo ABAIXO do player; ~-30 a -60 pros swings descendentes) | Alvo fica **relativo ao `AirAnchorZ` do player**, não stack de +150 cm |
| **Nudge extra por hit** | ×0.15 (`AirPopVerticalNudgeScale`) | Pequeno bump com decay — anti-infinito sem subir ao céu |
| **decayFactor** | 0.85 (`AirPopDecay`) | Nudge *= decay^hits |
| **Cap de hits no ar** (`MaxJuggleHits`) | **7** | Teto anti-abuse (`UDDRCombatComponent`) |

**Tuning no editor (preferido):** `BP_GA_AirAttack` → **Juggle Target Height Above Player** · **Air Pop Vertical Nudge Scale** (copiados ao ativar; fallback no Combat Component).

```
Hit aéreo no alvo (ANS_DDRHitbox bAirPop, após hit-stop):
  → GE_Damage
  → SetAirborneTargetZ(AirAnchorZ + JuggleTargetHeightAbovePlayer + nudge*decay^hits)
  → se hits >= MaxJuggleHits → EndAirborne (alvo cai)
```

> 🐞 **`ddr.JuggleDebug 1`** (no espírito do `ddr.LocomotionDebug`, doc 08 §5): desenhe em tela **altura do alvo, PopHeight atual, decay, contagem de hits**. Você precisa *ver* o juggle pra tunar o feel — é número, não intuição.

### 3.1 — Máquina de estados do ALVO (separada do player!) · *(revisão — Davi, risco ALTA)*

⚠️ **O alvo NÃO herda a Air SM do player** (a do `bIsFalling`, doc 13 §4). Mexer em altura + IA pausada (§8) com a state machine de queda normal rodando cria estados ambíguos ("é fall ou juggle?") e o alvo **treme** entre poses. Dê ao alvo uma SM própria, dirigida pela tag `State.Combat.Airborne`:

```
[SM do ALVO juggleado]   (gate: tag State.Combat.Airborne)
   Knockback (reage ao launcher) → Float (loop aéreo, reage a cada hit)
        → SlamDown (arremessado pelo slam) → (impacto) → Stun (recovery no chão)
```

Desacoplado do `bIsFalling`. Quando `Airborne` sai (pós-slam), o alvo volta ao fluxo normal de locomoção/IA.

> ✅ **Implementação M2:** o estágio **SlamDown→impacto→Stun é RAGDOLL físico** (`bRagdollOnSlammed` na base, [60 §4 item 10](../systems/60_M2_Editor_Setup.md)) — sem anim de queda: o corpo vira física no contato e levanta após `RagdollRecoverSeconds`. A SM acima vale pros estágios *aéreos* (Knockback/Float); o getup animado dos inimigos M3 é P1 ([51](../enemies/51_Enemy_Catalog_MVP.md)).

---

## 4. Ataques aéreos (GA_AirAttack)

```
GA_AirAttack (Ability.Attack.Air):
  - ActivationRequiredTags: State.Combat.InAir   ← só sai no ar
  - toca AM_AirCombo (seções, igual ao combo de chão — doc 15)
  - hit detection idêntico (trace na arma)
  - cada hit re-flutua o alvo (§3)
  - encadeia por input buffer (doc 07), igual ao chão
  - **sets de anim:** `06_Combo_Attack_Air_06` avança no ar (RM) · `07_Wave_07` fica parado — no set 06, `ANS_DDRHitbox` com **`bCarryAirborneTargets`** arrasta o alvo no XY ([60 §3](../systems/60_M2_Editor_Setup.md))
  - **tuning juggle:** `JuggleTargetHeightAbovePlayer` + `AirPopVerticalNudgeScale` no `BP_GA_AirAttack` (§3 acima)
```

> Reaproveita **toda** a arquitetura do combo de chão (doc 15): seções, janelas, hit detection, buffer. A diferença é a tag `InAir` e o re-float do alvo. Não reinvente — estenda.

---

## 5. Slam / Finisher (GA_AirSlam)

Fecha o combo com **peso**:

```
GA_AirSlam (Ability.Attack.AirSlam) — como implementado:
  1. toca AM_AirSlam seção "Start"; C++ liga Start→Loop e self-loopa o Loop
  2. ExitAirCombat(slam): estende o hold do ALVO juggleado (~2s); mantém
       ActiveJuggleTarget até o slam acabar (para snap + sweep no End)
  3. Falling + velZ -3500 INCONDICIONAL + tag State.Combat.SlamFall (AnimBP)
       → IgnoreRootMotion durante a queda (restaurado no EndAbility)
  3b. homing XY no alvo do soft-lock (cap 350cm — whiff honesto)
  4. Slam End Trigger (default BeforeGroundProximity):
       a ~250cm do chão → JumpToSection("End")
       b SnapSlamEndToJuggleTarget (co-altitude com o alvo no ar)
       c BeginSlamAirPin (congela Z ANTES do notify — velocity pousa em 1 frame)
  5. ANS_DDRHitbox na seção End (obrigatório):
       bSlamDownTargets + bAoEAtOwner (coluna vertical) + Pin In Air
       → sweep contínuo enquanto pinado no End (não só a duração do notify)
       → na seção End o C++ IGNORA bResumeFallAfterSlamPin (mantém no ar
         até a montage acabar; evita cair no meio do finisher)
       → derrube do alvo = RAGDOLL físico (bRagdollOnSlammed na base): corpo limp
         despenca, cápsula segue o pelvis, levanta após RagdollRecoverSeconds;
         caído = inatingível (janela de knockdown)
  6. fim da montage → solta o pin em QUEDA NATURAL (PostSlamFallVelocity, 0 = gravidade)
       → AnimBP assume: Fall Loop da locomoção → pouso com anim de land (SEM teleporte)
```

> 💥 O slam é o **clímax** — hit-stop e derrube vêm do **hitbox** na anim End (com player **congelado no ar** na co-altitude do juggle). **Pós-End: queda natural** (gravidade) caindo no **Fall Loop da locomoção** até o land — sem teleporte e sem sweep automático no `LandedDelegate`. Tag `State.Combat.SlamFall` → `Jump_Combat_*` no AnimBP ([58 §1.3](../locomotion/58_AnimGraph_Step_by_Step.md)).

---

## 6. Leitura em topdown (o desafio crítico)

Combo aéreo em topdown **só funciona se o jogador entender a altura**. Sem isso, vira confusão. Use (do [doc 06 §4](../systems/06_Camera_TopDown.md)):

| Técnica | Obrigatoriedade |
|---|---|
| **Sombra no chão** (player + alvo) | 🟢 **Obrigatório** — o gap sombra↔corpo = altura |
| **Escala sutil no ar** | 🟢 Recomendado |
| **Outline no alvo juggleado** | 🟡 Foco visual |
| **Leve zoom-out no juggle** | 🟡 Cabe a ação vertical |
| **Trail vertical no slam** | 🟡 Comunica o "desce!" |

> 🪂 **Sem sombra no chão, o combo aéreo é ilegível.** Trate como requisito do pilar, não polish. Teste cedo (M2) com um inimigo e veja se *você* entende a altura sem pensar.

> ❓ **A PERGUNTA DE UM MILHÃO (revisão de design — Sofia × Kael; ver [Design Review](../design/Design_Review_2026-06.md)):** o **follow launch** (§2, player sobe junto) é o que dá a fantasia DMC — mas em topdown, com a câmera de cima, o player subindo pode **sumir debaixo do próprio sprite** ou virar uma massa ambígua com o inimigo. *Follow launch ajuda o combo OU destrói a leitura que as sombras tentam salvar?* **Ninguém sabe ainda.** É a suposição mais arriscada do projeto e o **spike M⁻¹** ([Roadmap](../17_Implementation_Roadmap.md)) existe pra responder isto antes de você construir a infra toda. Alternativas a testar no spike: follow launch parcial (player sobe menos que o alvo), câmera com leve zoom-out no juggle, ou player fica no chão e só o alvo sobe.

---

## 7. Estado & tags (o esqueleto)

```
Chão:        (nenhuma tag aérea)
Launcher →   player: State.Combat.InAir   |  alvo: State.Combat.Airborne
Juggle:      ambos mantêm tags; abilities aéreas habilitadas
Slam →       remove tags no impacto; volta pro chão
```

| Tag | Quem tem | Habilita / bloqueia |
|---|---|---|
| `State.Combat.InAir` | player | habilita GA_AirAttack/GA_AirSlam; AnimBP usa SM aérea de combate |
| `State.Combat.SlamFall` | player | durante `GA_AirSlam` — AnimBP usa `Jump_Combat_Loop` / `Jump_Combat_End` |
| `State.Combat.Airborne` | alvo | alvo recebe re-float; IA pausada; pode ser combado |

> O AnimInstance lê `State.Combat.InAir` (doc 08 §6) → usa a state machine aérea de **combate** (não a de queda normal do doc 13). Mesmo ar, contexto diferente.

---

## 8. IA durante o juggle

Inimigo sendo combado precisa "se comportar":

- Pausa a IA/ataques enquanto `State.Combat.Airborne`.
- Toca pose de "knockback aéreo" (reage aos hits).
- Ao cair (pós-slam), breve stun antes de voltar à ação (janela de recuperação pro player).

> IA detalhada está fora do escopo desta doc, mas o **hook é a tag** `Airborne`: a IA checa e se desliga. Plugável no GAS.

---

## 9. Balanceamento (anti-abuse)

| Risco | Mitigação |
|---|---|
| Combo aéreo infinito | **Juggle decay** (§3) — cada hit dá menos altura |
| Cheese de 1 inimigo eternamente | Decay + cap de hits no ar (ex.: máx 6-8) |
| Player invulnerável no ar | Decida: pode tomar dano de outros inimigos no ar? (recomendado **sim**, pra ter risco) |
| Trivializa o jogo | Custo (stamina/cooldown no launcher) ou recurso |

---

## 10. Ordem de implementação (M2)

```
1. GA_Launcher: sobe alvo + player (follow) via RootMotionSource (alvo GravityScale~0)   ← primeiro
2. GA_AirAttack: 1 golpe aéreo que re-flutua o alvo
3. Sombra no chão + escala (LEITURA — testar já aqui!)
4. Encadeamento aéreo (seções + buffer, reusa doc 15)
5. GA_AirSlam: desce + hard land + AoE
6. Juggle decay + balance
```

> ✅ Depois do passo 3 você já sente se o pilar funciona. Se o combo aéreo for divertido e legível com 1 launcher + 1 hit + sombra, o jogo tem alma. **Esse é o momento mais importante do MVP.**

---

## 11. Checklist

- [ ] `GA_Launcher` sobe alvo **e** player (follow launch)
- [ ] Alvo dirigido por **RootMotionSource** (GravityScale~0 no Airborne), não gravidade balística
- [ ] Tags `State.Combat.InAir` (player) / `Airborne` (alvo)
- [ ] `GA_AirAttack` requer `InAir`, re-flutua o alvo a cada hit
- [ ] Encadeamento aéreo reusa seções/buffer do doc 15
- [ ] `GA_AirSlam`: desce + Hard Land (doc 13) + AoE
- [ ] **Sombra no chão** (player + alvo) — leitura
- [ ] Escala/outline/zoom de leitura aérea
- [ ] Juggle decay (anti-infinito)
- [ ] IA pausa em `Airborne`
- [ ] AnimBP usa SM aérea de combate via tag (doc 08)

---

## 12. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Inimigo sobe, player fica no chão | Sem follow launch | §2 — suba o player junto |
| Alvo cai rápido demais p/ combar | PopHeight/hang baixos OU gravidade ativa no alvo | §3 — RootMotionSource pop + GravityScale~0 (não gravidade balística) |
| Não entendo a altura (ilegível) | Sem sombra no chão | §6 — obrigatório |
| Combo aéreo infinito | Sem decay | §3, §9 |
| Air attack sai no chão | Faltou `ActivationRequiredTags: InAir` | §4 |
| Slam sem impacto | Não reusa Hard Land | §5, doc 13 §6 |
| IA "ataca" enquanto é combada | IA não checa `Airborne` | §8 |

---

## 13. Próximo passo

Combate completo. → Junte tudo na ordem certa: [17 — Roadmap de Implementação](../17_Implementation_Roadmap.md).
