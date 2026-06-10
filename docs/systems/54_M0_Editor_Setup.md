# 54 — M0 Editor Setup (passo a passo) · 🟢 P0

> **O que fazer no Unreal Editor** depois de compilar o C++ do milestone **M0 — Esqueleto**. O código já entrega: GAS base, CMC com gaits/sprint/dash, câmera topdown, input Enhanced Input. **Você** cria os assets de Input, Blueprint do player, mapa de teste e AnimBP mínimo.

> **Pré-req C++:** compile o projeto ([04 §2](04_Project_Setup.md)). Classes criadas:
> - `ADDRGameMode` · `ADDRPlayerController` · `ADDRPlayerCharacter`
> - `ADDRCharacterBase` · `UDDRCharacterMovementComponent`
> - `UDDRAbilitySystemComponent` · `UDDRAttributeSet`

> **Docs relacionados:** [06 Câmera](06_Camera_TopDown.md) · [07 Input](07_Input.md) · [08 Locomoção](../locomotion/08_Locomotion_Overview.md) · [09 Gaits](../locomotion/09_Gaits.md) · [17 Roadmap M0](../17_Implementation_Roadmap.md)

---

## 0. Checklist rápido

- [ ] Projeto compila (Visual Studio → Build)
- [ ] Editor abre sem erro de módulo
- [ ] Pastas Content criadas (§1)
- [ ] Input Actions + IMC (§2)
- [ ] `BP_DDRPlayer` (§3)
- [ ] Mapa `L_DDR_M0_Test` (§4)
- [ ] GameMode aponta pro BP (§5)
- [ ] PIE: mover, sprint, dash, pulo (§6)

---

## 1. Criar pastas Content

No **Content Browser** → botão direito → **New Folder**:

```
Content/DarkDungeonRift/
├── Characters/Player/
├── Input/
├── Maps/
└── Animation/
```

> Mantenha tudo sob `DarkDungeonRift/` ([04 §3.2](04_Project_Setup.md)).

---

## 2. Enhanced Input (IA_* + IMC)

### 2.1 Input Actions

Em `Content/DarkDungeonRift/Input/` → **Input → Input Action**:

| Asset | Nome | Value Type | Notas |
|---|---|---|---|
| `IA_Move` | IA_Move | **Axis2D (Vector2D)** | WASD / analógico esquerdo |
| `IA_Jump` | IA_Jump | Digital (bool) | Space |
| `IA_Sprint` | IA_Sprint | Digital (bool) | Shift (hold) |
| `IA_Dash` | IA_Dash | Digital (bool) | Ctrl / B no gamepad |

> Combate (`IA_Attack`, `IA_Launcher`…) fica pro **M1** — não precisa agora.

### 2.2 Input Mapping Context

Crie **`IMC_DDR_Default`** (Input Mapping Context):

| Action | Tecla (MVP) | Modificadores |
|---|---|---|
| IA_Move | W, A, S, D | opcional: **Swizzle Input Axis** (YXZ) se eixo invertido |
| IA_Move | Gamepad Left Stick 2D | — |
| IA_Jump | Space | — |
| IA_Jump | Gamepad Face Button Bottom (A) | — |
| IA_Sprint | Left Shift | — |
| IA_Sprint | Gamepad Left Shoulder (LB) | — |
| IA_Dash | Left Ctrl | — |
| IA_Dash | Gamepad Face Button Right (B) | — |

Mapa canônico completo: [39 — Controles](39_Controls.md).

### 2.3 O que o C++ espera

No `BP_DDRPlayer` (§3), assign nos slots do `ADDRPlayerCharacter`:

- **Default Mapping Context** → `IMC_DDR_Default`
- **Move Action** → `IA_Move`
- **Jump Action** → `IA_Jump`
- **Sprint Action** → `IA_Sprint`
- **Dash Action** → `IA_Dash`

Se algum slot ficar vazio, o log `LogDDR` avisa no **Output Log**.

---

## 3. Blueprint do Player — `BP_DDRPlayer`

### 3.1 Criar

1. `Content/DarkDungeonRift/Characters/Player/`
2. **Blueprint Class** → parent: **`DDRPlayerCharacter`** (C++)
3. Nome: **`BP_DDRPlayer`**

### 3.2 Mesh & animação (placeholder)

1. Abra `BP_DDRPlayer`
2. **Mesh** component → Skeletal Mesh: use o Mannequin do template se tiver localmente:
   - `/Game/Characters/Mannequins/Meshes/SKM_Manny` (ou equivalente no seu projeto)
3. **Anim Class:** crie um AnimBP mínimo (§3.3) ou deixe o do Third Person temporariamente

### 3.3 AnimBP mínimo (opcional no M0, recomendado)

1. Crie **`ABP_DDRPlayer`** (parent: `AnimInstance`)
2. No **Event Graph** → **Try Get Pawn Owner** → cast to `DDRPlayerCharacter`
3. Leia `GetDDRMovement()` → `GetLocomotionState()`:
   - `Speed`, `Gait`, `bIsFalling` → alimente um **Blend Space 2D** simples (Direction × Speed) ou state machine Idle/Move/Jump
4. Assign em `BP_DDRPlayer` → Mesh → Anim Class = `ABP_DDRPlayer`

> Locomoção polida (distance match, air SM) = [08](../locomotion/08_Locomotion_Overview.md) · [13](../locomotion/13_Jump_Fall_Landing.md). No M0, **responder** importa mais que anim perfeita.
>
> 🛠️ **Passo a passo COMPLETO do AnimGraph** (vars←CMC, Blend Space, State Machine, transições exatas e o **Slot que o combo do M1 exige**): **[58 — AnimGraph Step by Step](../locomotion/58_AnimGraph_Step_by_Step.md)**.

### 3.4 Input (Class Defaults)

No **Class Defaults** de `BP_DDRPlayer`:

| Property | Asset |
|---|---|
| Default Mapping Context | `IMC_DDR_Default` |
| Move Action | `IA_Move` |
| Jump Action | `IA_Jump` |
| Sprint Action | `IA_Sprint` |
| Dash Action | `IA_Dash` |

### 3.5 Câmera (já no C++, só tune)

Valores default do C++ ([06 §2](06_Camera_TopDown.md)):

| Property | Default C++ | Tune se quiser |
|---|---|---|
| CameraBoom → Target Arm Length | 950 | 800–1100 |
| CameraBoom → Rotation | (-55, -45, 0) | pitch/yaw isométrico |
| CameraBoom → **Absolute Rotation** | **ON** (C++) | **não desligue no BP** — evita spin no lugar |
| Camera Lag | ON (speed 10) | feel |

Não ligue **Use Pawn Control Rotation** no boom. Com **orient-to-movement**, o boom precisa de **Absolute Rotation = true**; senão o WASD usa um yaw que gira com o personagem e ele fica rodando no spot ([06 §3](06_Camera_TopDown.md)).

### 3.6 ✅ Dash = `GA_Dash` (GAS) — placeholder do CMC REMOVIDO

> **Histórico:** no M0 existia um `TryDash()` placeholder no CMC (velocidade fixa + `MOVE_Flying`). Como planejado, ele foi **removido no M1** — o dash agora é **só** `GA_Dash` (RootMotion + i-frames + cooldown, [19 §3](../combat/19_Abilities_Deep.md)) e o `bIsDashing` do snapshot vem da **tag `State.Movement.Dashing`**. Uma fonte da verdade. Se algum BP antigo chamar `TryDash`, ele não existe mais — troque por `AbilityLocalInputPressed(Dash)`.

| M0 (agora) | M1 (alvo) |
|---|---|
| `TryDash()` no CMC | **`GA_Dash`** ([19 §3](../combat/19_Abilities_Deep.md)) |
| Cooldown float no CMC | `GE_Cooldown_Dash` + tag `Cooldown.Dash` |
| Sem i-frames | `State.Movement.Dashing` + perfect-dodge ([56](../combat/56_Defensive_Combat.md)) |
| Sem sweep de parede | Capsule trace opcional (P1) para clamp de distância |

> 🗑️ **Não invista** em polish do dash do CMC (montage, VFX, anti-wall). No M1, **remova** a lógica de dash do CMC e deixe só o `GA_Dash` — evita duas fontes da verdade.

---

## 4. Mapa de teste — `L_DDR_M0_Test`

1. **File → New Level → Empty Level**
2. Salve em `Content/DarkDungeonRift/Maps/L_DDR_M0_Test`
3. Adicione:
   - **Directional Light**
   - **Sky Atmosphere** + **Sky Light** (ou Lighting Preset básico)
   - **Player Start**
   - **Plane** ou **Cube** escalado (chão 30×30 m) — chão **plano** ([52 §1](../world/52_Arena_Level_Design.md))
4. **Window → World Settings:**
   - **GameMode Override:** `BP_DDRGameMode` (§5) ou `DDRGameMode` C++
5. **Build → Build Paths** (NavMesh) — opcional no M0 puro, necessário no M3

---

## 5. GameMode Blueprint — `BP_DDRGameMode`

1. Crie **Blueprint Class** → parent **`DDRGameMode`**
2. Nome: `BP_DDRGameMode`
3. **Class Defaults:**
   - **Default Pawn Class** → `BP_DDRPlayer`
   - **Player Controller Class** → `DDRPlayerController` (C++)

### Project Settings (alternativa global)

**Edit → Project Settings → Maps & Modes:**

| Campo | Valor |
|---|---|
| Default GameMode | `BP_DDRGameMode` |
| Editor Startup Map | `L_DDR_M0_Test` |
| Game Default Map | `L_DDR_M0_Test` |

> `Config/DefaultEngine.ini` já aponta `DDRGameMode` C++; o BP override é mais conveniente pra iterar.

---

## 6. Testar (PIE)

1. Abra `L_DDR_M0_Test`
2. **Play (PIE)**
3. Verifique:

| Ação | Esperado |
|---|---|
| WASD | move **relativo à câmera** (não ao personagem) |
| Shift (hold) | sprint (~750 u/s) |
| Ctrl | dash (~550 u/s, ~0.22s, cooldown ~0.6s) |
| Space | pulo |
| Console `ddr.LocomotionDebug 1` | texto cyan acima do player (Gait, Speed, Sprint, Dash, Fall) |

**✅ M0 pronto quando:** mover, sprintar, dashar e pular numa arena vazia **sente responsivo** ([17 §2](../17_Implementation_Roadmap.md)).

---

## 7. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Não compila | Plugin GAS off / Build.cs | [04 §1-2](04_Project_Setup.md) |
| Personagem não aparece | GameMode pawn errado | §5 → `BP_DDRPlayer` |
| Input morto | IMC não assignado | §3.4 slots vazios |
| Move "torto" vs tela | Move usa controller yaw | C++ usa yaw do **CameraBoom** — recompile |
| **Gira em círculo sem sair do lugar** | Boom gira com pawn; `Move()` + orient-to-movement em loop | `SetUsingAbsoluteRotation(true)` no boom ([06 §3](06_Camera_TopDown.md)); não override no BP |
| Dash não faz nada | Cooldown / sem direção | anda antes ou dash usa forward |
| Dash atravessa parede | (resolvido no M1) | `GA_Dash` usa RootMotion com colisão — paredes bloqueiam ✅ |
| Warning `DefaultMappingContext not assigned` | BP sem IMC | §3.4 |
| Template ThirdPerson BP quebrado | redirect antigo | Use `BP_DDRPlayer`, não `BP_ThirdPersonCharacter` |

---

## 8. Próximo passo (M1)

→ **[55 — M1 Editor Setup](55_M1_Editor_Setup.md)** (passo a passo: `IA_Attack`, `AM_Combo` + notifies, dummy) · [05 GAS](05_GAS_Architecture.md) · [15 Combate](../combat/15_Combat_Overview.md).

---

## 9. Mapa C++ ↔ Editor

| Responsabilidade | C++ | Editor |
|---|---|---|
| Câmera topdown | `ADDRPlayerCharacter` | tune arm length (opcional) |
| Move relativo câmera | `Move()` | `IA_Move` + IMC |
| Sprint / gaits | `UDDRCharacterMovementComponent` | `IA_Sprint` |
| Dash | **`GA_Dash`** (GAS; placeholder do CMC removido no M1) | `IA_Dash` ([19 §3](../combat/19_Abilities_Deep.md)) |
| GAS skeleton | `ADDRCharacterBase` + AttributeSet | GE/GA assets no M1 |
| Tags | `Config/DefaultGameplayTags.ini` + `DDRTags::*` | Gameplay Tag Editor (sync) |
| Debug | `ddr.LocomotionDebug` | console ` |
