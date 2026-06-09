# 06 — Câmera Topdown · 🟢 P0

> A câmera **define o jogo**. Topdown 3/4 não é só "olhar de cima" — ela decide o que vale animar, como o combate é lido, e como a "altura" do combo aéreo é comunicada.

> Ponto de partida: o template tem `CameraBoom` (SpringArm) + `FollowCamera` atrás do personagem (third person). Vamos **reconfigurar** para topdown.

---

## 1. Anatomia da câmera topdown 3/4

```
        [Camera]
          ╲  ~55° de pitch pra baixo
           ╲
            ╲________
            [Player]   ← spring arm longo (700-1200), sem colisão de parede
```

| Componente | Config topdown | Nota |
|---|---|---|
| **SpringArm (CameraBoom)** | `TargetArmLength` 800-1100 | Distância do "zoom". Tune no playtest. |
| **Rotação do braço** | Pitch ~ **-55°**, Yaw fixo (ex.: -45° p/ 3/4 isométrico, ou 0°) | **Fixa** — não segue o mouse/personagem. |
| **`bUsePawnControlRotation`** | **false** | Crucial: a câmera **não** gira com o controller. |
| **`bInheritPitch/Yaw/Roll`** | **false** (todos) | Câmera independe da rotação do personagem. |
| **`bDoCollisionTest`** | false (geralmente) | Em topdown não quer a câmera "pulando" em paredes. |
| **Projeção** | Perspective (recomendado) ou Orthographic | Perspective dá profundidade ao combo aéreo; ortho é "isométrico puro". |

> 🎥 **Diferença vs template:** no third person `bUsePawnControlRotation=true` (câmera orbita). Em topdown é **false** e a rotação é fixa no braço. Esse é o ajuste nº 1.

---

## 2. Configuração no construtor (C++)

```cpp
// ADDRPlayerCharacter()
CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
CameraBoom->SetupAttachment(RootComponent);
CameraBoom->TargetArmLength = 950.f;
CameraBoom->SetRelativeRotation(FRotator(-55.f, -45.f, 0.f)); // pitch, yaw, roll
CameraBoom->bUsePawnControlRotation = false;
CameraBoom->bInheritPitch = false;
CameraBoom->bInheritYaw   = false;
CameraBoom->bInheritRoll  = false;
CameraBoom->bDoCollisionTest = false;
CameraBoom->bEnableCameraLag = true;          // suaviza o follow
CameraBoom->CameraLagSpeed   = 10.f;

FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
FollowCamera->bUsePawnControlRotation = false;
```

---

## 3. Rotação do personagem (decisão de design)

A câmera fixa libera a rotação do personagem para dois modelos clássicos de ARPG topdown:

| Modelo | Como | Sensação | Quando |
|---|---|---|---|
| **A — Orient to Movement** | `bOrientRotationToMovement=true`, `bUseControllerRotationYaw=false` | Personagem olha pra onde anda. Simples, fluido. | **Recomendado p/ MVP.** Tipo Hades. |
| **B — Orient to Cursor (twin-stick/aim)** | Personagem encara o mouse/stick direito | Mira independente do movimento. | Se quiser ataques mirados/ranged precisos. |

```cpp
// Modelo A (MVP):
GetCharacterMovement()->bOrientRotationToMovement = true;
GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f); // gira rápido = responsivo
bUseControllerRotationYaw = false;
```

> 🎯 **Por que isto elimina o "Turn In Place" no MVP:** com orient-to-movement + câmera topdown, o personagem **gira andando**, raramente fica parado girando o corpo. Turn In Place foi **removido do escopo** (sem assets, payoff ~nulo). Decisão consciente, documentada.

> Se for Modelo B, você roda o personagem por código (LookAt no cursor projetado no plano do chão) — e aí ataques direcionais ficam ótimos, mas é mais trabalho. Avalie no M1.

---

## 4. Comunicando "altura" no combo aéreo (o problema topdown)

Em topdown, um inimigo a 300cm de altura e um no chão **ocupam quase o mesmo pixel**. Resolva com:

| Técnica | Efeito | Custo |
|---|---|---|
| **Sombra no chão (blob/decal)** | A sombra fica no chão enquanto o corpo sobe → o gap comunica altura. | Barato, **essencial**. |
| **Escala sutil** | Inimigo no ar levemente maior (mais perto da câmera). | Barato. |
| **Perspective (não ortho)** | Profundidade real faz o objeto alto "subir" na tela. | Grátis (escolha de projeção). |
| **Leve dolly/zoom-out no juggle** | Câmera afasta um tico no combo aéreo → cabe a ação vertical. | Médio. P1. |
| **Outline/realce no alvo juggleado** | Foco visual no que está sendo combado. | Barato. P1. |

> 🪂 Sem **sombra no chão**, o combo aéreo fica ilegível em topdown. Trate isso como requisito do pilar (não polish).

---

## 5. Framing & foco (P1)

| Recurso | O que faz | Prioridade |
|---|---|---|
| **Camera lag** | Suaviza o follow (já no §2). | 🟢 P0 |
| **Lookahead pela velocidade** | Câmera antecipa a direção do movimento (mostra mais à frente). | 🟡 P1 |
| **Framing de combate** | Em combate, leve zoom-in; explorando, zoom-out. | 🟡 P1 |
| **Foco em alvo/boss** | Enquadrar player + alvo (midpoint). | 🟡 P1 (essencial no boss) |
| **Screen shake / hit-stop** | Juice de impacto. | 🟢 P0 (barato, alto retorno) |

> No MVP, **camera lag + screen shake** já entregam 80% do "feel". GameplayCameras plugin (rigs avançados) é P2.

---

## 6. Checklist

- [ ] SpringArm com pitch ~-55°, yaw fixo, `bUsePawnControlRotation=false`
- [ ] `bInheritPitch/Yaw/Roll = false`
- [ ] `TargetArmLength` ~950 (tunável)
- [ ] Camera lag ligado
- [ ] Modelo de rotação escolhido (A: orient-to-movement p/ MVP)
- [ ] **Sombra no chão** no player e inimigos (p/ leitura aérea)
- [ ] Screen shake no impacto (via GameplayCue — doc 05)
- [ ] Collision test desligado (sem câmera "pulando")

---

## 7. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Câmera gira com o personagem | `bInheritYaw=true` ou `bUsePawnControlRotation=true` | Desligue ambos (§2) |
| Câmera "treme"/pula perto de parede | `bDoCollisionTest=true` | Desligue p/ topdown |
| Combo aéreo ilegível | Sem sombra no chão | Adicione decal/blob shadow (§4) |
| Movimento sente "duro" | Sem camera lag, ou RotationRate baixo | Lag ON + RotationRate 720 (§3) |
| Personagem não vira pra onde anda | `bOrientRotationToMovement=false` | Ligue (Modelo A, §3) |

---

## 8. Próximo passo

→ [07 — Input](07_Input.md): ligar teclas a movimento e habilidades GAS.
