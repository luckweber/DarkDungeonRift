# 14 — Foot & Leg IK · 🔵 P2

> Cobre **Foot & Leg IK** (recurso #12). Ajusta os pés ao terreno (não atravessam/flutuam acima do chão) e adapta as pernas a rampas/escadas/desníveis.

> 🧭 **Veredito direto:** em **topdown** com a câmera a ~950 de distância e pitch -55°, o detalhe do pé é **quase imperceptível**. Foot IK é **P2** e só vale se suas masmorras tiverem **rampas/escadas/desníveis** visíveis. Em **arenas planas** (típico MVP), **pule inteiro**. Não é onde o jogo ganha.

---

## 1. O que Foot IK resolve (e quando você NÃO precisa)

| Cenário | Foot IK ajuda? |
|---|---|
| Arena **plana** | ❌ Não — chão nivelado, pés já assentam. **Pule.** |
| **Rampas / declives** | ✅ Sim — sem IK, um pé fica no ar/enterrado |
| **Escadas / degraus** | ✅ Sim — pernas adaptam a alturas diferentes |
| Personagem **parado** em superfície irregular | ✅ Sim — leitura de "plantado" |
| Câmera **topdown longe** | ⚠️ Mesmo quando ajuda, o ganho visual é pequeno |

> 🎯 **Decisão de design que economiza esse doc inteiro:** se você projetar as arenas do MVP **planas** (e dá pra ter level design ótimo só com paredes/obstáculos, sem desnível), **você não precisa de Foot IK no MVP**. Diablo/Hades têm pouquíssimo desnível jogável. Considere isso.

---

## 2. Se você precisar — duas abordagens

| Abordagem | Como | Prós/Contras |
|---|---|---|
| **A — Control Rig (recomendado UE5)** | Control Rig de "Foot IK" pós-locomoção (trace + two-bone IK + ajuste de pelvis) | Padrão UE5, robusto p/ slopes; +custo de setup |
| **B — Foot Placement node** | Nó do `AnimationLocomotionLibrary` | Pronto, data-driven, lida com slopes/stairs bem |
| **C — Two-bone IK manual** | `Trace pra baixo + Two Bone IK` no AnimGraph | Simples, controle total; menos robusto em casos extremos |

> Para UE 5.4, o **Foot Placement node** (B) é o caminho mais moderno e barato — habilite `AnimationLocomotionLibrary` (doc 04) e use o nó. O DungeonForged usa two-bone trace manual (C) — funciona, mas o Foot Placement node é mais robusto pra rampas/escadas.

---

## 3. Como funciona (conceito)

```
Para cada pé:
  1. Line trace do quadril pra baixo na posição do pé.
  2. Achou o chão a uma altura diferente? → calcula offset.
  3. Two-bone IK move o pé (e ajusta calf/thigh) pro ponto de contato.
  4. Ajusta o pelvis pra baixo o suficiente p/ ambos os pés alcançarem (sem esticar).
  5. Rotaciona o pé pra alinhar à normal do chão (em rampa).
```

**Pré-requisito:** skeleton com cadeia IK (`ik_foot_root`, `ik_foot_l/r`) — mesmo do [Stride Warping (doc 11 §3)](11_Warping.md). Mannequin já tem.

---

## 4. Onde fica no AnimGraph

Foot IK é um dos **últimos** nós, depois da locomoção e warping, perto do output:

```
... → [Stride Warping] → [Foot Placement / Foot IK] → [Slot DefaultSlot] → [Output]
```

> Vem **depois** de tudo que move as pernas (locomoção, warping) e **antes** do slot de combate (montages de ataque geralmente desligam foot IK).

---

## 5. Desligar IK quando não faz sentido

Foot IK deve **desligar** (alpha→0) quando:
- O personagem está **no ar** (`bIsFalling`) — não há chão pra assentar.
- Tocando uma **montage de combate/aéreo** — a anim de combate manda.
- Em **root motion** forte (dash, launcher).

Controle por um `FootIKAlpha` que cai a 0 nesses estados.

---

## 6. Veredito de prioridade

```
Arenas planas no MVP?  → NÃO faça Foot IK. Ponto.
Tem rampas/escadas?    → Use o Foot Placement node (B). ~1h de setup.
Quer polish pós-MVP?   → Control Rig de foot IK (A).
```

> Honestamente: das 12 features, esta é a de **menor prioridade** para um ARPG topdown de arenas. Gaste o tempo no combate.

---

## 7. Checklist (só se for fazer)

- [ ] Confirmado que há **desnível** nas arenas (senão, pule)
- [ ] Skeleton com cadeia IK (`ik_foot_root`/l/r)
- [ ] Abordagem escolhida (Foot Placement node recomendado)
- [ ] Nó posicionado após warping, antes do slot de combate
- [ ] `FootIKAlpha` desliga no ar / montages / root motion
- [ ] Ajuste de pelvis ativo (pés alcançam sem esticar)

---

## 8. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Pés atravessam rampa | Foot IK ausente/desligado | Ative o nó; cheque alpha |
| Personagem "afunda"/estica | Ajuste de pelvis errado | Limite o pelvis drop; cheque limb length |
| Pé "treme" em movimento | Trace sem suavização | Interpole o offset (foot lock/interp) |
| IK ativo no ar (perna "procura" chão) | `FootIKAlpha` não zera em `bIsFalling` | §5 |
| Dropdown de bones vazio | Skeleton sem IK bones | Retarget Mannequin (doc 11 §3) |

---

## 9. Referências

- [Foot Placement / Control Rig Foot IK — UE oficial](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-locomotion-library-in-unreal-engine)

---

## 10. Próximo passo

Locomoção completa. → Combate: [15 — Combat Overview](../combat/15_Combat_Overview.md), o coração do jogo.
