# 43 — Spike M⁻¹ (validação aéreo) · 🟢 P0

> **Protótipo descartável** que responde a pergunta de risco ALTA do projeto: *"follow launch + co-altitude funcionam em top-down?"* Faça **antes** do M0/M2 ([17 — Roadmap](17_Implementation_Roadmap.md), [16 §1](combat/16_Aerial_Combos.md)).

> ⏱️ **Orçamento:** 2-4 dias. **Não** é feature shippable — é cubos, tags mínimas e um número na tela.

---

## 1. Perguntas que o spike deve responder

| # | Pergunta | Critério de sucesso |
|---|---|---|
| 1 | `RootMotionSource` mantém **player e alvo na mesma altitude** por ≥2s? | Altura Z dos dois diverge < 30 cm durante juggle |
| 2 | Câmera top-down **enquadra** os dois no ar? | Ambos visíveis sem zoom manual em 1080p |
| 3 | Leitura: jogador entende **quem está no ar**? | 3 playtesters internos acertam estado sem tutorial |
| 4 | Pop + decay **termina** o juggle em 6-8 hits? | Alvo cai antes do hit 9 com valores do [16 §3](combat/16_Aerial_Combos.md) |
| 5 | Hit-stop + pop **não** desynca alturas? | Após freeze, co-altitude restaura em 1 frame |

Se **qualquer** pergunta 1 ou 2 falhar → revisar câmera ou follow launch **antes** de animar personagem.

---

## 2. Escopo (o que entra / o que fica de fora)

### ✅ Entra

- 2 cubos (`ACharacter` ou `AActor` + `UCapsuleComponent`)
- `GA_Launcher` mínima com `RootMotionSource` vertical
- `GA_AirAttack` mínima (1 montage ou timer fake)
- Tags: `State.Combat.InAir`, `State.Combat.Airborne` ([41](systems/41_Gameplay_Tags.md))
- Câmera top-down do [06](systems/06_Camera_TopDown.md) (spring arm existente)
- Debug: `PrintString` com Z player / Z alvo / hit count
- Opcional: 1 `DT_SpikeTuning` com `AltitudeAlvo`, `PopHeight`, `decayFactor` ([44](systems/44_Data_Driven.md))

### ❌ Fica de fora

- Mesh, AnimBP, VFX, SFX
- Combo de chão, dash, inimigos reais
- UI, run manager, Ecos
- Networking

---

## 3. Setup rápido (passo a passo)

### Dia 1 — Plugins + cubos

1. Habilitar **GameplayAbilities**, **MotionWarping** ([04](systems/04_Project_Setup.md)).
2. Mapa vazio `L_Spike_Aerial` com chão 20×20 m.
3. `BP_Spike_Player` — cubo azul, `AbilitySystemComponent`, `GA_Launcher`, `GA_AirAttack`.
4. `BP_Spike_Target` — cubo vermelho, `AbilitySystemComponent` (só recebe tags/RM).
5. Input: tecla `E` = launcher, `LMB` = air attack ([39](systems/39_Controls.md) simplificado).

### Dia 2 — RootMotionSource

Implementar no `GA_Launcher` ([16 §2](combat/16_Aerial_Combos.md)):

```
1. Trace curto à frente → acha BP_Spike_Target
2. No ALVO:
     - AddLooseGameplayTag(State.Combat.Airborne)
     - Apply RootMotionSource_JumpForce ou MoveToForce → AltitudeAlvo (300 cm)
3. No PLAYER:
     - AddLooseGameplayTag(State.Combat.InAir)
     - Mesmo AltitudeAlvo (300 cm) — MESMO valor
4. Hang ~0.4s no topo (timer ou RM duration)
```

> ⚠️ **Proibido** `LaunchCharacter` neste spike — só prova o contrato técnico aprovado na design review.

### Dia 3 — Juggle loop

`GA_AirAttack`:

```
1. Requer State.Combat.InAir no player
2. "Hit" no alvo (overlap ou trace)
3. GE_Damage fake (log only)
4. No alvo: novo RootMotionSource pop (PopHeight * decay^hitCount)
5. hitCount++; se hitCount >= MaxJuggleHits → remove Airborne, deixa cair
```

Parâmetros iniciais ([16 §3](combat/16_Aerial_Combos.md)):

| Var | Valor |
|---|---|
| AltitudeAlvo | 300 cm |
| PopHeight | 150 cm |
| decayFactor | 0.85 |
| MaxJuggleHits | 7 |
| Hang-time | 0.35 s |

### Dia 4 — Câmera + playtest

1. Ajustar SpringArm: offset, zoom, dead zone ([06](systems/06_Camera_TopDown.md)).
2. Gravar 30s de vídeo.
3. Preencher checklist §5.

---

## 4. Data-driven no spike (mínimo)

Mesmo no protótipo, exponha tuning em **`DT_SpikeTuning`**:

```cpp
USTRUCT()
struct FDDRSpikeTuningRow : public FTableRowBase
{
    float AltitudeAlvo = 300.f;
    float PopHeight = 150.f;
    float DecayFactor = 0.85f;
    int32 MaxJuggleHits = 7;
    float HangTime = 0.35f;
};
```

Abilities leem a row `Default` no `BeginPlay`. Prova o pipeline [44](systems/44_Data_Driven.md) antes do M0.

---

## 5. Checklist de aceite

- [ ] Player e alvo sobem juntos no launcher (follow launch)
- [ ] Divergência Z < 30 cm durante 5+ hits
- [ ] Juggle termina (alvo cai) antes do abuse
- [ ] Câmera mostra ambos sem perder leitura
- [ ] Valores tunáveis via `DT_SpikeTuning` sem recompile
- [ ] Vídeo de 30s anexado à decisão go/no-go
- [ ] Decisão registrada: **GO M2** ou **PIVOT** (câmera / sem follow launch)

---

## 6. Resultados possíveis e pivots

| Resultado | Ação |
|---|---|
| **GO** | Seguir roadmap M0→M2 com confiança no contrato RM |
| **Câmera falha** | Spike extra: zoom dinâmico em `InAir` ([06](systems/06_Camera_TopDown.md)) |
| **Co-altitude falha** | Revisar RM ancorado vs. kinematic controller; consultar [16 §9](combat/16_Aerial_Combos.md) |
| **Leitura falha** | Adicionar silhueta/outline no ar ([21](feel/21_Juice_FX.md)) — ainda no spike |

---

## 7. O que descartar depois

| Asset spike | Destino pós-spike |
|---|---|
| `L_Spike_Aerial` | Arquivar ou deletar |
| `BP_Spike_*` | Deletar |
| `GA_Launcher` spike | **Migrar** lógica RM para GA production |
| `DT_SpikeTuning` | **Migrar** rows para `DT_AttackData` / attributes |
| Tags `State.Combat.*` | **Manter** — vão para produção ([41](systems/41_Gameplay_Tags.md)) |

---

## 8. Próximo passo

| Se GO | Se PIVOT |
|---|---|
| [04 — Setup](systems/04_Project_Setup.md) → M0 | Atualizar [16](combat/16_Aerial_Combos.md) + re-spike |
| [05 — GAS](systems/05_GAS_Architecture.md) | — |
| [17 — Roadmap M0](17_Implementation_Roadmap.md) | — |
