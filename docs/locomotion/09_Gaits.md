# 09 — Gaits: Walk / Run / Sprint / Crouch · 🟢 P0 (sprint) / 🔵 P2 (crouch)

> Cobre **Multiple Gaits**, **Sprinting & Crouching** e **Gait Switch Transition States**. O gait é o "marcha" do personagem — quão rápido e com que animação ele se move.

> 🧭 **Topdown:** velocidade é **leitura de gameplay** (sprint pra reposicionar entre grupos é P0). Mas a *transição* visual entre gaits (recurso #9) é quase invisível de cima → P2.

---

## 1. Quantos gaits o MVP precisa?

Menos do que parece. Para um ARPG topdown:

| Gait | MVP? | Velocidade inicial | Uso |
|---|:---:|---|---|
| **Walk** | 🔵 Opcional | ~200 | Pouco usado em ARPG topdown. Talvez só em "exploração lenta". |
| **Run** | 🟢 **Sim** | ~500 | **Velocidade base.** O personagem corre por padrão (estilo Diablo/Hades). |
| **Sprint** | 🟢 **Sim** | ~750 | Burst pra reposicionar/fugir. Custa stamina (opcional). |
| **Crouch** | 🔵 P2 | ~150 | Raro em ARPG de ação. Só se houver stealth. **Corte do MVP.** |

> 💡 **Insight ARPG topdown:** o "default" costuma ser **Run**, não Walk. Walk vira o gait raro. Não copie o template third-person (que assume Walk default). Decida sua velocidade base como **Run**.

---

## 2. Gaits no Character Movement Component

O gait é **gameplay** → mora no CMC (autoridade), não na animação.

```cpp
// EDDRGait — definida UMA vez em DDRLocomotionTypes.h; o CMC (gameplay, abaixo)
// e o AnimInstance (poses, doc 08 §2) leem a MESMA enum (não duplicar).
UENUM(BlueprintType)
enum class EDDRGait : uint8 { Walk, Run, Sprint };

// Velocidades por gait (editáveis):
UPROPERTY(EditAnywhere, Category="DDR|Gait") float WalkSpeed   = 200.f;
UPROPERTY(EditAnywhere, Category="DDR|Gait") float RunSpeed    = 500.f;
UPROPERTY(EditAnywhere, Category="DDR|Gait") float SprintSpeed = 750.f;

EDDRGait CurrentGait = EDDRGait::Run;

void SetGait(EDDRGait NewGait)
{
    CurrentGait = NewGait;
    switch (NewGait)
    {
        case EDDRGait::Walk:   MaxWalkSpeed = WalkSpeed;   break;
        case EDDRGait::Run:    MaxWalkSpeed = RunSpeed;    break;
        case EDDRGait::Sprint: MaxWalkSpeed = SprintSpeed; break;
    }
}
```

> Em multiplayer, `CurrentGait` precisa ser replicado/predito (sprint via `FSavedMove`). No MVP single-player, simples assignment basta.

---

## 3. Sprint (🟢 P0)

### 3.1 Ativação

Sprint é hold (segurar) ou toggle. Liga no [input](../systems/07_Input.md):

```
IA_Sprint (Started)   → SetGait(Sprint)
IA_Sprint (Completed) → SetGait(Run)
```

### 3.2 Custo de stamina (opcional no MVP)

| Decisão | Recomendação MVP |
|---|---|
| Sprint custa stamina? | **Opcional.** Comece **sem** custo (sprint livre) p/ testar feel; adicione stamina se sprint trivializar o jogo. |
| Onde fica stamina | Attribute GAS (doc 05) — drena enquanto sprinta, regenera parado. |
| Exhaustion | P2 — se stamina zera, força Run por X s. |

> 🥊 **Sprint + combate:** decida se um ataque **cancela** sprint (provável sim) e se há um "sprint attack" (lunge). Isso conecta com o [doc 15](../combat/15_Combat_Overview.md). No MVP, sprint cancela ao atacar.

### 3.3 Feedback de sprint (barato, alto retorno)

Em topdown, comunique sprint por: leve **trail/partícula**, leve **FOV/zoom-out** (P1), ou tilt da câmera. Sem isso, sprint "parece" igual a run.

---

## 4. Crouch (🔵 P2 — provavelmente corte)

UE tem crouch nativo (`ACharacter::Crouch()`, `bWantsToCrouch`, `CrouchedHalfHeight`). **Mas:** num ARPG topdown de ação, crouch raramente serve ao gameplay. 

**Recomendação:** **não implemente no MVP.** Se um dia houver stealth/cobertura, o crouch nativo do UE resolve em horas. Não gaste tempo agora.

---

## 5. Gaits no AnimGraph

A animação **lê** o gait do CMC e escolhe poses. Duas abordagens:

| Abordagem | Como | Quando |
|---|---|---|
| **A — Blendspace por velocidade** | 1 blendspace 1D (Speed→Walk/Run/Sprint poses) ou 2D (Speed×Direction) | Simples, suave. **Recomendado MVP.** |
| **B — Pose direcional por gait (8-way)** | Sets de clips Start/Loop/Stop por gait × direção | Mais controle, mais clips. Pós-MVP. |

```
// AnimGraph (abordagem A):
[Get Gait] + [Get Speed] + [Get Direction]
   → [Blendspace 2D: Locomotion]  (eixo X=Direction, Y=Speed)
   → saída pro resto do graph
```

> No MVP, **um blendspace 2D** (Direction × Speed) cobrindo idle→run→sprint já entrega locomoção sólida e legível. Não precisa dos 24 clips de 8-way no começo.

---

## 6. Gait Switch Transition States (recurso #9 · 🔵 P2)

A transição **suave** entre gaits (ex.: run→sprint não dá "pop"). 

**Realidade topdown:** com um **blendspace por velocidade**, a transição já é suave de graça (interpolação do eixo Speed). O recurso #9 — *transition states dedicados* entre gaits — só importa se você for pra abordagem 8-way (B), onde cada gait tem clips próprios e o blend precisa de um estado-ponte.

**Recomendação:**
- Abordagem A (blendspace): transição suave **automática** → recurso #9 resolvido sem trabalho. ✅
- Abordagem B (8-way): adicione transition states só quando migrar (P2).

> Ou seja: escolher blendspace no MVP **te dá o recurso #9 de graça**. Mais um motivo pra começar simples.

---

## 7. Suavizar a troca de velocidade

Para o personagem não "saltar" de 500→750 instantâneo, interpole a velocidade alvo:

| Onde | Como |
|---|---|
| CMC | `MaxWalkSpeed` muda na hora; a **aceleração** (`MaxAcceleration`) controla quão rápido chega lá |
| Anim | O eixo Speed do blendspace interpola (`Speed` já suaviza via aceleração física) |

Tune `MaxAcceleration` (ex.: 1500-2500) e `BrakingDecelerationWalking` pro feel certo. Aceleração alta = responsivo; baixa = "pesado".

---

## 8. Checklist

- [ ] `EDDRGait` no CMC com Walk/Run/Sprint
- [ ] **Run como velocidade base** (não Walk)
- [ ] `SetGait` ajusta `MaxWalkSpeed`
- [ ] Sprint ligado no input (hold)
- [ ] Sprint cancela ao atacar (decisão de combate)
- [ ] Stamina: comece SEM, adicione se necessário
- [ ] Crouch **cortado** do MVP (P2)
- [ ] AnimGraph: blendspace 2D (Direction×Speed) lê Gait/Speed
- [ ] Feedback visual de sprint (trail/FOV — P1)
- [ ] `MaxAcceleration` tunado p/ responsividade

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Personagem "salta" de velocidade | `MaxAcceleration` muito alta + sem blend | Tune accel; deixe blendspace interpolar |
| Sprint não muda nada visualmente | Blendspace sem pose de sprint, ou Speed não chega lá | Adicione pose sprint; confira MaxWalkSpeed |
| Anim de gait errada | AnimBP lendo gait stale | Calcule gait em thread-safe update (doc 08) |
| Sprint "gruda" ligado | Faltou bind `Completed` no input | Doc 07 §8 |

---

## 10. Próximo passo

→ [10 — Start/Stop & Distance Match](10_Start_Stop_DistanceMatch.md): transições de início/parada com pés plantados.
