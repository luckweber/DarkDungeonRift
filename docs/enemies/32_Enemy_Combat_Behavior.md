# 32 — Comportamento de Combate do Inimigo · 🟢 P0 (M3)

> Como o inimigo **luta**: telegrafe (a leitura topdown), padrões de ataque (windup/active/recovery), hyperarmor, poise como porta do combo, e o **spacing** (não grudar/não atacar todos de uma vez). Pré-req: [30](30_AI_Overview.md), [31](31_Enemy_Archetypes.md), [18 §5](../combat/18_Combat_System_Deep.md).

---

## 1. 🚩 Telegrafe = leitura topdown (P0, o item nº1)

O combate só é **justo** se o jogador **lê o perigo antes** dele acontecer ([Pilar 4](../design/01_Game_Vision.md)). Em topdown a -55°/950, a animação de windup sozinha **não basta** — o jogador olha o chão. O [doc 21 (Vi)](../feel/21_Juice_FX.md) cravou: **telegrafe = juice defensivo P0.**

| Camada do telegrafe | O que faz |
|---|---|
| **Decal no chão** (zona de perigo) | 🟢 **Obrigatório** — mostra ONDE o golpe vai pegar (cone/círculo/linha vermelha pulsante) |
| **Windup animado** | o inimigo "carrega" o golpe (postura) |
| **Cor/flash** | o inimigo brilha na cor do ataque durante o windup |
| **Áudio** | um "tell" sonoro distinto por ataque |
| **Tempo de leitura** | windup longo o bastante pra reagir (~0.4-0.8s; mais pra golpes mortais) |

```
Ataque do inimigo (GameplayAbility):
  [WINDUP/telegrafe: decal no chão + flash + som]  ← janela de reação do player
        ↓
  [ACTIVE: hitbox ligada]  ← aqui machuca
        ↓
  [RECOVERY: vulnerável]   ← a JANELA do player atacar/combar
```

> 🎯 **Sem decal no chão, o combate topdown é injusto** — o jogador morre sem entender. Trate o telegrafe com o **mesmo carinho** dos hits do player. É o que separa "difícil justo" de "barulho frustrante".

---

## 2. Padrões de ataque (frame data simétrico)

O ataque do inimigo usa a **mesma estrutura** do player ([18 §1](../combat/18_Combat_System_Deep.md)): `FDDRAttackData` com windup/active/recovery, hitbox, poise damage. Telegrafe = a fase de windup, esticada pra leitura.

| Fase | Inimigo | Oportunidade do player |
|---|---|---|
| **Windup** | telegrafa (§1) | **ler** → dash/parry/reposicionar |
| **Active** | hitbox liga | evitar (i-frame do dash, [19](../combat/19_Abilities_Deep.md)) |
| **Recovery** | vulnerável, lento | **atacar / combar / lançar** ← a janela |

> 🥋 A **recovery vulnerável** é o *convite* do inimigo: é quando o player pune e inicia o combo. Recovery curto demais = sem abertura (frustra); longo demais = trivial. Tune por arquétipo.

---

## 3. Hyperarmor — golpes comprometidos não cambaleiam

Durante certos ataques telegrafados, o inimigo ganha `State.Combat.HyperArmor` ([18 §5.2](../combat/18_Combat_System_Deep.md)): **ignora flinch** (não é interrompido por hits leves) mas **ainda toma dano e hitstop**.

- **Por quê:** ensina o player a **não dar mash** num inimigo carregando golpe — tem que **esquivar/aparar** ([Review de Combate](../design/Design_Review_Combat_2026-06.md)). Sem hyperarmor, todo telegrafe é cancelável e o telegrafe perde o sentido.
- **Onde:** golpes fortes/elites/boss. Trash com golpe rápido não precisa.
- **Tell:** o hitstop ao bater contra hyperarmor (sem cambalear) é o sinal visual de "este vai sair, recue" ([18 §5.2](../combat/18_Combat_System_Deep.md)).

---

## 4. Poise como porta do combo (a ponte pro pilar)

[18 §5.2/§5.4](../combat/18_Combat_System_Deep.md): o inimigo só é **lançável** quando o **poise quebra**.

```
Player acumula hits → Poise do inimigo cai → Poise<=0 → STAGGER (vulnerável)
   → agora CanLaunch passa (18 §5.4) → launcher pega → combo aéreo
```

- **Trash (poise baixo):** quebra em 1-2 hits → lança quase imediato (fluidez do pilar).
- **Elite/Brutamontes (poise alto):** exige um combo de chão pra abrir → **então** lança (profundidade).

> 🔗 É o **toma-lá-dá-cá** que faz "quebrar a guarda" sentir uma conquista. O poise é o ritmo do combate ([18 §5.2](../combat/18_Combat_System_Deep.md)).

---

## 5. Spacing & o "token de ataque" (anti-overwhelm)

Se **todos** os inimigos atacam ao mesmo tempo, o jogador é massacrado sem chance de ler nada. Truque AAA clássico (character-action, Hades):

> 🎟️ **Token de ataque global:** só **N inimigos** (ex.: 1-2) podem estar no estado *Atacar* ao mesmo tempo. Os outros **circundam/posicionam** esperando o token. **Dono do token: o Encounter Manager** ([33 §5](33_Spawning_Encounters.md)) — ele vê todos os inimigos da sala. (Um "combat director" dedicado só se a IA crescer, P2.)

| Comportamento | Efeito |
|---|---|
| **Token de ataque** (1-2 por vez) | combate legível e justo mesmo cercado |
| **Reposicionar** (sem token) | inimigo circunda/flanqueia, não gruda parado |
| **Ranged mantém distância** | atirador recua se o player chega perto (kite) |
| **Respiro entre ataques** | janelas pro player atacar/reposicionar |

> 🎯 **Cercado ≠ atacado por todos.** O jogador deve sentir *pressão* (cercado) mas ter *turno* (poucos atacam por vez). Sem token de ataque, a horda topdown vira morte injusta.

---

## 6. Reação ao player (defesa do inimigo — leve no MVP)

- **Recuar de combo:** ao tomar muito hit, tentar criar distância (se não estiver em stagger/airborne).
- **Elite:** pode ter 1 golpe de "afastamento" (empurra o player) — telegrafado.
- **MVP:** mantenha a defesa do inimigo **simples** — a graça é o player ser agressivo. IA defensiva demais trava o pilar.

---

## 7. Prioridade MVP

| Item | Prioridade |
|---|---|
| **Telegrafe com decal no chão** | 🟢 **P0** (leitura/justiça) |
| Windup→active→recovery (frame data) | 🟢 P0 |
| Recovery vulnerável (a janela do player) | 🟢 P0 |
| Poise como porta do launch | 🟢 P0 |
| **Token de ataque** (anti-overwhelm) | 🟢 P0 |
| Hyperarmor em golpes fortes | 🟡 P1 |
| Reposicionar/flanquear (EQS) | 🟡 P1 |

---

## 8. Checklist

- [ ] Todo ataque inimigo tem **decal de telegrafe no chão**
- [ ] Windup com tempo de reação (~0.4-0.8s)
- [ ] Recovery vulnerável (janela pro player)
- [ ] Poise quebra → stagger → lançável ([18 §5.4](../combat/18_Combat_System_Deep.md))
- [ ] **Token de ataque** (1-2 atacam por vez)
- [ ] Hyperarmor em golpes comprometidos (P1)
- [ ] Ranged faz kite (mantém distância)

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| "Morri sem ver o golpe" | Telegrafe fraco / só animação | §1 — decal no chão obrigatório |
| Massacrado ao ser cercado | Todos atacam juntos | §5 — token de ataque |
| Mash interrompe tudo (sem tensão) | Sem hyperarmor | §3 |
| Não consigo lançar o elite | Poise alto + combo curto | §4 — trabalhar o poise primeiro (by design) |
| Inimigo gruda parado | Sem reposicionar | §5 |

---

## 10. Próximo passo

→ [33 — Spawning & Encontros](33_Spawning_Encounters.md): como os inimigos entram na arena (ondas, dificuldade, token director).
