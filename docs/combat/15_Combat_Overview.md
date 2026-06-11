# 15 — Combate (Hack'n'Slash) · 🟢 P0

> O **coração do jogo**. Sistema de combo melee data-driven via GAS, detecção de acerto, hit-stop e juice. Tudo aqui serve ao Pilar 1 (combate expressivo). É onde mais vale investir.

> **Pré-requisitos:** [GAS (doc 05)](../systems/05_GAS_Architecture.md), [Input + buffer (doc 07)](../systems/07_Input.md), [Slot 'DefaultSlot' no AnimGraph (doc 08)](../locomotion/08_Locomotion_Overview.md).

---

## 1. Anatomia de um combo

```
Apertou Attack → GA_Attack_Light ativa → toca AM_Combo (montage)
  Montage tem seções: [Atk1] → [Atk2] → [Atk3] → [Atk4(finisher)]
  Cada seção tem:
    • janela de hit (AnimNotifyState: liga/desliga hitbox)
    • janela de combo (AnimNotifyState: aceita próximo input → próxima seção)
  Apertou Attack de novo dentro da janela de combo? → pula pra próxima seção.
  Não apertou? → montage termina, volta pra locomoção.
```

O combo é **uma montage com seções**, dirigida por **input buffer** (doc 07) e **notifies**. Simples, escalável, data-driven.

---

## 2. A ability de ataque (GA_Attack_Light)

```
ActivateAbility:
  1. CommitAbility
  2. Determina a próxima seção (ComboIndex):
       - primeira ativação → "Atk1"
       - reativação dentro da janela → "Atk2", "Atk3"...
  3. PlayMontageAndWait(AM_Combo, StartSection = seção atual)
  4. WaitGameplayEvent("Combat.Hit")  → resolve dano (§4)
  5. Janela de combo (notify) seta um flag "pode encadear"
  6. Input bufferizado + flag → avança ComboIndex e re-ativa (ou usa MontageJumpToSection)
  7. Fim da montage / sem input → reset ComboIndex, EndAbility
```

**Onde guardar o ComboIndex:** num componente de combate (`UDDRComboComponent`) ou na própria ability (se instanced-per-actor). Reseta quando a janela de combo fecha sem input.

| Conceito GAS | Uso |
|---|---|
| `PlayMontageAndWait` | toca a montage, espera fim/interrupção |
| `WaitGameplayEvent` | recebe o evento de hit do AnimNotify |
| `MontageJumpToSection` | avança pro próximo golpe |
| Tag `State.Combat.Attacking` | bloqueia outras ações / sinaliza AnimBP |

---

## 3. Janelas (AnimNotifyState na montage)

Cada seção do combo precisa de **duas janelas**, marcadas como `AnimNotifyState` na montage:

| Janela | Notify | Função |
|---|---|---|
| **Hit window** | `ANS_Hitbox` | Liga a hitbox (detecção de acerto) no swing; desliga depois |
| **Combo window** | `ANS_ComboWindow` | Período em que apertar Attack encadeia o próximo golpe |

```
Seção Atk1: [───swing───][recovery]
ANS_Hitbox:      [██]              ← hitbox ativa só no swing
ANS_ComboWindow:     [████]        ← aceita input pro Atk2 aqui
```

> ⏱️ **Tuning é tudo.** Janela de combo cedo demais = combo "foge"; tarde demais = sente lento. Comece com combo window cobrindo ~40-70% da seção e ajuste no playtest. O **input buffer (doc 07 §5)** perdoa o jogador que aperta um tico cedo.

---

## 4. Detecção de acerto (hit detection)

Duas abordagens; escolha por arma/golpe:

| Abordagem | Como | Quando |
|---|---|---|
| **A — Hitbox por trace (recomendado)** | Durante `ANS_Hitbox`, faz sweep/trace na arma (sphere/capsule entre sockets `weapon_start`/`weapon_end`) a cada frame | Melee preciso. **Recomendado.** |
| **B — Overlap de collision** | Volume de colisão na arma liga/desliga | Mais simples, menos preciso |
| **C — Área (AoE)** | Sphere overlap no impacto | Ataques em área / slam |

**Fluxo (abordagem A):**
```
ANS_Hitbox.NotifyTick → sweep entre sockets da arma
  → acertou ator com ASC e tag inimigo?
  → envia GameplayEvent "Combat.Hit" pra própria ability (com o alvo)
  → ability aplica GE_Damage no ASC do alvo (escala por AttackPower)
  → evita hit duplo: lista de "já atingidos nesta seção"
```

> 🎯 **Canais de colisão** (`Hitbox`/`Hurtbox`) do [doc 04 §5](../systems/04_Project_Setup.md) entram aqui. Hitbox da arma testa contra Hurtbox do inimigo.

---

## 5. Juice — o que faz bater sentir BOM (P0, barato)

Sem juice, o combate sente "molhado". Estes são **baratos e essenciais**:

| Técnica | O que faz | Como |
|---|---|---|
| **Hit-stop / freeze frame** | Congela 1-3 frames no impacto → peso | **Freeze global** — ver caixa abaixo. Não é só pausa de montage. |
| **Screen shake** | Treme a câmera no acerto | GameplayCue (doc 06 §5) |
| **Knockback / impulso** | Empurra o inimigo | `GE` + **RootMotionSource** no alvo (não `LaunchCharacter` — trajetória previsível, [18 §5.3](18_Combat_System_Deep.md)) |
| **Hit VFX + SFX** | Faísca + som no contato | `GameplayCue.Hit.Light/.Heavy/.Air` (catálogo canônico: [21 §10](../feel/21_Juice_FX.md)) |
| **Hit flash** | Inimigo pisca branco | Material param no alvo |

> 🥊 **Hit-stop é o truque #1 do hack'n'slash.** 2-3 frames de pausa no impacto transformam o feel. DMC/Bayonetta vivem disso. Implemente cedo (M1).

### ⚠️ Hit-stop é FREEZE GLOBAL, não pausa de montage *(revisão de design — ver [Design Review](../design/Design_Review_2026-06.md), Kael)*

Pausar **só a montage** quebra no aéreo: o CMC continua aplicando launch/gravidade durante o "freeze", o personagem **desliza**, e o hit-stop **vaza**. Hit-stop AAA congela **tudo** pelos mesmos N frames:

```
No notify de hit:
  1. congela a anim (pause montage) do atacante
  2. congela/zera a VELOCIDADE do atacante E do alvo (CMC) por N frames
       - N escala com o peso do golpe (light ~2f → slam ~6-8f)
       - escala canônica por tipo de golpe: ver [21 — Juice §1](../feel/21_Juice_FX.md)
  3. SÓ DEPOIS do freeze soltar → aplica o launch / re-float / knockback
```

> 🪂 No juggle (doc 16 §3) o re-float e o hit-stop disparam no mesmo frame. Se o freeze não congelar o CMC do alvo, o pop "vaza" durante o congelamento e o clímax do M2 sente "molhado" sem ninguém saber por quê. **Congele os dois corpos, aplique o impulso depois.**

> 📦 **Hit-stop é só uma peça.** A camada sensorial completa (escala de intensidade, shake, slow-mo, VFX/SFX, GameplayCues) está em **[21 — Juice & FX](../feel/21_Juice_FX.md)**; a sensação cinestésica (peso, responsividade) em **[20 — Game Feel](../feel/20_Game_Feel.md)**; as regras a fundo (frame data, dano, reações, soft-lock) em **[18 — Combate Profundo](18_Combat_System_Deep.md)**.

---

## 6. Gramática de Cancelamento · 🟢 P0 no M1 *(revisão de design — ver [Design Review](../design/Design_Review_2026-06.md), Kael)*

> 🥋 **Isto É o jogo character-action.** Em DMC/Bayonetta, *o que cancela o quê, a partir de qual janela, pra qual estado* não é detalhe — é a fonte da expressão e do skill ceiling (Pilar 1, doc 01 §2). Sem esta tabela, o combo fica **on-rails**: bonito por 5 minutos, raso pra sempre. Por isso ela sobe pra **P0 do M1**, não "decisão futura".

A regra: cada golpe tem uma **janela de cancelamento** (AnimNotifyState, como a janela de combo do §3). Dentro da janela, certas ações **interrompem** o golpe atual e entram em outro estado. Fora dela, o golpe segue até a recovery.

### Tabela de cancelamento (preencher/tunar no M1)

| Estado atual ↓ \ Ação → | **Dash** | **Próximo ataque** | **Launcher** | **Jump / Air** |
|---|:---:|:---:|:---:|:---:|
| **Atk1 / Atk2 (golpe ativo)** | ✅ sempre (escape) | ⏳ só na janela de combo | ✅ na janela | ❌ |
| **Recovery do ataque** | ✅ sempre | ✅ (encadeia) | ✅ | ⏳ |
| **AirAttack (no ar)** | ⏳ air-dash (se houver) | ⏳ janela aérea | — | ✅ **jump-cancel → re-sobe** (extiende juggle) |
| **Slam (descida)** | ❌ (comprometido) | ❌ | ❌ | ❌ |

Legenda: ✅ permitido · ⏳ permitido só na janela · ❌ bloqueado · — n/a. *Estes valores são ponto de partida — tune no M1.*

> 🔑 **Cancelamentos-âncora do MVP (mínimo pra sentir "character-action"):**
> 1. **Dash-cancel** do ataque (sempre) → responsividade + escape.
> 2. **Attack→Launcher** na janela → a ponte chão↔ar (alimenta o pilar).
> 3. **Jump-cancel do AirAttack** → re-sobe o player no juggle (o "truque" do combo aéreo profundo).

**Como o GAS implementa:** `CancelAbilitiesWithTag` na ability que cancela. Ex.: `GA_Dash` lista `Ability.Attack` em `CancelAbilitiesWithTag` → dash sempre sai, mesmo no meio do combo. As janelas (`⏳`) são `AnimNotifyState` que ligam um flag "cancelável-para-X" lido pelo input buffer (doc 07 §5).

> 🐞 Dê um `ddr.CombatDebug` (no espírito do `ddr.LocomotionDebug`, doc 08 §5) que mostre em tela: estado atual, janelas abertas, e o último input bufferizado. Sem ver as janelas, tunar cancelamento é às cegas.

---

## 7. Conexão com locomoção

- A montage toca no **Slot 'DefaultSlot'** (doc 08 §4) → sobrescreve a pose de locomoção só na parte superior/relevante (use **slot por grupo de bones** se quiser correr + atacar; ou full body p/ golpes plantados).
- Tag `State.Combat.Attacking` → AnimBP pode reduzir locomoção / travar rotação.
- Ataque geralmente **trava ou reduz** o movimento (root motion do golpe manda). Decida por golpe: alguns avançam (lunge via root motion / motion warp).

---

## 8. Soft-lock + Motion Warping no ataque · 🟢 P0 (implementado)

Pipeline de 4 camadas ([18 §6](18_Combat_System_Deep.md)): **soft-lock** → **face no startup** → **motion warp** → **sweep**.

| Camada | C++ | Editor |
|---|---|---|
| Soft-lock + face | `UDDRCombatComponent::FaceSoftLockTarget` | tune `SoftLockRadius` / `SoftLockHalfAngleDegrees` no BP |
| Motion warp | `FaceAndSetupMotionWarp` + `UMotionWarpingComponent` | notify **Motion Warping** → warp target `AttackWarp` |

- Warp target canônico: **`AttackWarp`** (Skew Warp, Translation ON, Ignore Z, **Warp Rotation OFF**).
- Cap de distância (`MaxWarpDistance` ~200 cm) = whiff honesto fora do alcance.
- **`AM_Combo`:** uma janela Motion Warp **por seção** (Atk1–4) — C++ recalcula o alvo a cada golpe; sem notify na seção = encara mas não avança ([57 §2b](57_M1_Combo_Editor_Setup.md)).

> 🛠️ **Setup no editor:** [60 — M2 Editor Setup §7](../systems/60_M2_Editor_Setup.md) (combo chão, run-attack, launcher). Regras por ability (ground/air/launcher/slam) na mesma seção.

---

## 9. Checklist

- [ ] `GA_Attack_Light` com PlayMontageAndWait + seções
- [ ] `AM_Combo` com seções Atk1-4
- [ ] `ANS_Hitbox` (hit window) + `ANS_ComboWindow` (combo window) por seção
- [ ] ComboIndex avança por input bufferizado (doc 07)
- [ ] Hit detection (trace na arma) → GameplayEvent → GE_Damage
- [ ] Lista de "já atingidos" (sem hit duplo)
- [ ] Tag `State.Combat.Attacking`
- [ ] **Hit-stop** (2-3 frames) no impacto — P0
- [ ] Screen shake + hit VFX/SFX (GameplayCue)
- [ ] Dash cancela ataque (fluidez/escape)
- [ ] Montage no Slot 'DefaultSlot'
- [ ] Soft-lock + motion warp (`AttackWarp` nas montages) — [60 §7](../systems/60_M2_Editor_Setup.md)

---

## 10. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Combo não encadeia | Janela de combo / input buffer mal ligados | §3, doc 07 §5 |
| Acerta 2× no mesmo swing | Sem lista de já-atingidos | §4 |
| Bate "no vazio" | Falta motion warp / `AttackWarp` errado / fora do cap | §8, [60 §7](../systems/60_M2_Editor_Setup.md); tune soft-lock antes de aumentar hitbox |
| Sem peso no acerto | Sem hit-stop | §5 — implemente |
| Ataque "desliza" o personagem | Root motion + braking errados | Decida root motion por golpe |
| Montage não sobrepõe locomoção | Slot errado / AnimBP sem o slot | Use Slot 'DefaultSlot' (doc 08) |

---

## 11. Próximo passo

→ [16 — Combos Aéreos](16_Aerial_Combos.md): o pilar de assinatura — launcher, juggle e slam.
