# 34 — Mini-Boss · 🟡 P1 (M4)

> O **finale da run** ([02 M4](../design/02_MVP_Scope.md), [03 §2](../design/03_Core_Loop_Roguelike.md)): um inimigo maior, multi-fase, com ataques-assinatura telegrafados. É o clímax que valida a build do jogador. Pré-req: [30-32](30_AI_Overview.md), [28 §1 — boss bar](../ui/28_HUD.md).

> 🎯 **O boss é o teste da build.** Depois de pegar Ecos a run inteira, o player chega aqui e descobre se sua build funciona. Um bom mini-boss é **legível, justo e derrotável com skill+build** — não um saco de HP.

---

## 1. O que muda de um inimigo normal

| Aspecto | Inimigo comum | Mini-boss |
|---|---|---|
| **HP / Poise** | baixo | **alto** (mas não infinito) |
| **Fases** | nenhuma | **2** (muda em ~50% HP) |
| **Ataques** | 1-2 | **2-3 assinatura**, telegrafados |
| **Launch/juggle** | livre (trash) | só em **janela de vulnerabilidade** (§4) |
| **HUD** | nada/flutuante | **barra dedicada** ([28 §1](../ui/28_HUD.md)) |
| **Arena** | compartilhada | **sala própria** (espaço pra esquivar) |

> Reusa **tudo** do [doc 31/32](31_Enemy_Archetypes.md): é um `ADDREnemyCharacter` com `UDDREnemyData` de boss (stats altos, abilities próprias, BT/StateTree de boss). Não é uma classe nova — é dados + um BT mais rico.

---

## 2. Fases (multi-stage)

```
Fase 1 (100%→50% HP):  2 ataques telegrafados — o player APRENDE os padrões
        ↓ (50% HP — transição: rugido/telegrafe + breve invuln)
Fase 2 (50%→0%):       + 1 ataque novo, ataques mais rápidos/encadeados — o TESTE
```

- A transição de fase é um **momento** (telegrafe claro, pequena pausa) — dá respiro e sinaliza "ficou mais sério".
- **MVP: 2 fases.** Mais fases = pós-MVP.

---

## 3. Ataques-assinatura (telegrafe é vida ou morte)

Os ataques do boss **machucam muito** → o telegrafe ([32 §1](32_Enemy_Combat_Behavior.md)) é ainda **mais crítico**. Cada ataque-assinatura precisa de:

| Elemento | Exemplo |
|---|---|
| **Decal de zona claro** | linha/cone/círculo vermelho grande no chão |
| **Windup longo e legível** | ~0.6-1.0s (mortal = mais tempo de leitura) |
| **Padrão reconhecível** | o player aprende a ler e reagir (dash/parry) |
| **Recovery punível** | a janela pra atacar/combar o boss |

**Ataques MVP (3):**
1. **Investida telegrafada** (linha) — dash pra fora.
2. **AoE em área** (círculo) — sair do círculo.
3. **(Fase 2) Combo de 2-3 golpes** — esquivar em sequência.

> 🚩 Boss sem telegrafe legível = morte injusta = o jogador culpa o jogo, não a si mesmo. O telegrafe é o contrato de justiça do boss.

---

## 4. Pode combar o boss? (a recompensa do pilar)

Tensão de design: o pilar é o combo aéreo, mas um boss **não pode voar à vontade** (trivializa) nem ser **imune** (frustra / desperdiça o pilar). Solução — **janela de vulnerabilidade**:

```
Boss tem poise alto + tag Faction.Boss.NoLaunch normalmente (18 §5.4).
   → ao tomar X dano / quebrar poise / fim de um ataque grande:
       → entra em STAGGER ("aberto!") por uns segundos
       → remove NoLaunch → o player PODE lançar e combar no ar
   → janela fecha → volta a resistir
```

> 🥊 Isso transforma o combo aéreo na **recompensa de ler o boss**: você sobrevive aos padrões, abre a janela, e **despeja a build** num juggle. É o clímax do clímax. Hyperarmor ([32 §3](32_Enemy_Combat_Behavior.md)) garante que o boss não cambaleia fora da janela.

---

## 5. Arena & HUD

- **Sala própria**, maior, com espaço pra esquivar (e talvez 1 hazard ambiental — P2).
- **Barra de boss** no topo-centro do HUD ([28 §1](../ui/28_HUD.md)) — única que ocupa a faixa superior (boss é a exceção à "zona sagrada").
- Sem outros inimigos (ou poucos adds em fase 2 — P1).

---

## 6. Recompensa (fim da run)

Matar o boss = **fim da run vitorioso** ([29 §4](../ui/29_Run_Flow_Menus.md)):
- Tela de vitória + resumo + **Essência** (mais que uma morte comum — recompensa o feito).
- Volta ao Hub mais forte ([03 §5](../design/03_Core_Loop_Roguelike.md)).

---

## 7. Prioridade MVP

| Item | Prioridade |
|---|---|
| 1 mini-boss (DataAsset de boss + BT) | 🟡 P1 (M4) |
| 2 fases (transição em 50%) | 🟡 P1 |
| 2-3 ataques-assinatura **telegrafados** | 🟢 P0 (do boss) |
| Janela de vulnerabilidade (poder combar) | 🟡 P1 (recompensa do pilar) |
| Barra de boss no HUD | 🟡 P1 |
| Hazards de arena, adds, 3+ fases | 🔵 P2 |

---

## 8. Checklist

- [ ] Mini-boss = `ADDREnemyCharacter` + `UDDREnemyData` de boss (não classe nova)
- [ ] 2 fases (transição telegrafada em ~50% HP)
- [ ] 2-3 ataques-assinatura com **decal de telegrafe forte**
- [ ] **Janela de vulnerabilidade** (stagger remove `NoLaunch` → combável)
- [ ] Hyperarmor fora da janela (não cambaleia)
- [ ] Barra de boss no HUD ([28 §1](../ui/28_HUD.md))
- [ ] Kill → tela de vitória + Essência ([29 §4](../ui/29_Run_Flow_Menus.md))

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Boss "saco de HP" sem graça | Sem fases/padrões/janela | §2-4 |
| Morte injusta no boss | Telegrafe fraco | §3 — decal forte + windup longo |
| Combo aéreo inútil no boss | `NoLaunch` permanente | §4 — janela de vulnerabilidade |
| Boss trivial com o combo | Sem hyperarmor / lança à vontade | §3/§4 — poise alto + NoLaunch fora da janela |
| Transição de fase confusa | Sem telegrafe/respiro | §2 |

---

## 10. Próximo passo

Cluster de inimigos completo. → [Roadmap M3-M4](../17_Implementation_Roadmap.md) (IA + arena → run + boss) · volte ao [Index](../00_Index.md).
