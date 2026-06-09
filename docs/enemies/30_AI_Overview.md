# 30 — IA de Inimigos: Visão Geral & Arquitetura · 🟢 P0 (M3)

> Arquitetura da IA dos inimigos: o `AIController`, **Behavior Tree vs State Tree**, percepção, integração com **GAS**, e o gancho mais importante: **a IA pausa quando o inimigo está sendo combado no ar**. Leia antes de 31-34.

> 🧭 **Contexto:** este cluster preenche o buraco que a [Revisão de Design (Sofia)](../design/Design_Review_2026-06.md) apontou — "IA está fora do escopo mas é pré-requisito do jogo de verdade". Entra no **M3** ([roadmap](../17_Implementation_Roadmap.md)). 🎮 **Single-player** → sem replicação de IA.

---

## 1. O inimigo é GAS, igual ao player

Decisão-chave: **inimigos usam o mesmo GAS** ([05](../systems/05_GAS_Architecture.md)) — ASC + AttributeSet + GameplayAbilities pros ataques. Por quê:

- O **pipeline de dano** ([18 §3](../combat/18_Combat_System_Deep.md)), **poise/stagger** ([18 §5](../combat/18_Combat_System_Deep.md)) e **hit detection** já funcionam pros dois lados — simétrico.
- O ataque do inimigo é uma `GameplayAbility` (telegrafe→hitbox→recovery), tocada via ASC — mesma arquitetura do player ([19](../combat/19_Abilities_Deep.md)).
- Os Boons/effects que mexem em dano funcionam contra inimigos sem código especial.

```
ADDREnemyCharacter : ADDRCharacterBase   (já tem ASC + AttributeSet, doc 05)
   + AIController (a "mente")
   + AttributeSet: Health, Poise (doc 18 §5.2), MoveSpeed
   + Abilities: GA_Enemy_MeleeAttack, GA_Enemy_RangedAttack, ...
```

---

## 2. Behavior Tree vs State Tree (a decisão técnica)

| Abordagem | Prós | Contras | Veredito |
|---|---|---|---|
| **Behavior Tree** (clássico) | Maduro, visual, toneladas de exemplos, decorators/services prontos | Pode virar "árvore espaguete" com muitos estados | 🟢 **MVP** — rápido e conhecido |
| **State Tree** (UE5 moderno) | Data-driven, mais limpo pra estados, integra com Mass | Mais novo, menos exemplos | 🟡 Alternativa moderna |

**Decisão MVP: Behavior Tree.** É o caminho mais rápido pra um inimigo simples (perseguir/telegrafar/atacar/recuar). State Tree é a evolução natural se a IA crescer. (Mesmo padrão da decisão bespoke-vs-MotionMatching da locomoção: comece simples, evolua se precisar.)

---

## 3. O loop de IA (a máquina mental do inimigo)

```
        ┌──────────── (perde o alvo) ─────────────┐
        ↓                                          │
[Idle/Patrulha] ──(percebe player)──→ [Perseguir] ─┤
                                          │        │
                                  (no alcance)     │
                                          ↓        │
                              [Atacar] = telegrafe → golpe → RECUPERAÇÃO
                                          │        ↑
                                  (recovery / cooldown)
                                          ↓        │
                                   [Reposicionar] ─┘
```

| Estado | O que faz | Doc |
|---|---|---|
| **Idle/Patrulha** | Espera / anda devagar até perceber o player | §5 |
| **Perseguir** | NavMesh até o player (mantém distância se ranged) | §4 |
| **Atacar** | Telegrafe → hitbox → **recovery vulnerável** | [32](32_Enemy_Combat_Behavior.md) |
| **Reposicionar** | Recua/flanqueia, dá respiro (não gruda) | [32 §5](32_Enemy_Combat_Behavior.md) |

---

## 4. Blackboard & Navegação

**Blackboard keys (MVP):**
| Key | Uso |
|---|---|
| `TargetActor` | o player (alvo) |
| `LastKnownLocation` | última posição vista (busca) |
| `bCanSeeTarget` | percepção |
| `AIState` | enum (Idle/Chase/Attack/Reposition) |
| `AttackRange` / `bInRange` | decide atacar |

**Navegação:** NavMesh na arena. `MoveTo` pro player (melee) ou pra um ponto a `DesiredRange` (ranged, mantém distância). EQS ([opcional]) pra achar bons pontos de flanqueio/reposição.

---

## 5. Percepção (aggro)

`AIPerception` com **Sight** (cone ou raio). Para ARPG topdown de arena fechada, o mais comum é **aggro por raio/arena**: ao entrar na sala (ondas spawnam, [33](33_Spawning_Encounters.md)), os inimigos já "sabem" do player. Sight refinado é polish.

- **MVP:** raio de aggro simples (ou aggro imediato ao spawnar na arena).
- **P1:** sight cone + last-known-location (busca quando perde de vista).

---

## 6. 🔥 O gancho CRÍTICO: IA pausa durante o juggle

Este é **o** ponto de integração IA ↔ pilar aéreo. Quando o inimigo é lançado/combado ([16 §3.1](../combat/16_Aerial_Combos.md), [18 §5.6](../combat/18_Combat_System_Deep.md)), **a IA não pode agir** — senão ele "ataca" flutuando no ar e quebra o combo.

```
Inimigo recebe tag State.Combat.Airborne (ou State.Combat.Stagger):
   → Behavior Tree SUSPENSO (a "mente" desliga)
   → controle passa pra state machine do ALVO (Knockback→Float→SlamDown→Stun, doc 16 §3.1)
   → tag sai (pós-slam/stun) → BT retoma
```

**Como implementar:**
- **Decorator/Service no BT** que checa `State.Combat.Airborne`/`Stagger` na raiz → aborta/pausa a árvore enquanto a tag existe.
- Ou: o AIController **para de tickar lógica** enquanto a tag estiver presente.

> ⚠️ **Sem este gancho, o combo aéreo não funciona** — o inimigo revidaria no meio do juggle. É a regra nº1 da IA deste jogo. Conecta diretamente com [16 §8](../combat/16_Aerial_Combos.md) e o **player flinch/poise** ([18 §5.6](../combat/18_Combat_System_Deep.md)).

---

## 7. Performance (horda)

ARPG roguelike pode ter densidade alta (referência: Death Must Die). Cuidados:

| Técnica | Quando |
|---|---|
| **AI tick por significância** (longe = tick raro) | muitos inimigos |
| **Animation Budget Allocator** (plugin) | muitos esqueletos animando |
| **Limitar nº de inimigos ativos** | encadeado com o spawn director ([33](33_Spawning_Encounters.md)) |
| **Mass Entity** (avançado) | hordas *enormes* (P2/pós-MVP) |

> No MVP (4-8 inimigos/sala, [03 §6](../design/03_Core_Loop_Roguelike.md)), BT normal aguenta. Otimização só quando a densidade subir.

---

## 8. Prioridade MVP

| Item | Prioridade |
|---|---|
| `ADDREnemyCharacter` (GAS) + AIController + BT | 🟢 P0 (M3) |
| Loop perseguir→atacar→recuar | 🟢 P0 |
| **Pausa no `Airborne`** (gancho do juggle) | 🟢 **P0** (sem isso o pilar quebra) |
| Aggro por raio/arena | 🟢 P0 |
| Sight cone + busca (last-known) | 🟡 P1 |
| EQS pra flanqueio/reposição | 🟡 P1 |
| State Tree, Mass, AI LOD | 🔵 P2 |

---

## 9. Checklist

- [ ] `ADDREnemyCharacter` com ASC + AttributeSet (Health/Poise)
- [ ] AIController + Behavior Tree + Blackboard
- [ ] Inimigo ataca via **GameplayAbility** (simétrico ao player)
- [ ] Loop Idle→Chase→Attack→Reposition
- [ ] **BT pausa em `State.Combat.Airborne`/`Stagger`** (gancho do juggle)
- [ ] Aggro por raio/arena (MVP)
- [ ] NavMesh na arena

---

## 10. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Inimigo "ataca" sendo combado no ar | Falta o gancho de pausa | §6 — decorator no `Airborne` |
| Inimigo não persegue | NavMesh ausente / sem TargetActor | §4 |
| Dano do inimigo não funciona | Ataque não é ability/GE | §1 — usar GAS |
| FPS cai com muitos inimigos | Sem AI LOD/budget | §7 |
| Inimigo gruda no player (sem respiro) | Sem estado Reposicionar | [32 §5](32_Enemy_Combat_Behavior.md) |

---

## 11. Próximo passo

→ [31 — Arquétipos de Inimigo](31_Enemy_Archetypes.md) (o roster) · [32 — Comportamento de Combate](32_Enemy_Combat_Behavior.md) (telegrafe, juggle victim).
