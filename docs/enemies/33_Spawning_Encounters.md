# 33 — Spawning & Encontros · 🟢 P0 (M3)

> Como os inimigos **entram na arena**: o spawn director, as ondas, a colocação, a escalada de dificuldade, e o "token director" anti-overwhelm. Pré-req: [30](30_AI_Overview.md), [03 §3 — Core Loop](../design/03_Core_Loop_Roguelike.md).

> 🎲 **"Procedural sem procedural" ([03 §2](../design/03_Core_Loop_Roguelike.md)):** encontros são **templates montados à mão e embaralhados**, não gerados. Dá variação sem escrever um gerador.

---

## 1. Anatomia de uma sala de combate (recap)

Do [03 §3](../design/03_Core_Loop_Roguelike.md):

```
1. Player entra → portas TRANCAM.
2. Spawn da(s) onda(s).
3. Player limpa (combate — combo de chão + aéreo).
4. Última onda morre → portas ABREM + recompensa (Eco, doc 29 §3).
5. Avança.
```

O **Encounter Manager** (por arena) orquestra isso.

---

## 2. Encounter Manager (o diretor da sala)

```
ADDREncounterManager (Actor na arena):
  • Waves: TArray<FDDRWave>   (cada wave = lista de {EnemyData, count, spawnPoint})
  • OnPlayerEnter → tranca portas → inicia Wave 0
  • conta inimigos vivos → quando = 0 → próxima wave (ou fim)
  • fim das waves → abre portas + dispara recompensa (RunManager, doc 03)
  • dono do TOKEN DE ATAQUE global (§5)
```

```cpp
USTRUCT() struct FDDRWave {
  TArray<FDDRSpawnEntry> Spawns;   // {UDDREnemyData*, count, spawnPointTag}
  float DelayAntes = 0.f;          // respiro entre ondas
  int32 MaxAtivos = 8;             // teto simultâneo (perf + leitura)
};
```

---

## 3. Colocação de spawn (não spawnar no colo do player)

| Regra | Por quê |
|---|---|
| **Spawn points** marcados na arena (não aleatório no chão) | controle de design; evita spawn em parede |
| **Longe do player** (ou fora de vista) | spawnar no colo é injusto |
| **Telegrafe de spawn** (decal/VFX antes do inimigo aparecer) | leitura: o player vê de onde vem ([21](../feel/21_Juice_FX.md)) |
| **Escalonado**, não tudo de uma vez | a wave "entra" em ritmo, não num blob |

> 🚩 Reusa o princípio do telegrafe ([32 §1](32_Enemy_Combat_Behavior.md)): até o *spawn* é telegrafado em topdown, senão inimigos "aparecem do nada" e matam.

---

## 4. Escalada de dificuldade (data-driven)

A dificuldade sobe com a **profundidade da run** (sala 1 → boss). Expr. em dados, não hardcode:

| Variável | Sala 1 | Sala 4 | Como |
|---|---|---|---|
| Nº de inimigos | 4 | 8 | por wave/sala |
| Nº de ondas | 1 | 2 | |
| Mix | só grunt | grunt + atirador + 1 elite | proporção |
| Chance de elite | 0% | ~30% | rolagem |

> 📐 Números são chutes ([03 §6](../design/03_Core_Loop_Roguelike.md)) — calibre no playtest do M4. Exponha como curva/tabela por profundidade.

---

## 5. 🎟️ Token director (anti-overwhelm) — vive aqui

O "token de ataque" do [32 §5](32_Enemy_Combat_Behavior.md) é gerenciado pelo Encounter Manager (ele vê todos os inimigos da sala):

```
A cada N inimigos na sala, só MaxAtacantes (1-2) recebem "token de ataque".
   • inimigo pede token antes de entrar no estado Atacar (BT)
   • sem token → fica em Reposicionar (circunda/flanqueia)
   • token volta ao pool quando o ataque termina
```

> 🎯 É o que faz "cercado por 8" sentir **pressão justa** e não **morte instantânea**. Um dos ajustes mais importantes do feel de combate em massa.

---

## 6. Pacing (respiro entre ondas)

- **Delay entre ondas** (`DelayAntes`): 1-2s pro player reposicionar, pegar fôlego, ver a próxima entrar.
- **Não** encadeie waves instantâneas — o respiro é parte do ritmo (ataca → limpa → respira → próxima).
- Sala final antes do boss: pode ser uma "elite room" (poucos elites) pra variar o pacing.

---

## 7. "Procedural sem procedural" (variação barata)

- Monte **6-8 arenas** à mão, cada uma com seus spawn points + templates de wave — playbook em **[52 — Arena & Level Design](../world/52_Arena_Level_Design.md)**.
- Sorteie 4 por run ([03 §2](../design/03_Core_Loop_Roguelike.md)).
- Varie o **mix** de inimigos por sorteio dentro de um template.
- Resultado: cada run sente diferente **sem** um gerador procedural. Procedural de verdade = pós-MVP.

---

## 8. Prioridade MVP

| Item | Prioridade |
|---|---|
| `ADDREncounterManager` (portas + waves + conta vivos) | 🟢 P0 (M3) |
| Spawn points + colocação longe do player | 🟢 P0 |
| **Token director** (anti-overwhelm) | 🟢 P0 |
| Escalada por profundidade (data) | 🟡 P1 (M4) |
| Telegrafe de spawn | 🟡 P1 |
| Respiro entre ondas (pacing) | 🟡 P1 |
| 6-8 arenas embaralhadas | 🟡 P1 (M4) |

---

## 9. Checklist

- [ ] `ADDREncounterManager`: entra → tranca → ondas → limpa → abre + recompensa
- [ ] `FDDRWave` data-driven (EnemyData + count + spawnPoint)
- [ ] Spawn longe do player + telegrafado
- [ ] **Token de ataque** gerenciado aqui (1-2 atacam por vez)
- [ ] Respiro entre ondas (1-2s)
- [ ] Escalada de dificuldade por profundidade (P1)
- [ ] Conecta com RunManager → recompensa/Eco ([03](../design/03_Core_Loop_Roguelike.md), [29 §3](../ui/29_Run_Flow_Menus.md))

---

## 10. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Inimigos aparecem no colo do player | Spawn sem regra de distância | §3 |
| Portas não abrem | Contagem de vivos errada | §2 — conta no death do inimigo |
| Massacre ao spawnar wave | Sem token / tudo ataca | §5 |
| Sala sente "blob" sem ritmo | Sem respiro/escalonamento | §3, §6 |
| Runs sentem iguais | Sem embaralhar arenas/mix | §7 |

---

## 11. Próximo passo

→ [52 — Arena & Level Design](../world/52_Arena_Level_Design.md) · [51 — Catálogo Inimigos](51_Enemy_Catalog_MVP.md) · [42 — Run & Room Manager](../systems/42_Run_Room_Manager.md) · [34 — Mini-Boss](34_MiniBoss.md).
