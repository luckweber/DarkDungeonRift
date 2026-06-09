# 04 — Setup do Projeto · 🟢 P0

> Plugins, módulos de build, e organização de pastas para um ARPG topdown roguelike com GAS no **UE 5.4**. Faça isto **antes** de qualquer gameplay — é o alicerce.

> Estado atual do projeto: template Third Person. `Build.cs` só tem `Core, CoreUObject, Engine, InputCore, EnhancedInput`. Sem GAS, sem plugins de warping. Vamos corrigir.

---

## 1. Plugins a habilitar (`.uproject` ou Editor → Plugins)

| Plugin | P0/P1 | Para quê | Obrigatório? |
|---|:---:|---|:---:|
| **GameplayAbilities** | 🟢 P0 | GAS — combate, stats, habilidades, morte | **Sim** |
| **EnhancedInput** | 🟢 P0 | Input moderno (já vem ligado no template) | **Sim** |
| **AnimationLocomotionLibrary** | 🟢 P0 | Distance Matching, Foot Placement, Speed Warping (nós de AnimGraph) | Sim |
| **AnimationWarping** | 🟡 P1 | Stride / Orientation / Slope warping | Recomendado |
| **MotionWarping** | 🟢 P0 | Warp de ataque (alinhar golpe ao alvo) + traversal | **Sim p/ combate** |
| **MotionTrajectory** | 🟡 P1 | Trajetória p/ distance match & (futuro) Motion Matching | Recomendado |
| **Chooser** | 🟡 P1 | Seleção data-driven de anim/montage | Opcional |
| **GameplayCameras** | 🟡 P1 | Rigs de câmera avançados | Opcional (topdown simples não precisa no MVP) |
| **CommonUI** | 🔵 P2 | UI escalável | Pós-MVP |
| **PoseSearch** (Motion Matching) | 🔵 P2 | Locomoção data-driven (spike) | Só se for testar MM (doc 08) |

> ⚠️ **Não habilite tudo "por garantia".** Cada plugin é tempo de compile e superfície de bug. Ligue **GameplayAbilities + AnimationLocomotionLibrary + MotionWarping** agora; o resto quando o doc correspondente pedir.

### Como habilitar via `.uproject`

Adicione ao array `"Plugins"` do [DarkDungeonRift.uproject](../../DarkDungeonRift.uproject):

```json
{ "Name": "GameplayAbilities", "Enabled": true },
{ "Name": "AnimationLocomotionLibrary", "Enabled": true },
{ "Name": "MotionWarping", "Enabled": true },
{ "Name": "AnimationWarping", "Enabled": true },
{ "Name": "MotionTrajectory", "Enabled": true }
```

---

## 2. Módulos no `Build.cs`

Edite [DarkDungeonRift.Build.cs](../../Source/DarkDungeonRift/DarkDungeonRift.Build.cs):

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore",
    "EnhancedInput",
    "GameplayAbilities", "GameplayTags", "GameplayTasks",   // GAS (os 3 andam juntos)
    "MotionWarping",                                         // warp de ataque
    "AnimationLocomotionLibrary"                             // distance match / foot placement (se usar em C++)
});

// Módulos de editor (opcional, p/ ferramentas):
// PrivateDependencyModuleNames.AddRange(new string[] { "GameplayAbilitiesEditor" });
```

> 💡 **GAS exige os 3:** `GameplayAbilities`, `GameplayTags`, `GameplayTasks`. Esquecer `GameplayTags`/`GameplayTasks` dá erro de link obscuro.

Depois de editar: **feche o editor → Regenerate Visual Studio project files → recompile**.

---

## 3. Organização de pastas

### 3.1 C++ (`Source/DarkDungeonRift/`)

Saia do "tudo na raiz" do template. Estrutura sugerida:

```
Source/DarkDungeonRift/
├── Characters/        DDRCharacterBase, DDRPlayerCharacter, DDREnemyCharacter
├── Movement/          DDRCharacterMovementComponent (sprint/dash/air)
├── Animation/         DDRAnimInstance, DDRLocomotionTypes.h (enums/structs)
├── GAS/
│   ├── Abilities/     DDRGameplayAbility (base), GA_Attack, GA_Dash, GA_Launcher...
│   ├── Attributes/    DDRAttributeSet (HP, Stamina, Damage...)
│   ├── Effects/       (effects geralmente são assets, mas helpers ficam aqui)
│   └── DDRAbilitySystemComponent
├── Combat/            DDRComboComponent, hit detection helpers
├── Camera/            DDRCameraComponent / setup topdown
├── Run/               DDRRunManager, DDRRoomManager (roguelike loop)
└── DarkDungeonRift.h / .cpp (módulo)
```

> 📛 **Prefixo de classe:** o template usa nomes longos (`ADarkDungeonRiftCharacter`). Adote um prefixo curto: **`DDR`** (ex.: `ADDRPlayerCharacter`, `UDDRAttributeSet`). Renomeie a classe do template cedo — é mais barato agora que depois.

### 3.2 Content (`Content/DarkDungeonRift/`)

```
Content/DarkDungeonRift/
├── Characters/
│   ├── Player/        BP_DDRPlayer, ABP_DDRPlayer, mesh, montages
│   └── Enemies/       BP_Enemy_Melee, BP_Enemy_Ranged
├── GAS/
│   ├── Abilities/     GA_* (blueprint abilities), montages associadas
│   ├── Effects/       GE_* (GameplayEffects)
│   └── Attributes/    (curve tables, etc.)
├── Animation/
│   ├── Locomotion/    BlendSpaces, Start/Stop/Loop, Turn, Jump
│   └── Combat/        AM_Combo_*, AM_Aerial_*, AM_Launcher
├── Input/             IMC_Default, IA_Move/Look/Attack/Dash/Jump/Sprint
├── Run/               salas (levels/sublevels), DataTables de recompensa
└── UI/                HUD funcional
```

> 🗂️ Tudo sob `Content/DarkDungeonRift/` (não solto na raiz de `Content/`). Facilita migração e mantém os assets do template/engine separados dos seus.

---

## 4. Gameplay Tags — registre cedo

GAS vive de tags. Crie um arquivo de tags nativo (C++) **ou** `Config/DefaultGameplayTags.ini`. Convenção:

```
State.Combat.Attacking
State.Combat.InAir          ← juggle / combo aéreo
State.Movement.Sprinting
State.Movement.Dashing
State.Dead
Ability.Attack.Light
Ability.Attack.Launcher
Ability.Attack.AirSlam
Ability.Dash
Cooldown.Dash
GameplayCue.Hit.Impact
GameplayCue.Land.Hard
```

> Detalhes de como usar em [05 — GAS](05_GAS_Architecture.md). Registre-as agora pra não espalhar strings mágicas.

---

## 5. Config inicial recomendada

| Config | Onde | Valor MVP |
|---|---|---|
| **Default GameMode** | Project Settings → Maps & Modes | seu `ADDRGameMode` |
| **Default Pawn** | GameMode | `ADDRPlayerCharacter` (ou nada, se spawnar via BP) |
| **Fixed/Smoothed framerate** | Project Settings | deixe default; mire 60fps |
| **Collision channels** | Project Settings → Collision | crie canais `Player`, `Enemy`, `Hitbox` (combate precisa) |

> 🎯 **Canais de colisão de combate** (`Hitbox`, `Hurtbox`) valem criar já — o doc 15 (hit detection) depende deles.

---

## 6. Checklist

- [ ] Plugins P0 habilitados (GameplayAbilities, AnimationLocomotionLibrary, MotionWarping)
- [ ] `Build.cs` com `GameplayAbilities`+`GameplayTags`+`GameplayTasks`
- [ ] Projeto recompila sem erro após adicionar módulos
- [ ] Pastas C++ reorganizadas (Characters/GAS/Combat/...)
- [ ] Pastas Content sob `Content/DarkDungeonRift/`
- [ ] Classe do template renomeada p/ prefixo `DDR`
- [ ] Gameplay Tags iniciais registradas
- [ ] Canais de colisão de combate criados

---

## 7. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Erro de link `Unresolved external` em GAS | Falta `GameplayTags`/`GameplayTasks` no Build.cs | Adicione os 3 módulos |
| Plugin não aparece | Engine não reiniciada | Reinicie o editor após habilitar |
| `.generated.h` não acha símbolo GAS | Header sem `#include "AbilitySystemComponent.h"` | Inclua e regenere project files |
| Mudou Build.cs e nada mudou | Build cacheado | Regenerate project files + rebuild |

---

## 8. Próximo passo

→ [05 — Arquitetura GAS](05_GAS_Architecture.md): montar o esqueleto de combate/stats.
