# 46 — Profundidade & Replay (auditoria + novas mecânicas) · 🟡 P1 / 🔵 P2

> **Verifica** os sistemas de run atuais e propõe as **mecânicas que viciam** — as que separam um roguelike "ok" de um "só mais uma run". Honesto sobre escopo: **o MVP não precisa de tudo isto.** É o mapa da camada de *retenção*. Pré-req: [03](03_Core_Loop_Roguelike.md), [03b](03b_Reward_System.md), [40](40_Eco_Pool_Catalog.md), [42](../systems/42_Run_Room_Manager.md), [45 — Lore](45_World_Lore.md).

---

## 1. ✅ Auditoria — o que os sistemas de run JÁ têm

| Sistema | Estado | Nota |
|---|---|---|
| **Loop da run** (sala→onda→Eco→boss→morte) | 🟢 Sólido | [42](../systems/42_Run_Room_Manager.md) — máquina de estados clara, data-driven |
| **Ecos / build** | 🟢 **Forte** | [40](40_Eco_Pool_Catalog.md) — 18 Ecos, 4 famílias, **sinergias**, raridade, "muda o combo" |
| **Escalada por profundidade** | 🟢 Bom | [42 §7](../systems/42_Run_Room_Manager.md) — inimigos + pesos de Eco por sala |
| **Meta-progressão** | 🟡 Ok | Essência → hub → **desbloqueia mecânica** (não só +%, [03b §7](03b_Reward_System.md)) |
| **"Procedural sem procedural"** | 🟢 Bom | arenas embaralhadas ([03 §2](03_Core_Loop_Roguelike.md)) |
| **Lore/identidade** | 🟢 Novo | [45](45_World_Lore.md) — Eco=eco, morte diegética, pilar aéreo diegético |

**Veredito:** a **espinha está pronta e boa**. O loop prova *fun* no M4-M5. O que falta é a **camada de retenção** — os ganchos que fazem voltar pela **20ª** run, não a 2ª.

---

## 2. ⚠️ O que falta pra VICIAR (os gaps de engajamento)

| Gap | Sintoma sem ele | Referência que resolve |
|---|---|---|
| **Toda sala é combate** | monotonia; sem "curiosidade do que vem" | Hades, Dead Cells, STS |
| **Sem escolha de rota** | run é um corredor; zero agência entre salas | **Hades (preview de porta)** |
| **Sem economia in-run** | nenhuma decisão de recurso momento-a-momento | Hades (óbolos), STS (ouro) |
| **Sem dificuldade customizada** | acabou o conteúdo → acabou o jogo | **Hades (Pacto), DMD, STS (Ascensão)** |
| **Sem identidade de início de run** | toda run começa igual | Hades (Amuletos) |
| **Narrativa não avança** | volta só pelo combate; sem "quero ver o que acontece" | **Hades (drip narrativo)** |

> 🎯 Os **3 de maior alavanca/menor custo**: **variedade de salas**, **preview de porta** e **drip narrativo**. São P1 (M4-M5). O resto é pós-MVP.

---

## 3. 🍱 O menu de mecânicas (priorizado)

> 📂 **A, B, D e H têm doc próprio** (o detalhe vive lá; aqui é só o resumo do menu): **A/B → [47 — Tipos de Sala & Rota](47_Room_Types_Routing.md)** · **D → [48 — Pacto/Heat](48_Pact_Heat.md)** · **H → [49 — Codex/Voz](49_Codex_Limiar.md)**. C/E/F/G/I/J são detalhados só aqui.

### A. 🚪 Variedade de tipos de sala · 🟡 P1 → detalhe em [47](47_Room_Types_Routing.md)
Hoje toda sala é combate ([33](../enemies/33_Spawning_Encounters.md)). Adicione **8 tipos** (Combate/Elite/Tesouro/Loja/Santuário/Descanso/Evento/Boss) ao `EDDRRoomType` em `FDDRRoomDefinition` ([42 §3](../systems/42_Run_Room_Manager.md)). **MVP = 3** (Combate/Elite/Tesouro). Tabela completa de tipos: **[47 §1](47_Room_Types_Routing.md)**.

> Mesmo **3 tipos** (combate + elite + tesouro) já quebram a monotonia. Data-driven: é um `enum RoomType` + lógica por tipo.

### B. 🔮 Preview de porta & rota · 🟡 P1 (a joia do Hades)
Antes de escolher a próxima sala, **mostre o que ela oferece** (tipo + recompensa) e deixe **escolher entre 2 portas**.

```
        ┌─ Porta Esq ─┐   ┌─ Porta Dir ─┐
        │ ⚔️ Combate  │   │ 💰 Tesouro  │
        │ 🩸 Eco Carniça│  │ 🔥 Eco Fúria │   ← jogador ESCOLHE a rota
        └─────────────┘   └─────────────┘
```

> 🎯 **Transforma o corredor em decisão.** "Quero o Eco de Carniça mas a porta de combate é arriscada..." = agência = engajamento. Barato: o `RunManager` ([42 §4](../systems/42_Run_Room_Manager.md)) já sorteia salas — só **revele 2 e deixe escolher**.

### C. 💰 Economia in-run (ouro + loja) · 🟡 P1
Uma 2ª moeda **temporária** (some no fim da run), separada da Essência (meta):

| Moeda | Escopo | Gasta em |
|---|---|---|
| **Ouro/Fragmentos** | só a run | loja: comprar Eco, cura, reroll, remover Eco |
| **Essência** | persiste (meta) | hub ([40 §6](40_Eco_Pool_Catalog.md)) |

> Decisão momento-a-momento ("curo agora ou guardo pro Eco lendário da loja?") = tensão saudável. GAS-friendly (atributo `Gold` na run).

### D. 🔥 Pacto / Dificuldade customizada (Heat) · 🔵 P2 — **o rei da retenção de longo prazo**
Depois de vencer, deixe o jogador **ligar modificadores de dificuldade** por **mais recompensa/desbloqueio**. (Hades: *Pacto de Punição*; DMD; STS: *Ascensão*.)

```
PACTO (acumula "Calor"):
  + Inimigos +20% vida        (+1 Calor)
  + Boss tem fase extra        (+2 Calor)
  + Você cura 50% menos        (+1 Calor)
  → recompensa escala com o Calor total; desbloqueios gated por Calor
```

> 🪝 **É o que dá vida infinita ao jogo** quando o conteúdo "acaba". O jogador *escolhe* o próprio desafio. Data-driven (lista de modificadores = DataTable). **Maior investimento de retenção pós-MVP.**

### E. 🧿 Relíquias / escolha de início de run · 🔵 P2
No Limiar, escolha **1 relíquia** (passiva) antes de descer → identidade desde o frame 1 (Hades: Amuletos). Ex.: "comece com +1 no cap de juggle", "1º Eco é sempre Raro". Lore: um **fragmento de um caído** ([45 §4](45_World_Lore.md)).

### F. 📈 Evolução de Eco na run · 🔵 P2
Deixe **investir** num Eco já pego pra subir de tier (Hades: Poms). Na sala de Descanso (§A) ou loja (§C): "subir JugglePlus pra +2". Aprofunda a build escolhida em vez de só somar Ecos novos.

### G. ⛧ Santuários de risco/recompensa · 🔵 P2
Sala com uma **oferta com custo**: "sacrifique 30% da vida máx → Eco Lendário garantido" / "aceite uma maldição → +50% Essência". Tensão = runs memoráveis. (Lore: pactos com os caídos, [45](45_World_Lore.md).)

### H. 📖 Drip narrativo & codex · 🟡 P1 (a cola de retenção)
Liga o [45 — Lore](45_World_Lore.md) ao loop:
- **Voz da Fenda:** 1-2 linhas no boot/morte/profundidade (texto barato).
- **NPCs reagem** à sua progressão/mortes no Limiar.
- **Codex:** Essência destrava **memórias** dos caídos (lore das famílias).
- Vencer o boss N vezes **avança a verdade**.

> 🪝 É o que faz voltar **pra ver o que acontece**, não só pra bater. Barato no MVP (texto), enorme em retenção. **Faça o mínimo já no M5.**

### I. 🎲 Sorte & pity (anti-frustração) · 🔵 P2
- **Reroll** de Ecos (1 grátis/sala ou pago em ouro).
- **Garantia:** sem ver uma família por X salas → peso sobe (pity).
- Evita a run "azarada" que mata o vício.

### J. 🏆 Maestria / ranking / endless · 🔵 P2
- **Rank de estilo** ([28 §3](../ui/28_HUD.md)) → pontuação de run → "melhor combo" no save ([25](../ui/25_Save_Load_Slots.md)).
- **Endless/Ascensão** pós-boss pra quem quer mais.
- Casa com o Pacto (§D).

---

## 4. 🎯 Priorização honesta (não inche o MVP)

> ⚠️ **A disciplina de escopo continua valendo** (Sofia, [Design Review](Design_Review_2026-06.md)). O MVP precisa do **loop core jogável** — não desta lista inteira. Adicione a camada de retenção **em ordem de alavanca/custo**:

| Onda | Mecânicas | Quando |
|---|---|---|
| **Já no MVP (M4-M5)** | Loop core ([42](../systems/42_Run_Room_Manager.md)) + **3 tipos de sala** (A) + **preview de porta** (B) + **drip narrativo mínimo** (H) | 🟡 P1 |
| **Polish pós-jogável** | Economia in-run (C) + santuários (G) + pity (I) | 🔵 P2 |
| **Retenção de longo prazo** | **Pacto/Heat (D)** + relíquias (E) + evolução de Eco (F) + maestria/endless (J) | 🔵 P2+ |

> 🥇 **Se fizer só 3 coisas além do core:** variedade de salas (A) + preview de porta (B) + drip narrativo (H). Custam pouco e entregam 80% do "só mais uma".
> 🏆 **A maior aposta pós-lançamento:** o **Pacto (D)** — é o que dá centenas de horas quando o conteúdo "acaba".

---

## 5. Como tudo encaixa no que já existe (zero reescrita)

| Mecânica nova | Encaixa em | Como |
|---|---|---|
| Tipos de sala (A) | `FDDRRoomDefinition` ([42 §3](../systems/42_Run_Room_Manager.md)) | + `enum RoomType` + lógica por tipo |
| Preview de porta (B) | `RunManager::BeginRoomSelect` | revela 2 salas sorteadas, jogador escolhe |
| Ouro/loja (C) | GAS atributo `Gold` (run) | reusa pipeline de Eco pra "comprar" |
| Pacto (D) | DataTable de modificadores + GE/run config | aplica `GE` globais na run |
| Relíquias/evolução (E/F) | `UDDREcoData`/`UDDREcoComponent` ([40 §5](40_Eco_Pool_Catalog.md)) | reusa o sistema de Eco |
| Drip/codex (H) | `UDDRRunManager` delegates + `FText` ([38](../systems/38_Localization.md)) | eventos disparam linhas/unlocks de codex |

> Tudo **data-driven** ([44](../systems/44_Data_Driven.md)) e **GAS** ([05](../systems/05_GAS_Architecture.md)) — a arquitetura já comporta. Nada aqui pede reescrever o core.

---

## 6. Checklist (camada de retenção)

- [ ] `enum RoomType` + ≥3 tipos de sala (combate/elite/tesouro) — A
- [ ] Preview de 2 portas com tipo+recompensa — B
- [ ] Moeda in-run `Gold` + 1 loja — C (P2)
- [ ] Drip narrativo mínimo (Voz da Fenda + 1 reação de NPC + codex stub) — H
- [ ] **Pacto/Heat** desenhado no papel (DataTable de modificadores) — D (P2, mas planeje)
- [ ] Reroll/pity de Eco — I (P2)
- [ ] Rank de run salvo (maestria) — J (P2)
- [ ] Nada disto bloqueia o **loop core jogável** do MVP

---

## 7. Próximo passo

→ [45 — Lore](45_World_Lore.md) (a ficção que a narrativa drip usa) · [42 — Run Manager](../systems/42_Run_Room_Manager.md) (onde tipos de sala/preview entram) · [40 — Ecos](40_Eco_Pool_Catalog.md) · [03 — Core Loop](03_Core_Loop_Roguelike.md).
