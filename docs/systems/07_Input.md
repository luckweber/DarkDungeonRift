# 07 — Input (Enhanced Input) · 🟢 P0

> Mapeamento de input para ARPG topdown, e — crucial pra hack'n'slash — **como o input liga nas abilities do GAS** e como bufferizar combos para o jogo sentir responsivo.

> O template já usa **Enhanced Input** (`IMC`, `IA_*`). Vamos estender pra combate e ligar no GAS.

> 🎮 **Mapa de teclas e botões (o que o jogador aperta):** [39 — Controles](39_Controls.md) — tabela canônica KB+M e gamepad. Este doc (07) cobre a **implementação**; o 39 cobre o **layout**.

---

## 1. Input Actions necessárias (MVP)

| Input Action | Tipo | Ação | Liga em |
|---|---|---|---|
| **IA_Move** | Axis2D (Vector2D) | Mover (WASD / left stick) | CMC (AddMovementInput) |
| **IA_Look** | Axis2D | Mirar cursor / right stick | Câmera/rotação (Modelo B) ou nada (Modelo A) |
| **IA_Attack** | Digital (bool) | Ataque leve / combo | GAS: `Ability.Attack.Light` |
| **IA_Dash** | Digital | Dash/esquiva | GAS: `Ability.Dash` |
| **IA_Jump** | Digital | Pulo (base do aéreo) | GAS ou `Character::Jump` |
| **IA_Sprint** | Digital (hold) | Correr | CMC (gait) — ver [doc 09](../locomotion/09_Gaits.md) |
| **IA_Launcher** | Digital | Lançar (combo aéreo) | GAS: `Ability.Attack.Launcher` |
| **IA_AirSlam** | Digital | Slam descendente | GAS: `Ability.Attack.AirSlam` |
| **IA_Interact** | Digital | Pegar recompensa / porta | gameplay |

> 💡 **Move em topdown:** o vetor 2D do `IA_Move` precisa ser convertido pro **espaço do mundo relativo à câmera fixa** (não à rotação do personagem). Como a câmera tem yaw fixo, use a rotação do braço/câmera (só yaw) pra montar Forward/Right. Ver §3.

---

## 2. Estrutura de assets

```
Content/DarkDungeonRift/Input/
├── IMC_Default            (Input Mapping Context)
├── IA_Move, IA_Look
├── IA_Attack, IA_Dash, IA_Jump, IA_Sprint
├── IA_Launcher, IA_AirSlam
└── IA_Interact
```

Adicione o `IMC_Default` ao subsystem no `BeginPlay` (o template já faz isso — reaproveite).

---

## 3. Move relativo à câmera (topdown)

```cpp
void ADDRPlayerCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D Input = Value.Get<FVector2D>();
    if (!Controller) return;

    // Câmera topdown tem YAW FIXO no braço — use o yaw da câmera, não do controller.
    const float CamYaw = CameraBoom->GetComponentRotation().Yaw;
    const FRotator YawRot(0.f, CamYaw, 0.f);
    const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
    const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

    AddMovementInput(Forward, Input.Y);
    AddMovementInput(Right,   Input.X);
}
```

> ⚠️ Se usar o **controller rotation** (template padrão) em vez do yaw da câmera, o "pra cima" do analógico não casa com a tela quando o yaw da câmera é -45° (isométrico). Sempre derive do **yaw fixo da câmera**.

---

## 4. Ligar input → GAS (o padrão correto)

Não chame a ability "na mão". Use o **binding por Input ID** do GAS para que combos/cooldowns/cancelamentos funcionem.

**Opção recomendada (UE 5.4):** Enhanced Input + um enum de `InputID`, vinculando no `SetupPlayerInputComponent`:

```cpp
// Ao conceder a ability (doc 05 §8), passe o InputID:
ASC->GiveAbility(FGameplayAbilitySpec(GA_Attack, 1, (int32)EDDRAbilityInputID::Attack));

// No SetupPlayerInputComponent, traduza a IA pra ativação por InputID:
EnhancedInput->BindAction(IA_Attack, ETriggerEvent::Started, this, &ThisClass::OnAttackPressed);
// OnAttackPressed → ASC->AbilityLocalInputPressed((int32)EDDRAbilityInputID::Attack);
```

> Alternativa mais moderna: o padrão **`UGameplayAbility` + `AbilityInputBinding`** do Lyra (mapeia `IA` direto a `GameplayTag` da ability). Para o MVP, o InputID enum é mais simples e suficiente.

| Ability | Input ID | Ativada por |
|---|---|---|
| Attack | `Attack` | IA_Attack (Started) |
| Dash | `Dash` | IA_Dash |
| Launcher | `Launcher` | IA_Launcher |
| AirSlam | `AirSlam` | IA_AirSlam |

---

## 5. Input buffering (essencial p/ hack'n'slash)

Sem buffer, o jogador precisa apertar no frame *exato* — o combate sente "engasgado". **Bufferize.**

**Como funciona:**
1. Durante um golpe, há uma **janela de combo** (definida por AnimNotifyState na montage — ver [doc 15](../combat/15_Combat_Overview.md)).
2. Se o jogador aperta Attack **antes** da janela, **guarde** o input (timestamp).
3. Quando a janela abre, **consuma** o input bufferizado → avança o combo.

```
Frame:    [---- golpe 1 ----][janela combo][---- golpe 2 ----]
Input:           ↑ apertou aqui (cedo)
Buffer:          └─ guarda ──────┘ consome quando janela abre → golpe 2 sai liso
```

**Implementação MVP:** uma variável `BufferedInputTag` + `BufferedTime` no character/combo component; a janela de combo (notify) checa se há input válido recente (ex.: últimos 0.3s) e ativa a próxima ability. GAS também oferece `UAbilityTask_WaitInputPress` dentro da ability.

| Parâmetro | Valor inicial |
|---|---|
| Janela de buffer | 0.2-0.35 s antes da janela de combo |
| Janela de combo (na montage) | definida por notify, ~0.3-0.5 s |

> 🥊 **Buffer é o que separa um combate "ok" de um "gostoso".** É P0 pro pilar de combate, mesmo que pareça detalhe.

---

## 6. Considerações de gamepad

- ARPG topdown joga muito bem no gamepad. Garanta deadzone no `IA_Move`.
- **Sprint:** hold (gatilho/botão) ou toggle — exponha como opção.
- **Modelo B (aim):** right stick → direção de mira. Sem mouse, derive a rotação do stick.

---

## 7. Checklist

- [ ] Input Actions MVP criadas (Move/Look/Attack/Dash/Jump/Sprint/Launcher/AirSlam/Interact)
- [ ] `IMC_Default` adicionado ao subsystem no BeginPlay
- [ ] Move deriva Forward/Right do **yaw fixo da câmera** (§3)
- [ ] Abilities concedidas com Input ID (§4)
- [ ] Input liga em `AbilityLocalInputPressed`, não chamada direta
- [ ] **Input buffer** de combo implementado (§5)
- [ ] Deadzone de gamepad no Move

---

## 8. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| "Pra cima" do analógico não casa com a tela | Move usa controller rotation, não yaw da câmera | §3 — use yaw da câmera fixa |
| Combo "engasga", precisa timing perfeito | Sem input buffer | §5 |
| Ability não ativa pela tecla | Input não vinculado ao InputID, ou ability não concedida com esse ID | §4 |
| Dash dispara repetido segurando | Use `Started`, não `Triggered`, p/ ações de toque | Ajuste `ETriggerEvent` |
| Sprint não para ao soltar | Faltou bind do `Completed`/`Canceled` | Bind hold com Started+Completed |

---

## 9. Próximo passo

→ [39 — Controles](39_Controls.md) (bindings default) · Locomoção: [08 — Visão Geral](../locomotion/08_Locomotion_Overview.md).
