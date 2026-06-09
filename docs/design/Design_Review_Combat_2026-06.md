# Revisão de Design — Roundtable de Combate 2026-06

> Segunda mesa-redonda, focada em **combate, feel & juice** (combo, parry, hits, dash/dodge). 4 agentes-especialistas leram o cluster de combate (docs 15/16/18/19/20/21) + o inventário de animação (22). Preserva o **porquê** das mudanças. Companheiro do [Design Review 2026-06](Design_Review_2026-06.md) (revisão macro).

> 🔑 **Gatilho desta revisão:** o [inventário de animação (22)](../22_Animation_Inventory.md) revelou assets reais (Parry, Dodge, Roll, Executions, Charge) — permitindo re-debater "o que entra no MVP" com fatos, não suposições.

---

## 1. O painel

| Cadeira | Persona | Lente |
|---|---|---|
| 🥋 Ofensa | **Kael** | Character-action (DMC): combo, cancelamento, launcher |
| 🛡️ Defesa | **Rha** | Counterplay (fighting-game/Sekiro): parry, esquiva, risco-recompensa |
| ✋ Feel | **Nø** | Cinestésica: o *tato* de bater/esquivar/aparar |
| 👁️ Juice | **Vi** | FX & **legibilidade topdown**: o *eco* sensorial |

---

## 2. Posições (resumo)

- **Kael:** "Lindo *sistema de hits* à espera de um pilar. **Sem launcher não há combo aéreo** — traga-o pro spike M⁻¹." Parry só quando entrar, e **ofensivo** (parry→launch).
- **Rha:** "A defesa está terceirizada pro dash → vira *mash + i-frame spam*. **Perfect-dodge no M1** é o melhor ROI: um `if` no pipeline que já existe. Parry é barato → P1, não P2." Expõe 2 buracos: falta **dodge-offset** e **player flinch**.
- **Nø:** "**Dash-cancel universal/instantâneo = critério de aceite do M1.** i-frame generoso (perdão) briga com perfect-dodge (timing) — resolva com 2 camadas." Três esquivas = crise de identidade tátil → colapse pra uma.
- **Vi:** "O calcanhar é a **leitura**, não o impacto. Follow-launch pode virar massa ambígua — teste legibilidade no spike antes de qualquer juice aérea. Flash > partícula na horda; telegrafe de inimigo = juice defensivo P0."

---

## 3. Consenso forte

1. **UMA esquiva no MVP** (Kael+Rha+Nø): `Dodge→GA_Dash`, **Roll cortado**.
2. **Launcher é o gap crítico** → pro **spike M⁻¹** com `IA_Launcher` dedicado (não reciclar o 4º golpe).
3. **Parry não é sistema no MVP** (os 4) — mas é barato e ofensivo (debate 2).
4. **Follow-launch legível = risco existencial** → resolvido só no spike (cubos coloridos + sombra).
5. **Dash-cancel universal = a espinha** — todos dependem dele.

---

## 4. Debates

| # | Tensão | Resolução adotada |
|---|---|---|
| 1 | **Perfect-dodge no M1?** Rha puxa (barato, alto ROI) · Nø resiste (i-frame generoso mata o "perfect") · Vi confirma (cue já existe, órfã) | **SIM no M1.** i-frame em **2 camadas**: generoso/front-loaded (perdão) + **sub-janela "perfect"** nos 1ºs frames (timing→witch-time). Reconcilia feel × counterplay. |
| 2 | **Parry P2 ou P1?** | **P2→P1**, sabor **ofensivo** (parry→stagger→launch), com **tick tátil** (Nø) + **flash visível** (Vi). Depois do loop básico, não no core. |
| 3 | **Follow-launch total vs parcial** (Kael × Vi × Nø: presença × leitura) | **Não resolvida** — o spike M⁻¹ testa total/parcial/só-alvo com cubos coloridos + sombra. |
| 4 | **Buraco defensivo** (Rha→Kael): toda janela de cancel é ofensiva | Registrar 2 gaps P1: **player flinch** (pune ganância; usa os 49 clips de Hit) + **dodge-offset** (esquiva no meio do combo *sem perdê-lo*, estilo Bayonetta). |

---

## 5. Veredito → ações aplicadas

| # | Ação | Origem | Onde |
|---|---|---|:---:|
| 1 | **Launcher no spike M⁻¹** + `IA_Launcher` dedicado | Kael | [17 §1.5](../17_Implementation_Roadmap.md), [16 §2](../combat/16_Aerial_Combos.md), [22 §4](../22_Animation_Inventory.md) |
| 2 | **UMA esquiva** (Dodge→GA_Dash; Roll cortado) | Kael+Rha+Nø | [22 §7](../22_Animation_Inventory.md), [19](../combat/19_Abilities_Deep.md), [02](02_MVP_Scope.md) |
| 3 | **Perfect-dodge no M1** + i-frame 2 camadas | Rha+Vi | [19](../combat/19_Abilities_Deep.md), [20 §4.1](../feel/20_Game_Feel.md), [21](../feel/21_Juice_FX.md), [17 M1](../17_Implementation_Roadmap.md) |
| 4 | **Parry P2→P1** ofensivo (parry→stagger→launch) | Rha+Kael | [22 §5](../22_Animation_Inventory.md), [19](../combat/19_Abilities_Deep.md), [17](../17_Implementation_Roadmap.md) |
| 5 | **Player flinch + dodge-offset** (gaps P1) | Rha | [18](../combat/18_Combat_System_Deep.md) |
| 6 | **Dash-cancel universal = critério de aceite M1** (testar c/ `ddr.CombatDebug`) | Nø | [17 M1](../17_Implementation_Roadmap.md), [20 §6](../feel/20_Game_Feel.md) |
| 7 | **Regras de juice/leitura topdown** (flash>partícula; numbers só crit/slam; FX respeitam time-dilation; FX no chão; telegrafe=P0) | Vi | [21](../feel/21_Juice_FX.md) |

---

## 6. Perguntas abertas (pro spike / playtest)

- ❓ **Follow-launch total vs parcial** — presença (feel) vs leitura (juice). Spike M⁻¹.
- ❓ **Frame exato em que o dash-cancel abre no startup** — Nø quer o número (cancel-on-whiff não pode virar jaula).
- ❓ **Quantos frames o input buffer perdoa** — generoso demais tira o dente do "cancel-on-hit"; curto demais o combo foge (Kael→Nø).

---

## 7. Meta-lição

O combate estava **forte na ofensa, fino na defesa**: grafo de combo + cancelamento + juggle ricos, mas a camada defensiva inteira terceirizada pro dash. A revisão **não pediu mais sistemas** — pediu *baratos e ofensivos*: perfect-dodge (um `if`), parry-que-vira-launch, flinch (usa assets que já existem). E reforçou que, em topdown, **a leitura é o gargalo de tudo** — o spike M⁻¹ com launcher real é onde o pilar se prova ou morre.
