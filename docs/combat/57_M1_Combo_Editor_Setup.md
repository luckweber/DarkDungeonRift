# 57 — M1: Setup do Combo no Editor (AM_Combo + BP_GA_AttackLight) · 🟢 P0 (M1)

> **Registro de implementação do M1** no editor: o setup real da montage `AM_Combo` e do `BP_GA_AttackLight`, incluindo o **fix do bug "todas as seções tocam sozinhas"**. Implementa no editor o que [15 §2-3](15_Combat_Overview.md), [18 §4](18_Combat_System_Deep.md) e [19 §3](19_Abilities_Deep.md) especificam.

---

## 0. 🐛 O bug encontrado (e a causa exata)

**Sintoma:** apertando ataque **1 vez**, a montage tocou `Atk1 → Atk2 → Atk3 → Atk4` inteira — ignorando a janela de combo. E errando a janela, não parava no fim da seção.

**Causa:** no painel **Montage Sections** da `AM_Combo`, o botão **"Create Default" LIGA as seções em cadeia** (você vê `Atk1 → Atk2 → Atk3 → Atk4` com setas numa linha só). Seção *linkada* = **auto-advance**: ao terminar Atk1, a montage continua sozinha pra Atk2. A janela de combo nem é consultada — quem manda é o link.

**✅ Fix (1 clique):** `AM_Combo` → painel **Montage Sections** → **[Clear]**. Cada seção vira uma linha **sem setas** = independente. Agora:
- Tocou `Atk1` sem input na janela → a seção roda **até o fim e a montage PARA** (blend out → volta pra locomoção).
- Encadear é **responsabilidade da ability** (`Montage Jump to Section`), **nunca** da montage.

> 🔑 **Regra (contrato):** *a montage não decide o combo — a ability decide.* Seções **sempre sem link**. Vale pra `AM_Combo`, `AM_Combo_Heavy`, `AM_AirCombo` ([16 §4](16_Aerial_Combos.md)) e qualquer montage seccionada futura.

---

## 1. O contrato do combo (como deve fluir)

```
IA_Attack ──→ GA_Attack_Light ativa ──→ PlayMontageAndWait("Atk1")
                                            │
   input ANTES da janela ──→ vai pro BUFFER (não faz nada ainda)
                                            │
   [ANS_ComboWindow BEGIN] ──→ tem buffer fresco (≤0.25s)? ──→ JumpToSection("Atk2")
                                │ não
                                └─→ input AO VIVO durante a janela → JumpToSection("Atk2")
                                            │
   [ANS_ComboWindow END] sem input ──→ seção toca até o fim
                                  ──→ montage para (SEM link!) → blend out
                                  ──→ OnBlendOut/OnCompleted → reset ComboIndex → EndAbility
```

Comportamentos garantidos:
- **Errou a janela do golpe 1** → roda o golpe 1 inteiro e **para nele**. ✅ (sua pergunta)
- **Apertou um tico cedo** → o buffer perdoa e encadeia quando a janela abre. ✅ (input buffer, [07 §5](../systems/07_Input.md))
- **Apertou DEPOIS da janela fechar** → ignorado no MVP (a seção termina; re-pressione pra começar combo novo). *Carregar buffer pro próximo ciclo = P1.*

---

## 2. AM_Combo — checklist de configuração no editor

| Item | Valor | Por quê |
|---|---|---|
| **Montage Sections → links** | **NENHUM (Clear!)** | ← o fix do §0. Sem setas entre seções |
| Seções | `Atk1`-`Atk4` no frame inicial de cada golpe | nomes = os usados pela ability |
| **Enable Auto Blend Out** | ✅ ON (default) | seção acaba → blend de volta pra locomoção |
| Slot | `DefaultGroup.DefaultSlot` | o AnimGraph precisa ter o nó Slot ([08 §4](../locomotion/08_Locomotion_Overview.md)) |
| `ANS_Hitbox` | 1 por seção, cobrindo o **swing** | janela de acerto ([15 §3](15_Combat_Overview.md)) |
| `ANS_ComboWindow` | 1 por seção (Atk1-3), ~**40-70%** da seção (recovery) | janela de encadear ([15 §3](15_Combat_Overview.md)) |
| `ANS_ComboWindow` no **Atk4** | **não** (é o finisher) | último golpe não encadeia em si; a janela do Atk4 pode existir só p/ **attack→launcher** ([15 §6](15_Combat_Overview.md)) |
| **`Motion Warping`** (engine) | **1 por seção** (Atk1–4), no startup/swing | ver §2b — sem isso o golpe encara mas não avança |

---

## 2b. Motion Warping — uma janela **por seção** (`Atk1`–`Atk4`)

O `AM_Combo` é seccionado: cada golpe é uma **seção independente** na mesma montage. O Motion Warp segue a mesma regra dos outros notifies (`ANS_Hitbox`, `ANS_ComboWindow`) — **um por seção**.

### Duas camadas (C++ × editor)

| Camada | Responsável | O que faz em cada golpe |
|---|---|---|
| **C++** | `UGA_AttackLight` → `FaceAndSetupMotionWarp` | Recalcula o alvo `AttackWarp` no Atk1 **e** a cada `AdvanceCombo` / `MontageJumpToSection` |
| **Montage** | Notify state **`Motion Warping`** na timeline da seção | A engine **aplica** o lunge só enquanto essa janela está ativa na seção que toca |

> 🔑 **Encarar ≠ alcançar.** O C++ sempre atualiza face + warp point no startup de cada golpe. Mas se **Atk2** não tiver notify na timeline, o personagem **vira** pro dummy e a esfera magenta aparece (`ddr.CombatDebug 1`) — só **não desliza** no swing daquele golpe.

### O que colocar em cada seção

Em **Atk1**, **Atk2**, **Atk3** e **Atk4** (mesma config em todas):

1. Notify state **`Motion Warping`** no **startup/swing**, **antes** do `ANS_DDRHitbox`.
2. **Root Motion Modifier** = `Skew Warp` (ou `Warp`).
3. **Warp Target Name** = `AttackWarp` · **Warp Translation** ✅ · **Ignore Z Axis** ✅.

Passo a passo completo + perfis por ability: [60 §7.3](../systems/60_M2_Editor_Setup.md).

### Ordem de setup (não precisa das 4 de uma vez)

1. Coloque warp **só em Atk1** → PIE: 1 golpe deve alcançar o dummy.
2. Encadeie até Atk2 → se Atk2 **olha mas não avança**, falta o notify na seção Atk2.
3. Replique pras seções restantes (copiar/colar o notify entre seções é ok).

---

## 3. Notifies → GameplayEvents (como a montage fala com a ability)

Os `ANS_*` são AnimNotifyState BP que enviam **GameplayEvent** ao Owner (`SendGameplayEventToActor`):

| Notify | NotifyBegin envia | NotifyEnd envia |
|---|---|---|
| `ANS_ComboWindow` | `Event.Combat.ComboWindow.Begin` | `Event.Combat.ComboWindow.End` |
| `ANS_Hitbox` | (liga o sweep) → hits enviam `Event.Combat.Hit` | (desliga o sweep) |

> Tags registradas no catálogo **[41 §7](../systems/41_Gameplay_Tags.md)**. A ability escuta com `WaitGameplayEvent` — desacoplado: a montage não conhece a ability.

---

## 4. BP_GA_AttackLight — o graph (M1, Blueprint)

**Class Defaults** ([19 §3](19_Abilities_Deep.md)): Instancing `InstancedPerActor` · **Retrigger Instanced Ability = false** (apertar de novo NÃO reinicia do Atk1 — o press vira input pra ability já ativa) · tags da ficha do 19.

**Variáveis:** `ComboIndex` (int, 1-4) · `bComboWindowOpen` (bool) · `bAttackBuffered` (bool) · `BufferedTime` (float).

**Fluxo do ActivateAbility:**

```
1. ComboIndex = 1
   PlayMontageAndWait(AM_Combo, StartSection="Atk1")
     OnCompleted / OnBlendOut / OnInterrupted / OnCancelled → ResetCombo() → EndAbility

2. [loop] WaitInputPress (bTestAlreadyPressed=true)
     → bAttackBuffered = true; BufferedTime = GameTimeInSeconds
     → se bComboWindowOpen → AdvanceCombo()
     → re-arma WaitInputPress (chama o task de novo)

3. WaitGameplayEvent(Event.Combat.ComboWindow.Begin, OnlyTriggerOnce=false)
     → bComboWindowOpen = true
     → se bAttackBuffered E (Now - BufferedTime) <= 0.25 → AdvanceCombo()

4. WaitGameplayEvent(Event.Combat.ComboWindow.End, OnlyTriggerOnce=false)
     → bComboWindowOpen = false; bAttackBuffered = false   // input tarde não atravessa

AdvanceCombo():
   ComboIndex++  (se > 4 → nada; deixa terminar)
   Montage Jump to Section("Atk{ComboIndex}")   // nó "Montage Jump to Section" do GA
   bAttackBuffered = false; bComboWindowOpen = false
```

> 🔑 **Por que `WaitInputPress`:** com `Retrigger=false`, o GAS **não** reativa a ability no 2º press — ele entrega o press à instância ativa. `WaitInputPress` é como o BP recebe isso ([05 §4](../systems/05_GAS_Architecture.md)). É o input buffer do [07 §5](../systems/07_Input.md) implementado dentro da ability (M1/BP); a versão polida em componente C++ é P1.

---

## 5. Números (canônicos)

| Knob | Valor | Fonte |
|---|---|---|
| Frescor do buffer | **0.25 s** (faixa 0.2-0.35) | [07 §5](../systems/07_Input.md), [20 §8](../feel/20_Game_Feel.md) |
| Janela de combo na seção | ~40-70% (recovery) | [15 §3](15_Combat_Overview.md) |
| Seções | 4 (Atk1-4) | [19 §3](19_Abilities_Deep.md) |

---

## 6. Como testar (QA manual do M1)

| Teste | Esperado |
|---|---|
| Apertar **1×** | toca **só Atk1** e volta pra locomoção ✅ |
| **Mash** (apertar rápido várias vezes) | encadeia Atk1→2→3→4 liso (buffer pega os presses cedo) |
| Apertar **um tico antes** da janela | encadeia assim que a janela abre (buffer) |
| Apertar **dentro** da janela | encadeia imediato |
| Apertar **depois** da janela fechar | NÃO encadeia; seção termina e para |
| Dash no meio de qualquer golpe | corta o golpe (dash-cancel, [15 §6](15_Combat_Overview.md)) → `OnInterrupted` reseta o combo |
| Combo encadeia mas **só Atk1 alcança** o dummy | Motion Warp só na 1ª seção | §2b — replique o notify em Atk2/3/4 |

---

## 7. Checklist

- [ ] **Montage Sections → Clear** (zero links entre seções) ← o fix
- [ ] Auto Blend Out ON; slot DefaultSlot (e nó Slot no ABP)
- [ ] `ANS_Hitbox` + `ANS_ComboWindow` por seção (janela ~40-70%)
- [ ] Atk4 sem ComboWindow de encadear (só gancho p/ launcher)
- [ ] `Retrigger Instanced Ability = false`
- [ ] `WaitInputPress` em loop + buffer 0.25s
- [ ] `WaitGameplayEvent` Begin/End da janela
- [ ] `AdvanceCombo` usa **Montage Jump to Section** (nunca link de montage)
- [ ] Notify **Motion Warping** (`AttackWarp`, Skew Warp, Ignore Z) no startup de cada seção — [60 §7.3](../systems/60_M2_Editor_Setup.md)
- [ ] Todos os 6 testes do §6 passam

---

## 8. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| **Toca todas as seções com 1 press** | **Seções LINKADAS** (Create Default) | §0 — Montage Sections → **Clear** |
| Apertar de novo reinicia do Atk1 | `Retrigger = true` | §4 — false; press vai pro `WaitInputPress` |
| 2º press não faz nada | `WaitInputPress` não re-armado após disparar | §4 passo 2 — loop |
| Combo "engasga" (só pega press perfeito) | sem buffer (só checa na janela) | §4 passo 2/3 — flag + frescor 0.25s |
| Seção não para no fim (desliza pro próximo frame) | ainda há link residual entre seções | §0 — confira: cada seção numa linha, sem setas |
| Montage não toca no jogo (só no preview) | ABP sem o nó `Slot 'DefaultSlot'` | [08 §4](../locomotion/08_Locomotion_Overview.md) |
| Combo não reseta após dash | faltou tratar `OnInterrupted/OnCancelled` | §4 passo 1 |
| **Bate no vazio** perto do dummy | falta motion warp na montage | [60 §7.3](../systems/60_M2_Editor_Setup.md) — target `AttackWarp`; soft-lock já é automático no C++ |
| **Atk1 ok, Atk2+ no vazio** (olha pro alvo) | warp só em Atk1 | §2b — **uma janela por seção** |

---

## 9. Próximo passo

→ Hit detection + `Event.Combat.Hit` → dano ([15 §4](15_Combat_Overview.md), [18 §2-3](18_Combat_System_Deep.md)) · hit-stop global ([21 §3](../feel/21_Juice_FX.md)) · **soft-lock + motion warp** ([60 §7](../systems/60_M2_Editor_Setup.md)) · ficha da ability: [19 §3](19_Abilities_Deep.md).
