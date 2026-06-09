# 11 — Stride Warping (+ Orientation / Slope) · 🟡 P1 / 🔵 P2

> Cobre **Stride Warping** (recurso #10). Estica/encolhe o tamanho da passada pra casar com a velocidade real, eliminando foot skating **sem** precisar de um clip por velocidade.

> 🧭 **Topdown:** Stride Warping é **anti-skating de pés** — exatamente o que a câmera de cima mais esconde. **P1 baixo, tendendo a P2.** Faça só depois de start/stop, e só se notar deslize na sua faixa de velocidades. Não é onde o MVP ganha.

---

## 1. O que resolve

Loops de locomoção são autorados a uma velocidade fixa (ex.: 450 cm/s). Se a velocidade real for 750 (sprint), os pés **arrastam** — a anim "anda" mais devagar que a cápsula. Stride Warping **escala a passada** pra casar:

```
StrideScale = LocomotionSpeed / RootMotionSpeed
   1.0 = sem mudança · 0.5 = meia passada · 2.0 = passada dobrada
```

- `LocomotionSpeed` = velocidade real (CMC).
- `RootMotionSpeed` = velocidade de autoria do clip.

> Sem isto, ou você autora 1 clip por velocidade (caro) ou aceita o skating. Stride warping é o meio-termo data-driven. Mas em topdown, **aceitar um pouco de skating é uma opção válida pro MVP**.

---

## 2. Plugin & wiring

**Plugin:** `AnimationWarping` (habilite — doc 04). O nó **Stride Warping** trabalha em **Component Space**:

```
... → [Local To Component] → [Stride Warping] → [Component To Local] → [Slot 'DefaultSlot'] → ...
```

Campos obrigatórios (senão dá WARNING):

| Campo | Valor (Mannequin) |
|---|---|
| **Pelvis Bone** | `pelvis` |
| **IK Foot Root Bone** | `ik_foot_root` |
| **Foot Definitions** | 2 índices (perna L + R) |

**Foot Definitions** (por perna): `IK Foot Bone` (`ik_foot_l/r`), `FK Foot Bone` (`foot_l/r`), `Thigh Bone` (`thigh_l/r`), `Num Bones In Limb` = `2`.

---

## 3. ⚠️ Pré-requisito CRÍTICO: skeleton com IK bones

Stride Warping **exige** a cadeia de IK bones virtuais do padrão UE:

```
root
└─ ik_foot_root
   ├─ ik_foot_l
   └─ ik_foot_r
```

- Se usar o **Mannequin UE5**, já tem tudo. ✅
- Se usar mesh de pacote (Fab) **sem** IK bones → **retarget pro Mannequin** (melhor) ou adicione virtual bones. Sem a cadeia IK, o dropdown só mostra `None` e o nó não warpa.

> Como o MVP usa Mannequin/placeholder (doc 01 §6), você já tem os IK bones. Quando trocar pra arte final, garanta o retarget.

---

## 4. Modo: Graph vs Manual

| | **Graph** (recomendado) | **Manual** |
|---|---|---|
| Quem calcula o scale | o nó (`LocomotionSpeed / RootMotionSpeed`) | você (`Speed / AuthoredLoopSpeed`) |
| Precisa root motion nos Loops? | **Sim** | Não (clips in-place) |
| Pino exposto | `Locomotion Speed` ← `Speed` | `Stride Scale` ← `Speed / AuthoredLoopSpeed` |

> Se suas Loops têm root motion → **Graph** (mais simples). Se são in-place → **Manual** com `Speed / AuthoredLoopSpeed` (exponha `AuthoredLoopSpeed`, default ~450, no AnimInstance).

**Clamp de segurança** (Stride Scale Modifier): `Clamp Min 0.5`, `Clamp Max 1.5` — evita "passos de gigante".

---

## 5. Orientation Warping (bônus, recurso "de graça")

O mesmo plugin tem **Orientation Warping**: gira a parte inferior do corpo pra apontar a direção real do movimento, **sem** precisar de 8 clips direcionais.

> 🎯 **Em topdown com orient-to-movement, Orientation Warping tem payoff baixo** (o corpo já gira inteiro pra direção do movimento via CMC). Só vale se você for pro **Modelo B (aim/strafe)** — aí o personagem mira numa direção mas anda em outra, e orientation warping mantém os pés coerentes. Avalie só se adotar strafe.

---

## 6. Slope Warping (rampas)

Ajusta os pés/pelvis à inclinação do chão. Relevante se suas masmorras têm **rampas/declives**. Para arenas planas (típico MVP), **pule**. Se tiver rampas, é mais barato que foot IK completo pra esse caso.

---

## 7. Veredito de prioridade (seja honesto)

```
Investir em Stride Warping no MVP topdown? 
  → Só se: (a) combate já fechado E (b) você VÊ o skating na sua faixa de velocidade.
  → Senão: pule. Aceite o skating mínimo. Ninguém vai notar de cima a 950 de zoom.
```

Se for implementar, o caminho mais barato: **Graph mode + Mannequin + clamp 0.5-1.5**. 20 minutos de setup, fim.

---

## 8. Checklist

- [ ] `AnimationWarping` habilitado
- [ ] Skeleton tem `ik_foot_root`/`ik_foot_l`/`ik_foot_r` (§3)
- [ ] Wiring `Local→Component → Stride Warping → Component→Local`
- [ ] Pelvis/IK Foot Root/Foot Definitions preenchidos (mata WARNING)
- [ ] Modo escolhido (Graph se root motion; Manual se in-place)
- [ ] `Locomotion Speed` ← `Speed` (Graph) ou `Speed/AuthoredLoopSpeed` (Manual)
- [ ] Clamp 0.5-1.5 no Stride Scale Modifier
- [ ] Orientation/Slope: **pulados** salvo se aim/rampas

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| WARNING no nó | Pelvis/IK Foot Root/Foot Definitions vazios | Preencha (§2) |
| Dropdown só mostra `None` | Skeleton sem IK bones | Retarget pro Mannequin (§3) |
| Nó sem warning mas não warpa | Manual com Stride Scale=1.0 fixo | Ligue `Speed/AuthoredLoopSpeed` |
| Passos "de gigante" | Sem clamp | Clamp Max 1.5 |
| Snap brusco no scale | `AuthoredLoopSpeed` longe do real | Calibre pela Loop; ative Interp Result |

---

## 10. Referências

- [Pose Warping (Stride/Orientation/Slope) — UE oficial](https://dev.epicgames.com/documentation/en-us/unreal-engine/pose-warping-in-unreal-engine)
- Setup detalhado equivalente no DungeonForged: `docs/animation/19_StrideWarping_Setup.md` (projeto irmão) — mesmo nó, passo a passo com troubleshooting de WARNING.

---

## 11. Próximo passo

→ [13 — Jump / Fall / Landing](13_Jump_Fall_Landing.md): a base do combo aéreo (esta sim é P0). *(Turn In Place foi removido do escopo.)*
