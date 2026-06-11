# 22 — Inventário de Animação & Mapa de Assets · 🟢 P0 (referência)

> Mapeia o **set de animação que você tem** (~**334 clips**) contra os sistemas/docs que os consomem, com **prioridade MVP** por grupo. E — o mais importante — aponta os **gaps** (o que falta) e o **over-scope** (o que adiar). Os números à direita (Idle **4**, Attack **55**…) são **contagens de clips por categoria**.

> 🧭 **Por que este doc existe:** os docs anteriores foram escritos assumindo Mannequin/placeholder. Agora há um set real — isto **aterra** o plano em assets concretos e expõe onde o set e o MVP topdown casam (ou não).

---

## 0. Resposta curta: **SIM, ajuda muito** ✅

1. **Valida os docs contra assets reais** — deixa de ser teoria.
2. **Confirma a remoção do Turn In Place:** o set **não tem nenhum clip de TIP**. A decisão estava certa.
3. **Cobre o PILAR aéreo quase inteiro:** air combo, air-to-floor (slam), hit air, e *hit air large to floor* (o knockdown do juggle). O loop do [doc 16](combat/16_Aerial_Combos.md) tem assets de ponta a ponta. 🔥
4. **Cobre as reações de hit** do [doc 18](combat/18_Combat_System_Deep.md) (poise/stagger/knockdown) muito bem.
5. **Expõe 2 gaps críticos** (launcher, landings graduados) e **bastante over-scope** (parry, charge, buff, execution, block, equip) — coisas que, sem este mapa, você só descobriria tarde.

---

## 1. Visão geral (por categoria)

| Categoria | Clips | Papel no MVP |
|---|---:|---|
| **Idle** | 4 | 🟢 P0 — idle normal + combate |
| **Attack** | 55 | 🟢 P0 (núcleo do combate; parte é over-scope) |
| **Walk** | 92 | 🟡 P1 — gait *raro* em topdown (run é a base) |
| **Run** | 80 | 🟢 P0 (gait base) + over-scope (equip) |
| **Jump** | 22 | 🟢 P0 — base do combo aéreo |
| **Dodge** | **48** (6 variantes × **8 direções** — corrigido: a lista original dizia "4 ways/16") | 🟢 P0 — o verbo de esquiva (= dash); impl. em [59](combat/59_Directional_Dodge.md) |
| **Roll** | 16 | 🟡 P1 — esquiva alternativa (reconciliar, §7) |
| **Hit** | 49 | 🟢 P0 — reações (e o knockdown do juggle) |
| **Total** | **~334** | |

---

## 2. Mapa: clip → sistema → prioridade

### 🥋 Attack (55) — o coração

| Grupo | Consome em | Prio | Nota |
|---|---|:---:|---|
| **4 Attacks combo** | [15](combat/15_Combat_Overview.md)/[18](combat/18_Combat_System_Deep.md) `AM_Combo` · [19](combat/19_Abilities_Deep.md) `GA_Attack_Light` | 🟢 **P0** | O combo de chão. As 4 seções (Atk1-4) saem daqui. |
| **1 Attack combo air** | [16](combat/16_Aerial_Combos.md) `GA_AirAttack` | 🟢 **P0** | Pilar — o combo no ar. |
| **3 Attack Air to Floor** | [16 §5](combat/16_Aerial_Combos.md) `GA_AirSlam` | 🟢 **P0** | **O slam/finisher!** 3 variantes = ótimo p/ o clímax. |
| **1 Dash Air Attack** | [16](combat/16_Aerial_Combos.md)/[19](combat/19_Abilities_Deep.md) | 🟡 P1 | Mobilidade aérea. *(O "2×" na lista original era duplicata → conta como **1**.)* |
| **2 Run attacks** | [15](combat/15_Combat_Overview.md)/[19](combat/19_Abilities_Deep.md) | 🟢 **M2** | Estocada do **dash-attack** (dash→LMB cancela o dodge, [60 §6](systems/60_M2_Editor_Setup.md)); `Run_Attack_01` já em uso como opener. |
| **2 Charge Attack** | [18](combat/18_Combat_System_Deep.md)/[19](combat/19_Abilities_Deep.md) | 🟡/🔵 | Pesado/carregado — ótimo, mas além do combo MVP. |
| **1 Parry counter** | — (sistema de parry) | 🔵 P2 | **Over-scope** — parry não está no MVP (§5). |
| **2 Buff** | [19](combat/19_Abilities_Deep.md) (ability/ultimate) | 🔵 P2 | Auto-buff; casa com Ecos/ultimate ([03b](design/03b_Reward_System.md)). |
| **3 Execution** | — (finisher cinemático) | 🔵 P2 | **Over-scope** — execuções são pós-MVP (§5). |

### 🚶 Walk (92) & 🏃 Run (80) — locomoção

| Grupo | Consome em | Prio | Nota |
|---|---|:---:|---|
| **Run normal 8-way** | [09](locomotion/09_Gaits.md) | 🟢 **P0** | Run é o **gait base**. Subset (fwd) já roda no M0. |
| **2 Run speed** | [09 §1-3](locomotion/09_Gaits.md) | 🟢 **P0** | Jog + **sprint** (os gaits). |
| **Run combat 8-way** | [09](locomotion/09_Gaits.md)/[15](combat/15_Combat_Overview.md) | 🟡 P1 | Locomoção travada em combate (strafe). |
| **Walk normal 8-way** | [09](locomotion/09_Gaits.md) | 🟡 P1 | ⚠️ Walk é **raro** em ARPG topdown (§6). |
| **Walk combat 8-way** | [09](locomotion/09_Gaits.md)/[15](combat/15_Combat_Overview.md) | 🟡 P1 | idem. |
| **Walk block 8-way** | — (guarda/bloqueio) | 🔵 P2 | **Over-scope** — bloqueio não é MVP (§5). |
| **Walk/Run equip** | — (troca de arma) | 🔵 P2 | **Over-scope** — MVP tem 1 arma fixa ([02](design/02_MVP_Scope.md)). |

### 🦘 Jump (22) · 🌀 Dodge (16) · 🤸 Roll (16)

| Grupo | Consome em | Prio | Nota |
|---|---|:---:|---|
| **Jump 4-way** | [13](locomotion/13_Jump_Fall_Landing.md) | 🟢 **P0** | Pulo (base do aéreo). 4-way é generoso; MVP precisa de pouco. ⚠️ ver gaps (landing, §4). |
| **Dodge 8-way × 6 variantes** | [19](combat/19_Abilities_Deep.md) `GA_Dash` + [59](combat/59_Directional_Dodge.md) | 🟢 **P0** | Esquiva com i-frames. **M1 usa `02_Dodge_Combat`** (8 seções na `AM_Dodge`); `03/04_Air` → M2; `01` e `05/06_to_Run` → P2. |
| **Roll 4-way** | [19](combat/19_Abilities_Deep.md) | 🟡 P1 | Esquiva alternativa — **reconciliar dash/dodge/roll** (§7). |

### 💥 Hit (49) — reações (gold pro combate E pro pilar)

| Grupo | Consome em | Prio | Nota |
|---|---|:---:|---|
| **Hit / Hit combat** | [18](combat/18_Combat_System_Deep.md) (hit reaction) | 🟢 **P0** | Reação base de dano. |
| **Hit air / Hit combat air** | [16 §3.1](combat/16_Aerial_Combos.md) (alvo no Float) | 🟢 **P0** | **Pilar** — a reação do alvo a cada hit do juggle. |
| **Hit air large to floor** / **combat air large to floor** | [16 §5](combat/16_Aerial_Combos.md) (SlamDown→chão) | 🟢 **P0** | **Pilar** — o knockdown quando o slam joga o alvo no chão. 🔥 |
| **Hit large / Hit large combat** | [18](combat/18_Combat_System_Deep.md) (stagger/poise) | 🟡 P1 | Reação pesada (guarda quebrada → elegível a launch, [16 §2](combat/16_Aerial_Combos.md)). |
| **Knock down / Getup / KD combat** | [18](combat/18_Combat_System_Deep.md)/[16 §3.1](combat/16_Aerial_Combos.md) (Stun→Getup) | 🟢/🟡 | Recuperação pós-knockdown. |
| **Block** | — (guarda) | 🔵 P2 | **Over-scope** (§5). |

### 🧍 Idle (4)
Idle normal + idle combate → [08](locomotion/08_Locomotion_Overview.md)/[09](locomotion/09_Gaits.md) (estados idle) e [15](combat/15_Combat_Overview.md) (postura de combate). 🟢 **P0**.

---

## 3. 🟢 O que o set cobre MUITO bem

- **O loop aéreo inteiro tem assets:** `Attack combo air` (combar) → `Attack Air to Floor` ×3 (slam) → `Hit air` (reação do alvo) → `Hit air large to floor` (knockdown). É raro um set cobrir o pilar de ponta a ponta. **Isto reduz muito o risco de assets do M2.**
- **Reações de hit ricas** (49 clips): normal/combat × air/large/knockdown + getup. O [doc 18](combat/18_Combat_System_Deep.md) (poise/stagger/reações) e a [SM do alvo (16 §3.1)](combat/16_Aerial_Combos.md) estão **lastreados**.
- **Gaits cobertos:** run 8-way + 2 velocidades + combat = a base de locomoção do [09](locomotion/09_Gaits.md).
- **Esquiva sobrando:** dodge **e** roll 4-way — dá pra ter dash (i-frame) e um roll mais longo se quiser.

---

## 4. ⚠️ GAPS — o que FALTA pro MVP

| Gap | Por que é crítico | Mitigação |
|---|---|---|
| **🔴 Launcher / uppercut dedicado** | É a **entrada do pilar** ([16 §2](combat/16_Aerial_Combos.md)) — o golpe que joga o alvo pro alto. **Não há clip explícito.** | **Input dedicado `IA_Launcher`** com clip próprio — Kael: reciclar o 4º golpe do combo sente "fim", não "início". Sourcing de 1 uppercut tosco é barato. **Entrou no spike M⁻¹** ([17 §1.5](17_Implementation_Roadmap.md)). |
| **🟡 Landings graduados (soft/medium/hard)** | [13 §6](locomotion/13_Jump_Fall_Landing.md) precisa de pouso por altura; o **slam** ([16 §5](combat/16_Aerial_Combos.md)) precisa de um **hard land** com peso. "Jump 4-way" provavelmente tem 1 land genérico, não graduado. | Confirmar o que há dentro dos 22 de Jump; se faltar, 1 hard-land é suficiente p/ o MVP (reusado no slam). |
| **🟢 Dash de chão** | Os docs falam "dash"; o set tem **Dodge** (i-frame) e **Roll**. | **Sem gap real** — mapeie `Dodge` → `GA_Dash` (§7). |

> 🎯 **O gap do launcher é o único bloqueante do pilar.** Tudo mais o set cobre. Decisão ([Review de Combate](design/Design_Review_Combat_2026-06.md)): **clip dedicado + `IA_Launcher`**, não reciclar o 4º golpe (sente "fim", não "início"). Provado no **spike M⁻¹**.

---

## 5. 🔵 OVER-SCOPE — ótimo, mas ADIE (pós-MVP)

Estes clips implicam **sistemas inteiros** que o [escopo do MVP (02)](design/02_MVP_Scope.md) deixou de fora. Não os jogue fora — **sequencie pra depois**:

| Clips | Sistema implicado | Quando |
|---|---|---|
| **1 Parry counter** | Parry **ofensivo** (parry→stagger→**launch**) | 🟡 **P1** — *primeiro* pós-core; barato (janela+tag, reusa poise). Reclassificado P2→P1 ([Review Combate](design/Design_Review_Combat_2026-06.md)) |
| **Block + Walk block 8-way** | Guarda/bloqueio direcional | Pós-MVP |
| **3 Execution** | Finishers em inimigos fracos (cinemático) | Pós-MVP (alto polish) |
| **2 Charge Attack** | Ataque carregado (hold) | P1-P2 (extensão do combo) |
| **2 Buff** | Abilities de auto-buff | P2 (casa com Ecos/ultimate, [03b](design/03b_Reward_System.md)/[19](combat/19_Abilities_Deep.md)) |
| **Walk/Run equip** | Troca/saque de arma | Pós-MVP (MVP = 1 arma fixa) |

> ⚠️ **Armadilha de escopo:** ter o clip ≠ precisar do sistema. **Exceção (Review de Combate):** o **parry é barato** (janela+tag, reusa poise) → promovido a **P1 ofensivo** (parry→launch), primeiro item pós-core. Block/execution/charge/buff/equip **continuam over-scope**. O MVP-core segue combo de chão + aéreo ([02](design/02_MVP_Scope.md)).

---

## 6. 🧭 ROI topdown: cuidado com os 172 clips de 8-way

Walk (92) + Run (80) = **172 clips direcionais**. Dois alertas honestos:

1. **Os docs ([09 §5](locomotion/09_Gaits.md)) recomendam um blendspace** pro MVP, não 8-way autorado à mão. Agora você **tem** o 8-way → *pode* usar, mas **não trave o M0** fiando os 8 ângulos × normal/combat/block. Comece com um subset (fwd + um blendspace 2D) e expanda depois — chega no jogável mais rápido.
2. **Walk é o gait RARO** em ARPG topdown (run é a base, [09 §1](locomotion/09_Gaits.md)). 92 clips de walk é **ROI menor** do que a contagem sugere. Priorize wirar **Run** bem; walk pode esperar.

> Não é desperdício ter os clips — é sequência. O jogador chega ao fun (combate) mais rápido se você não gastar o M0 fiando 172 direções que a câmera de cima mal distingue.

---

## 7. Reconciliações com decisões existentes

| Item do set | Decisão | Ação |
|---|---|---|
| **Dodge vs Roll vs "dash"** | Docs usam "dash". Você tem **Dodge** + **Roll**. | **UMA esquiva no MVP** ([Review Combate](design/Design_Review_Combat_2026-06.md)): `Dodge → GA_Dash` (i-frame 2 camadas + **perfect-dodge**, [19](combat/19_Abilities_Deep.md)). **`Roll` CORTADO** (volta pós-MVP só com assinatura distinta: longo/comprometido/sem i-frame frontal). |
| **"2 Run speed"** | = jog + sprint | São os gaits do [09](locomotion/09_Gaits.md). ✅ |
| **Idle normal & combat** | Tag `State.Combat.*` troca a postura | [15](combat/15_Combat_Overview.md)/[08 §6](locomotion/08_Locomotion_Overview.md). ✅ |
| **Sem Turn In Place** | TIP foi removido | ✅ Set confirma — nada a fazer. |

---

## 8. Mapa por milestone (quais clips por fase)

| Milestone | Clips necessários (mínimo) |
|---|---|
| **M⁻¹ spike** | Nenhum — cubos ([17 §1.5](17_Implementation_Roadmap.md)). |
| **M0** | Idle, **Run** (subset/blendspace), Jump (+land), Dodge. |
| **M1** | Idle combat, **4 Attacks combo**, **Hit/Hit combat**, Run attack. |
| **M2 (pilar)** | **Launcher** (GAP §4!), **Attack combo air**, **Attack Air to Floor** (slam), **Hit air**, **Hit air large to floor**, hard-land. |
| **M3** | Hit large/knockdown/getup (reações dos inimigos), Block (se houver guarda — senão pula). |
| **Pós-MVP** | Parry, Charge, Buff, Execution, equip, walk block, walk 8-way completo. |

---

## 9. Checklist

- [ ] Decidir o **launcher** (4º golpe do combo vira uppercut, ou clip dedicado) — gap crítico §4
- [ ] Confirmar **landing** dentro dos 22 de Jump (existe hard-land p/ o slam?)
- [x] Mapear **Dodge → GA_Dash** ✅ direcional 8-way implementado ([59](combat/59_Directional_Dodge.md)); Roll segue cortado
- [ ] M0: começar com **Run subset/blendspace**, não 8-way completo (§6)
- [ ] Marcar Parry/Charge/Buff/Execution/Block/Equip como **pós-MVP** (não construir sistemas agora)
- [ ] Idle/Run/Hit prontos p/ M0-M1; pilar aéreo prov. coberto p/ M2 (menos o launcher)

---

## 10. Próximo passo

→ O gap do **launcher** afeta o [16 §2](combat/16_Aerial_Combos.md) — decida o mapeamento antes do M2. Para a ordem geral: [17 — Roadmap](17_Implementation_Roadmap.md). Para o que cada clip faz no combate: [18](combat/18_Combat_System_Deep.md) (regras) e [16](combat/16_Aerial_Combos.md) (aéreo).
