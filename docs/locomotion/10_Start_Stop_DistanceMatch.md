# 10 — Start & Stop Transitions + Distance Match Stops · 🟡 P1

> Cobre **Start & Stop Transitions** (recurso #7) e **Distance Match Stops** (recurso #8). São o que faz o personagem **começar e parar com os pés plantados**, sem deslizar ("foot skating").

> 🧭 **Topdown:** isto é **P1, não P0**. Foot skating é muito menos perceptível de cima e à distância. Implemente **depois** do combate jogável (M1+). Aqui priorizamos *responsividade* — um stop bonito que atrasa o controle é pior que um stop simples e responsivo.

---

## 1. O problema que resolvem

```
SEM start/stop transition:           COM:
Idle → [pop!] → Loop a 100%          Idle → Start (acelera o passo) → Loop
Loop → [pop!] → Idle                 Loop → Stop (planta o pé na distância certa) → Idle
       ↑ pés deslizam                       ↑ pés plantados
```

- **Start Transition:** clip curto que leva de parado → loop de corrida, com o pé certo saindo.
- **Stop Transition:** clip que desacelera e **planta o pé** na posição onde o personagem realmente vai parar.
- **Distance Matching:** sincroniza o *tempo* da animação de stop com a *distância* física que falta até parar — é o que elimina o deslize.

---

## 2. Distance Matching — o conceito

A animação de stop tem uma **curva de distância** (quanto o root percorre ao longo do clip). O sistema toca o clip na posição que corresponde à **distância restante até a parada**:

```
Distância até parar (física, do CMC) ──→ [Distance Matching node] ──→ tempo do clip de Stop
        500cm restantes → início do clip
        0cm restantes   → fim do clip (pé plantado)
```

**Plugin:** `AnimationLocomotionLibrary` fornece os nós de **Distance Matching** (`Distance Matching`, `Advance Time by Distance Matching`). Habilite-o (doc 04).

**Equação mental:** a anim não toca por *tempo*, toca por *distância percorrida*. Assim, não importa a velocidade — o pé sempre planta no lugar certo.

---

## 3. Pré-requisito: braking previsível no CMC

Distance matching só funciona se você **souber onde** o personagem vai parar. Isso exige um braking controlado:

```cpp
// UDDRCharacterMovementComponent
// Braking que "desliza" de forma previsível até parar (estilo distance-match):
BrakingDecelerationWalking = 600.f;   // baixo = desliza mais (mais frames de stop)
// (mais alto = para seco; menos espaço p/ distance match)
```

Calcule a **distância de parada** prevista (`PredictStopLocation` ou física simples: `v²/(2a)`) e passe pro AnimInstance. O DungeonForged usa "stop-glide braking" pra exatamente isso — o personagem desliza um tico de forma controlada e a curva casa.

> ⚖️ **Trade-off de responsividade:** braking baixo = stops lindos mas o personagem "patina" um tico ao parar (menos responsivo). Braking alto = para seco (responsivo) mas menos espaço p/ distance match. **Para ARPG de ação, prefira o lado responsivo** — o jogador quer parar *já*. Tune com cuidado.

---

## 4. Montagem no AnimGraph

```
[State Machine: Locomotion]
  Idle ──(bIsMoving)──▶ Start ──(blend done)──▶ Loop
   ▲                                              │
   └──── Stop ◀──(bWantsToStop = accel~0 & speed>0)┘

Estado "Start":  toca Start clip; ou usa distance matching no início do Loop.
Estado "Stop":   [Distance Matching] dirige o tempo do Stop clip
                 pela DistanceToStop vinda do CMC.
```

**`bWantsToStop`** (do struct de estado, doc 08): verdadeiro quando aceleração ≈ 0 mas velocidade > limiar → entra no estado Stop.

---

## 5. Start Transition — opções

| Opção | Esforço | Qualidade |
|---|---|---|
| **Sem start (Idle→Loop direto)** | 0 | Aceitável em topdown! O "pop" inicial some à distância. |
| **Start clips por direção (4 ou 8-way)** | Alto | Melhor; o pé certo sai. Pós-MVP. |
| **Distance matching no start** | Médio | Sincroniza o passo com a aceleração. |

> 🎯 **Recomendação MVP topdown:** comece **sem start transition** (Idle→Loop direto). É a opção que prioriza responsividade e o jogador não nota de cima. Adicione start clips só se o feel pedir (P1+).

---

## 6. Stop Transition — onde vale o esforço

O **stop** é mais perceptível que o start (o personagem fica parado, dá tempo de ver o pé). Se for investir em um dos dois, invista no **stop**.

1. Tenha 1-2 clips de stop (parar com pé esquerdo / direito, ou um genérico).
2. Distance matching dirige o tempo pela `DistanceToStop`.
3. (Opcional) escolha o clip pelo pé que está em contato (foot phase) — polish P2.

---

## 7. Direção (start/stop em 8-way)

Para movimento em qualquer direção, idealmente start/stop têm variantes direcionais. **Mas** isso multiplica clips. Para o MVP:

- Use **forward** start/stop só, ou
- Confie no orient-to-movement (o personagem **gira** pra direção antes de iniciar, então "forward" cobre a maioria dos casos).

> Orient-to-movement (doc 06) simplifica muito: como o corpo aponta a direção do movimento, você precisa de menos variantes direcionais de start/stop.

---

## 8. Checklist

- [ ] `AnimationLocomotionLibrary` habilitado (doc 04)
- [ ] CMC expõe `DistanceToStop` previsível (braking controlado)
- [ ] `bWantsToStop` no struct de estado (doc 08)
- [ ] State machine: Idle↔Start↔Loop↔Stop
- [ ] Distance Matching node dirige o clip de Stop
- [ ] **Start:** começar SEM (Idle→Loop) no MVP; adicionar depois
- [ ] **Stop:** ≥1 clip + distance match
- [ ] Braking tunado pro lado **responsivo** (ARPG de ação)

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Pés deslizam ao parar | Sem distance matching, ou `DistanceToStop` errada | §2-3; valide a curva de distância no clip |
| Stop "demora" / sente travado | Braking muito baixo (desliza demais) | Aumente `BrakingDecelerationWalking` |
| Personagem "afunda" no stop clip | Curva de distância do clip mal autorada | Reautore/normalize a curva Distance no clip |
| Distance match não engata | Plugin não habilitado / nó sem curva | Habilite lib; clip precisa da curva "Distance" |
| Start dá "pop" forte | Sem transição + accel alta | Aceitável topdown; ou adicione start clip |

---

## 10. Referências

- [Distance Matching — UE oficial (AnimationLocomotionLibrary)](https://dev.epicgames.com/documentation/en-us/unreal-engine/distance-matching-in-unreal-engine)
- Padrão equivalente no DungeonForged: `docs/animation/18_8Way_StartLoopStop_Setup.md` (no projeto irmão).

---

## 11. Próximo passo

→ [11 — Stride Warping](11_Warping.md): casar o tamanho da passada com a velocidade real.
