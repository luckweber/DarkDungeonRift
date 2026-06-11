# 55 — M1 Editor Setup (passo a passo) · 🟢 P0

> **O que fazer no Unreal Editor** depois de compilar o C++ do milestone **M1 — Bater**. O código entrega GAS ligado, `GA_Attack_Light`, `GA_Dash`, hit detection, hit-stop e boneco de treino. **Você** cria `IA_Attack`, montage de combo com notifies, assigna assets no BP e coloca o dummy na arena.

> **Pré-req:** M0 completo ([54 — M0 Editor Setup](54_M0_Editor_Setup.md)) e projeto compilado com as classes M1.

> **Classes C++ novas:**
> - `UDDRGameplayAbility` · `UGA_AttackLight` · `UGA_Dash`
> - `UGE_DDRDamage` · `UGE_DDRCooldownDash` · `UGE_DDRDashIFrames`
> - `UDDRCombatComponent` · `UDDRHitStopSubsystem`
> - `UANS_DDRHitbox` · `UANS_DDRComboWindow`
> - `ADDRTrainingDummy`

> **Docs:** [05 GAS](05_GAS_Architecture.md) · [07 Input](07_Input.md) · [15 Combate](../combat/15_Combat_Overview.md) · [19 Abilities](../combat/19_Abilities_Deep.md) · [17 Roadmap M1](../17_Implementation_Roadmap.md)

---

## 0. Checklist rápido

- [ ] Projeto compila com M1 C++
- [ ] `IA_Attack` + binding no IMC (§2)
- [ ] `BP_DDRPlayer`: `Attack Action` assignado (§3)
- [ ] `AM_Combo` com seções Atk1–Atk4 + notifies (§4)
- [ ] `GA_Attack_Light` → `Combo Montage` no BP ou CDO (§3)
- [ ] **Espada:** socket `weapon_r` no skeleton + SM no `WeaponMesh` + sockets `weapon_start`/`end` na SM (§3.4)
- [ ] `BP_TrainingDummy` na arena (§5)
- [ ] PIE: combo 3 golpes + hit-stop + dash-cancel (§6)
- [ ] Soft-lock + motion warp (`AttackWarp` em `AM_Combo`) — [60 §7](60_M2_Editor_Setup.md) *(C++ pronto; janelas no editor)*

---

## 1. Input — `IA_Attack`

Em `Content/DarkDungeonRift/Input/`:

| Asset | Value Type | Tecla MVP |
|---|---|---|
| `IA_Attack` | Digital (bool) | **LMB** / **X** (gamepad) |

Abra **`IMC_DDR_Default`** e adicione:

| Action | Key | Trigger |
|---|---|---|
| `IA_Attack` | Left Mouse Button | **Started** |

> O dash já usa GAS (`GA_Dash`) — `IA_Dash` continua em **Started** → `AbilityLocalInputPressed(Dash)`.

---

## 2. `BP_DDRPlayer` — slots de input

Abra `Content/DarkDungeonRift/Characters/Player/BP_DDRPlayer`:

| Slot (Class Defaults) | Asset |
|---|---|
| Attack Action | `IA_Attack` |
| Dash Action | `IA_Dash` (já do M0) |
| Default Mapping Context | `IMC_DDR_Default` |

**Startup Abilities** (já default no C++ — confira no Details):

| Ability | Input ID |
|---|---|
| `GA_AttackLight` | Attack |
| `GA_Dash` | Dash |

> Se precisar trocar montage por BP: crie `BP_GA_AttackLight` (parent `GA_AttackLight`) e substitua na lista Startup Abilities.

---

## 3. Montage de combo — `AM_Combo`

### 3.1 Criar a montage

1. `Content/DarkDungeonRift/Animation/` → importe ou duplique animações de ataque do seu set (ex.: Sword & Shield do projeto).
2. Crie **`AM_Combo`** (Anim Montage).
3. Adicione **4 seções** na timeline:

| Seção | Nome exato (C++ default) |
|---|---|
| 1 | `Atk1` |
| 2 | `Atk2` |
| 3 | `Atk3` |
| 4 | `Atk4` |

> Os nomes devem bater com `ComboSections` em `GA_AttackLight` (default: Atk1–Atk4).

4. ⚠️ **PASSO CRÍTICO — painel Montage Sections → [Clear]:** o "Create Default" **linka as seções em cadeia** (`Atk1 → Atk2 → …` com setas) = auto-advance: 1 press toca o combo INTEIRO, ignorando a janela. Clique **Clear** pra cada seção ficar independente (sem setas). O avanço é da ability (`MontageJumpToSection`), nunca da montage. Bug completo + graph do buffer: **[57 — M1: Combo no Editor](../combat/57_M1_Combo_Editor_Setup.md)**.

### 3.2 Notifies por seção

Em **cada** seção, adicione dois **Anim Notify State**:

| Notify | Classe C++ | Onde na anim |
|---|---|---|
| Hitbox | **`ANS_DDRHitbox`** | no swing (janela ativa) |
| Combo Window | **`ANS_DDRComboWindow`** | recovery, antes do fim da seção |

Tune no `ANS_DDRHitbox` (Details):

| Campo | Valor inicial |
|---|---|
| Base Damage | 12 / 14 / 16 / 20 (Atk1→4) |
| Hit Stop Frames | 2–3 |
| Sweep Radius | 45 |
| Sweep Forward Offset | 80 |
| Sweep Reach | 120 |

### 3.3 Assignar no player

Opção A — **BP filho de GA_AttackLight:**

1. Content Browser → Blueprint Class → parent **`GA_AttackLight`** → `BP_GA_AttackLight`.
2. Class Defaults → **Combo Montage** = `AM_Combo`.
3. Em `BP_DDRPlayer` → Startup Abilities → troque ability por `BP_GA_AttackLight`.

Opção B — editar CDO de `GA_AttackLight` no editor (menos recomendado).

### 3.4 Espada no player (`WeaponMesh`) — novo no C++

> As anims do combo são **com espada** — o C++ agora entrega um **`WeaponMesh`** (StaticMeshComponent) em `ADDRCharacterBase`, anexado ao socket **`weapon_r`** do skeleton (nome editável: `WeaponSocketName`), **sem colisão** (o hit é o sweep, nunca a mesh — [15 §4](../combat/15_Combat_Overview.md)). E o sweep agora é **na lâmina**: usa os sockets `weapon_start`/`weapon_end` da Static Mesh da arma ([18 §2](../combat/18_Combat_System_Deep.md)); sem eles, **fallback** pro sweep frontal antigo (nada quebra).

**Passos no editor:**

1. **Socket no skeleton:** abra o skeleton do player (`SK_Mannequin`) → Skeleton Tree → bone **`hand_r`** → botão direito → **Add Socket** → renomeie **`weapon_r`**. Botão direito no socket → **Add Preview Asset** (a SM da espada) → ajuste posição/rotação até a empunhadura casar com a mão (use uma pose de ataque no preview).
2. **Sockets na espada:** abra a **Static Mesh** da espada → `Window → Socket Manager` → **Create Socket** ×2:
   | Socket | Onde |
   |---|---|
   | `weapon_start` | base da lâmina (junto à guarda) |
   | `weapon_end` | ponta da lâmina |
3. **`BP_DDRPlayer`:** selecione o componente herdado **`WeaponMesh`** → **Static Mesh = sua espada** (SM do set Sword & Shield).
4. Com a lâmina, **reduza `Sweep Radius` p/ ~30** nos `ANS_DDRHitbox` (45 era pro sweep frontal gordo; a lâmina + 45 fica exagerado).

> ⚠️ Se a arma do pack for **Skeletal Mesh** (não Static), converta: abra-a → `Make Static Mesh`, ou use a versão SM do pack. O componente é `UStaticMeshComponent`.

> 🐞 **`ddr.CombatDebug 1` agora DESENHA** (antes só logava — corrigido): **linha + 2 esferas** do volume do sweep (**verde** = sem hit, **vermelho** = hit) + **esfera amarela** no ponto de impacto. Se os sockets da arma existem, você vê o sweep **na lâmina**; senão, o frontal (fallback).

---

## 4. Dash GAS (substitui CMC M0)

O dash agora é **`GA_Dash`** (RootMotionSource + i-frames + cooldown).

| Comportamento | Detalhe |
|---|---|
| Cancela ataque | `CancelAbilitiesWithTag: Ability.Attack` |
| I-frames | `GE_DDRDashIFrames` (~0.25s, tag `State.Movement.Dashing`) |
| Cooldown | `GE_DDRCooldownDash` (~0.6s, tag `Cooldown.Dash`) |
| Movimento | `UAbilityTask_ApplyRootMotionConstantForce` |

> O placeholder `TryDash()` no CMC **não é mais chamado** pelo player. Pode ignorar no M1.

✅ **Montage direcional suportada:** o `GA_Dash` agora toca **`AM_Dodge` com 8 seções** (F/FR/R/BR/B/BL/L/FL) escolhidas pelo ângulo facing→direção, **mantendo o facing** (dash-cancel de combate). Passo a passo completo (montage, Clear, BP_GA_Dash, root motion dos clips): **[59 — Dodge Direcional](../combat/59_Directional_Dodge.md)**.

---

## 5. Boneco de treino

### 5.1 Blueprint

1. Blueprint Class → parent **`DDRTrainingDummy`** → `BP_TrainingDummy`.
2. Assigne **Static Mesh** no componente `DummyMesh` (cubo/engine mesh serve).
3. Escala ~ (1,1,2) para ficar do tamanho do player.

### 5.2 Colocar na arena

1. Abra `L_DDR_M0_Test` (ou duplique → `L_DDR_M1_Combat`).
2. Arraste `BP_TrainingDummy` para perto do spawn.
3. Rotacione de frente pro player.

O dummy tem ASC, 5000 HP, tag `Faction.Enemy` — recebe dano do sweep.

---

## 6. Teste PIE (critério M1)

| Teste | Esperado |
|---|---|
| LMB | inicia combo (`State.Combat.Attacking`) |
| LMB na janela de combo | encadeia Atk2, Atk3… |
| Acerto no dummy | hit-stop global (~2 frames), log **+ draw** se `ddr.CombatDebug 1` |
| `ddr.CombatDebug 1` | **desenha o sweep**: linha + 2 esferas (verde=sem hit, vermelho=hit) + esfera amarela no impacto; na lâmina se os sockets existem (§3.4) |
| Dash durante ataque | **cancela** combo imediatamente, desloca com i-frames |
| Dash spam | bloqueado por `Cooldown.Dash` ~0.6s |

Console útil:

```
ddr.CombatDebug 1
ddr.LocomotionDebug 1
```

---

## 7. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| **Editor crasha ao abrir** (stack em `GE_DDRCooldownDash` / `FindOrAddComponent`) | `FindOrAddComponent` no **construtor** do GE (NewObject no CDO) | Recompile — tags vão em `PostInitProperties` ([05 §5](05_GAS_Architecture.md)) |
| **Crash ao apertar ataque** (breakpoint em `AddDynamic`) | Callbacks de ability task sem `UFUNCTION()` no C++ | Recompile M1 — `OnMontageEnded`, `OnMontageCancelled`, `OnHitEvent` precisam de `UFUNCTION()` |
| Attack não faz nada | `IA_Attack` não assignado / não no IMC | §1–2 |
| Ability não ativa | ASC não init | recompile; confira PIE com `BP_DDRGameMode` |
| Sem dano no dummy | Dummy sem ASC / fora do sweep | §5; aumente `Sweep Reach` no notify |
| Combo não encadeia | Falta `ANS_DDRComboWindow` | §3.2 |
| Hit sem peso | Hit-stop desligado | verifique `Hit Stop Frames` > 0 no notify |
| **Hit-stop muito lento** (quase trava o jogo) | bug: timer seguia `GlobalTimeDilation` | recompile M1; tune `Hit Stop Frames` no notify `ANS_DDRHitbox` (default **2**, tente 1–3) |
| Dash não cancela combo | `GA_Dash` sem tag `Ability.Attack` em Cancel | recompile C++ M1 |
| Warning GameplayCue | Cue BP não criada ainda | opcional M1 — crie `GC_Hit_Light` depois ([21](../feel/21_Juice_FX.md)) |
| **Espada não aparece** | SM não setada no `WeaponMesh` / socket `weapon_r` não existe no skeleton | §3.4 passos 1 e 3 |
| Espada na posição errada | transform do socket não ajustado | §3.4 passo 1 — Add Preview Asset + ajustar |
| **`ddr.CombatDebug 1` não desenha** | build antiga (o cvar só logava) | recompile o C++ — agora **1 = log + draw** |
| Sweep não sai da lâmina (sai do peito) | sockets `weapon_start`/`end` ausentes na SM | §3.4 passo 2 (sem eles o fallback frontal é o esperado) |
| **Bate no vazio** perto do dummy | falta motion warp / `AttackWarp` errado | [60 §7](60_M2_Editor_Setup.md) — soft-lock já vem do C++; tune `SoftLockRadius` no Combat Component |
| Personagem não olha pro dummy | soft-lock fora do cone | tune `SoftLockHalfAngleDegrees` / `SoftLockRadius` no BP; `ddr.CombatDebug 1` (linha ciano) |

---

## 8. Próximo passo (M2)

→ [60 — M2 Editor Setup](60_M2_Editor_Setup.md): launcher/juggle/slam, sombra blob, **soft-lock + Motion Warping (§7)** — o **pilar** do jogo. Spec: [16 — Combos Aéreos](../combat/16_Aerial_Combos.md).

---

## 9. Mapa C++ ↔ Editor

| Feature | C++ | Editor |
|---|---|---|
| Input → GAS | `AbilityLocalInputPressed` | `IA_Attack` + slot no BP |
| Combo | `UGA_AttackLight` | `AM_Combo` + seções |
| Hitbox | `ANS_DDRHitbox` → `UDDRCombatComponent` | notifies na montage |
| Combo window | `ANS_DDRComboWindow` | notify na montage |
| Dano | `UGE_DDRDamage` | automático |
| Hit-stop | `UDDRHitStopSubsystem` | `Hit Stop Frames` no notify |
| Dash | `UGA_Dash` | `IA_Dash` (já do M0) |
| Alvo | `ADDRTrainingDummy` | `BP_TrainingDummy` no mapa |
| **Espada** | `WeaponMesh` em `ADDRCharacterBase` (socket `weapon_r`) | socket no SK + SM no BP + sockets `weapon_start`/`end` na SM (§3.4) |
| **Sweep da lâmina** | `UDDRCombatComponent` (`WeaponTraceSocketStart/End`, fallback frontal) | automático com os sockets da SM |
| Debug visual | cvar `ddr.CombatDebug` (**1 = log + draw**) | console no PIE |
| **Soft-lock** | `FindSoftLockTarget` / `FaceSoftLockTarget` | tune Combat Component no BP |
| **Motion warp** | `FaceAndSetupMotionWarp` (ground) | notify `Motion Warping` → `AttackWarp` em `AM_Combo` — [60 §7](60_M2_Editor_Setup.md) |
