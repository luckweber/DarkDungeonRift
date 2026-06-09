# 47 — Tipos de Sala & Mapa da Run (Variedade & Rota) · 🟡 P1

> Detalha as mecânicas **A (variedade de salas)** e **B (preview de porta)** do [46 — Profundidade](46_Depth_Replayability.md). Transforma o corredor de combate em **rota com escolha**. Estende o [42 — Run Manager](../systems/42_Run_Room_Manager.md). Lore: [45](45_World_Lore.md).

> 🎯 **Por que é a mecânica de maior alavanca P1:** variedade + escolha de rota é o que faz cada run sentir **diferente e sua** — sem escrever um gerador procedural. É o coração do "só mais uma" do Hades/Dead Cells.

---

## 1. Os tipos de sala

Estende `FDDRRoomDefinition` ([42 §3](../systems/42_Run_Room_Manager.md)) com um `ERoomType`:

| Tipo | Ícone | O que faz | Recompensa | Lore ([45](45_World_Lore.md)) |
|---|---|---|---|---|
| **Combate** | ⚔️ | ondas normais ([33](../enemies/33_Spawning_Encounters.md)) | Eco (1 de 3) | câmara da Fenda |
| **Elite** | 💀 | 1-2 inimigos fortes (poise alto) | **Eco garantido Raro+** | guardião menor |
| **Tesouro** | 💰 | sem combate | Essência/ouro/Eco grátis | veio de Essência |
| **Loja** | 🪙 | gasta **Fragmentos** in-run ([53 §3](53_In_Run_Economy_Shop.md)) | comprar Eco/cura/reroll | um caído mercador |
| **Santuário** | ⛧ | risco/recompensa (oferta com custo) | Eco Lendário ↔ maldição | altar de um caído |
| **Descanso** | 🔥 | cura + subir um Eco de tier | sustain/build | brecha calma na Fenda |
| **Evento** | ❓ | escolha narrativa ([49](49_Codex_Limiar.md)) | variável | encontro com a Voz |
| **Boss** | 👑 | mini-boss ([34](../enemies/34_MiniBoss.md)) | fim da run + Essência | Guardião da camada |

```cpp
UENUM(BlueprintType)
enum class EDDRRoomType : uint8
{
    Combat, Elite, Treasure, Shop, Shrine, Rest, Event, Boss
};
```

> 🎯 **MVP = 3 tipos:** Combate + **Elite** + **Tesouro**. Já quebram a monotonia com custo quase zero (reusam inimigos/recompensa existentes). Loja/Santuário/Descanso/Evento são P2.

---

## 2. 🔮 Preview de porta & escolha de rota (a joia)

No fim de cada sala, em vez de "próxima sala", **revele 2 portas** com **tipo + recompensa** e deixe escolher:

```
        SALA LIMPA → escolha a rota:
   ┌──── Porta Esquerda ────┐   ┌──── Porta Direita ────┐
   │  ⚔️  COMBATE           │   │  💀  ELITE            │
   │  recompensa: 🩸 Carniça │   │  recompensa: ⚡ Raro+  │
   │  perigo: ●●○            │   │  perigo: ●●●          │
   └────────────────────────┘   └───────────────────────┘
         [A] escolher esquerda      [D] escolher direita
```

| Elemento do card | Lê de |
|---|---|
| **Tipo** (ícone) | `ERoomType` da sala sorteada |
| **Recompensa prévia** | que **família** de Eco / Essência aquela porta dará |
| **Perigo** (●○) | `Depth` + tipo (elite = mais) |

> 🎯 **Isto transforma o corredor em decisão.** "Quero o Eco de Carniça pra fechar minha build, mas a porta elite é perigosa..." = **agência** = engajamento. É o que o Hades faz e quase nenhum roguelike topdown faz.

**Implementação:** o `RunManager` ([42 §4](../systems/42_Run_Room_Manager.md)) já sorteia salas — só **sorteie 2, mostre o preview, e deixe o jogador escolher** qual carregar. Pré-rola a recompensa pra mostrar.

> 🎰 **Reconciliação com "4-5 Ecos por run" ([03b §2](03b_Reward_System.md)):** salas de **Combate e Elite sempre ofertam Eco** (`bOffersEco`, [42 §3](../systems/42_Run_Room_Manager.md)); Tesouro/Loja dão **outras** recompensas (Essência/ouro). Numa run de ~4 salas com maioria Combate/Elite, o jogador pega ~3-4 Ecos — bate com a meta. Quem escolhe rota de tesouro **troca Ecos por recurso** (é a decisão de build, não um bug de contagem).

---

## 3. Estrutura do mapa da run

Em vez de linha reta embaralhada, um **grafo raso** de 2 portas por nó:

```
              ┌─ ⚔️ ─┐         ┌─ 💰 ─┐
[Início] ──┬──┤      ├──┬──────┤      ├──┬── 👑 BOSS
           └──┤ 💀 │  └──┐  ┌──┤ ⛧ │  └──┐
              └──────┘     └──┴──────┘     (rotas convergem no boss)
            sala 1        sala 2        sala 3...
```

- Cada "profundidade" oferece **2 escolhas**; as rotas convergem no boss.
- **MVP simples:** nem precisa de grafo visual — só o **preview de 2 portas** por transição já entrega a escolha. O mini-mapa de grafo é P2.

---

## 4. Pacing — distribuição dos tipos pela run

Não é 100% aleatório; há **regras de ritmo** (data-driven em `DA_RunConfig`):

| Regra | Por quê |
|---|---|
| **Loja a cada ~3 salas** | dá o que fazer com o ouro acumulado |
| **Descanso antes do boss** | respiro + ajuste final de build |
| **Elite garante Eco Raro+** | recompensa o risco |
| **Nunca 2 santuários seguidos** | evita sobrecarga de decisão |
| **1ª sala sempre Combate** | onboard limpo |

```cpp
// DA_RunConfig — pacing data-driven
struct FDDRRunPacing {
    int32 ShopEveryNRooms = 3;
    bool  bRestBeforeBoss = true;
    float EliteChance = 0.25f;     // por profundidade
    float ShrineChance = 0.15f;
};
```

---

## 5. Tipos de sala em detalhe (os mais ricos)

### ⛧ Santuário (risco/recompensa) — P2
Uma **oferta com custo** (lore: pacto com um caído, [45 §4](45_World_Lore.md)):
- "Sacrifique **30% da vida máx** → Eco Lendário garantido."
- "Aceite a **maldição** (inimigos +20% dano nesta run) → +50% Essência."
- Aplica via `GE` na run (atributo/tag). É decisão memorável = run memorável.

### 🔥 Descanso — P2
- Cura X% da vida.
- OU **sobe um Eco** já pego de tier ([46 §F](46_Depth_Replayability.md)) — aprofunda a build.
- Lore: uma brecha calma onde a Fenda "respira".

### ❓ Evento — P2
- Encontro narrativo ([49](49_Codex_Limiar.md)): a Voz da Fenda oferece um dilema → escolha com consequência (Eco, maldição, Essência).
- Texto barato ([38 FText](../systems/38_Localization.md)), alto charme.

---

## 6. Integração (zero reescrita)

| Peça | Encaixa em |
|---|---|
| `ERoomType` | `FDDRRoomDefinition` ([42 §3](../systems/42_Run_Room_Manager.md)) |
| Lógica por tipo | `RunManager` switch no `LoadRoom` |
| Preview de 2 portas | novo estado `RoomSelect` na máquina de run ([42 §2](../systems/42_Run_Room_Manager.md)) |
| Recompensa prévia | reusa `RollEcoOptions` ([42 §4](../systems/42_Run_Room_Manager.md)) — só rola antes |
| Ouro/loja | atributo `Gold` (run) + UI de loja |
| Pacing | `DA_RunConfig` ([44](../systems/44_Data_Driven.md)) |

---

## 7. Prioridade

| Item | Prioridade |
|---|---|
| `ERoomType` + 3 tipos (Combate/Elite/Tesouro) | 🟡 P1 |
| **Preview de 2 portas + escolha** | 🟡 P1 (a joia) |
| Pacing data-driven (`DA_RunConfig`) | 🟡 P1 |
| Loja + ouro in-run | 🔵 P2 |
| Santuário / Descanso / Evento | 🔵 P2 |
| Mini-mapa de grafo visual | 🔵 P2 |

---

## 8. Checklist

- [ ] `EDDRRoomType` + 3 tipos no MVP (Combate/Elite/Tesouro)
- [ ] Estado `RoomSelect` no RunManager ([42 §2](../systems/42_Run_Room_Manager.md))
- [ ] **Preview de 2 portas** (tipo + recompensa + perigo)
- [ ] Recompensa pré-rolada por porta
- [ ] Pacing em `DA_RunConfig` (loja/descanso/elite)
- [ ] Elite dá Eco Raro+ garantido
- [ ] Lore por tipo de sala ([45](45_World_Lore.md))

---

## 9. Próximo passo

→ [48 — Pacto/Heat](48_Pact_Heat.md) (dificuldade customizada) · [42 — Run Manager](../systems/42_Run_Room_Manager.md) · [46 — Profundidade](46_Depth_Replayability.md) · [49 — Codex/Eventos](49_Codex_Limiar.md).
