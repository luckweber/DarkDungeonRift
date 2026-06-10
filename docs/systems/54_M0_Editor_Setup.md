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
| Camera Lag | ON (speed 10) | feel |

Não ligue **Use Pawn Control Rotation** no boom.

### 3.6 ⚠️ Dash no M0 = placeholder (CMC, não GAS)

O dash que você testa no M0 vive em **`UDDRCharacterMovementComponent::TryDash()`** — velocidade fixa + modo `Flying` por ~0.22s. Serve só pra validar **input, direção e feel** numa arena vazia.

| M0 (agora) | M1 (alvo) |
|---|---|
| `TryDash()` no CMC | **`GA_Dash`** ([19 §3](../combat/19_Abilities_Deep.md)) |
| Cooldown float no CMC | `GE_Cooldown_Dash` + tag `Cooldown.Dash` |
| Sem i-frames | `State.Movement.Dashing` + perfect-dodge ([51](../combat/51_Defensive_Combat.md)) |
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
| Console `ddr.LocomotionDebug 1` | texto cyan acima do player (Gait, Speed, Dash CD) |

**✅ M0 pronto quando:** mover, sprintar, dashar e pular numa arena vazia **sente responsivo** ([17 §2](../17_Implementation_Roadmap.md)).

---

## 7. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Não compila | Plugin GAS off / Build.cs | [04 §1-2](04_Project_Setup.md) |
| Personagem não aparece | GameMode pawn errado | §5 → `BP_DDRPlayer` |
| Input morto | IMC não assignado | §3.4 slots vazios |
| Move "torto" vs tela | Move usa controller yaw | C++ usa yaw do **CameraBoom** — recompile |
| Dash não faz nada | Cooldown / sem direção | anda antes ou dash usa forward |
| Dash atravessa parede | CMC M0 não faz sweep | esperado no M0; P1 = capsule trace ([19 §3](../combat/19_Abilities_Deep.md)) |
| Warning `DefaultMappingContext not assigned` | BP sem IMC | §3.4 |
| Template ThirdPerson BP quebrado | redirect antigo | Use `BP_DDRPlayer`, não `BP_ThirdPersonCharacter` |

---

## 8. Próximo passo (M1)

→ [05 — GAS](05_GAS_Architecture.md): `GA_Attack_Light`, hit detection, hit-stop · [15 Combate](../combat/15_Combat_Overview.md) · [19 Abilities](../combat/19_Abilities_Deep.md).

Assets de editor no M1:
- `IA_Attack`, montages `AM_Combo_*`
- `BP_TrainingDummy` com ASC
- Gameplay Effects `GE_Damage`

---

## 9. Mapa C++ ↔ Editor

| Responsabilidade | C++ | Editor |
|---|---|---|
| Câmera topdown | `ADDRPlayerCharacter` | tune arm length (opcional) |
| Move relativo câmera | `Move()` | `IA_Move` + IMC |
| Sprint / gaits | `UDDRCharacterMovementComponent` | `IA_Sprint` |
| Dash (M0 placeholder) | `TryDash()` no CMC | `IA_Dash` → **`GA_Dash` no M1** ([19 §3](../combat/19_Abilities_Deep.md)) |
| GAS skeleton | `ADDRCharacterBase` + AttributeSet | GE/GA assets no M1 |
| Tags | `Config/DefaultGameplayTags.ini` + `DDRTags::*` | Gameplay Tag Editor (sync) |
| Debug | `ddr.LocomotionDebug` | console ` |
