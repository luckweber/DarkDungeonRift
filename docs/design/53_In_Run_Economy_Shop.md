# 53 — Economia In-Run (Ouro & Loja) · 🟡 P1 (pós-core)

> Detalha a mecânica **C** do [46 — Profundidade](46_Depth_Replayability.md): moeda **temporária** da run, lojas e decisões momento-a-momento. Estende tipos de sala **Loja** ([47 §1](47_Room_Types_Routing.md)). **Não é MVP-core** — entra quando tipos de sala (M4-M5 polish) existirem.

> 🧭 **Por que documentar agora:** a arquitetura (atributo GAS + UI + DataTable de preços) encaixa no pipeline de Eco sem reescrita ([05](../systems/05_GAS_Architecture.md), [44](../systems/44_Data_Driven.md)).

---

## 1. Duas moedas — não confundir

| Moeda | Escopo | Ganha em | Gasta em |
|---|---|---|---|
| **Fragmentos** (ouro in-run) | **só a run atual** | kills, salas Tesouro, eventos | **Loja** — Eco, cura, reroll |
| **Essência** | **meta** (persiste) | fim de run / morte | **Hub** — desbloqueios ([03 §5](03_Core_Loop_Roguelike.md)) |

```
Run:  Fragmentos → Loja (decisão agora: compro Eco ou guardo?)
Meta: Essência   → Hub (decisão entre runs: destravo o quê?)
```

> 🎯 Tensão saudável: "curo agora ou guardo pro Eco lendário da loja?" — sem segunda moeda, Loja vira menu morto.

---

## 2. Atributo GAS — `Gold` / Fragmentos

```cpp
// UDDRRunAttributeSet (ou extensão do PlayerAttributeSet — run-scoped)
UPROPERTY(BlueprintReadOnly, Category = "Run")
FGameplayAttributeData Gold;          // Fragmentos
UPROPERTY(BlueprintReadOnly, Category = "Run")
FGameplayAttributeData GoldMax;         // cap opcional (ex.: 999)

// Reset no RunStart; NÃO persiste no SaveGame meta
```

**Ganhar Fragmentos:**

| Fonte | Quantidade (inicial) |
|---|---:|
| Kill Grunt | +3 |
| Kill Archer | +4 |
| Kill Brute | +10 |
| Sala Tesouro | +15-25 (fixo) |
| Evento narrativo | variável ([49](49_Codex_Limiar.md)) |

```cpp
// GE_GrantGold — Instant, SetByCaller magnitude
ASC->ApplyGameplayEffectToSelf(GE_GrantGold,
    /* level */ 1,
    MakeEffectContextWithSetByCaller(DDRTags.Data.Gold, Amount));
```

> Expor drops no `UDDREnemyData` (`GoldDrop` field) — [51 §2](../enemies/51_Enemy_Catalog_MVP.md) pode adicionar coluna.

---

## 3. Sala Loja — fluxo

```
Player entra Arena_Shop (RoomType = Shop, 47)
  → sem combate (ou 1 onda opcional P2)
  → UI Loja abre (W_Shop — [29](../ui/29_Run_Flow_Menus.md) extensão)
  → 3-4 ofertas sorteadas de DT_ShopInventory
  → compra gasta Gold → aplica efeito (Eco / heal / reroll)
  → porta de saída → próxima sala (sem Eco-select — loja substitui recompensa)
```

| Regra | Detalhe |
|---|---|
| `bOffersEco` | **false** na loja ([42 §3](../systems/42_Run_Room_Manager.md)) |
| Preview de porta | ícone 🪙 + "Loja" ([47](47_Room_Types_Routing.md)) |
| Lore | "Um caído mercador" ([45 §4](45_World_Lore.md)) |

---

## 4. `DT_ShopInventory` — catálogo de ofertas

```cpp
USTRUCT() struct FDDRShopOffer : public FTableRowBase
{
    FGameplayTag OfferId;              // Shop.Heal.Small
    FText DisplayName;
    FText Description;
    int32 GoldCost = 25;
    EDDRShopOfferType Type;            // Heal, EcoRoll, RemoveEco, RerollEco
    TSoftObjectPtr<UDDREcoData> Eco;    // se Type = Eco
    float HealPercent = 0.f;           // se Type = Heal
    int32 MinDepth = 1;                // só aparece a partir da sala N
};
```

**Ofertas MVP (6 linhas):**

| ID | Nome | Custo | Efeito |
|---|---|---:|---|
| `Shop.Heal.Small` | Orbe de Sangue | 20 | +25% HP |
| `Shop.Heal.Full` | Elixir Raro | 45 | +60% HP |
| `Shop.Eco.Common` | Eco Comum | 35 | 1 Eco Comum aleatório |
| `Shop.Eco.Rare` | Eco Raro | 70 | 1 Eco Raro (pool filtrado) |
| `Shop.Reroll` | Reroll de Eco | 15 | reroll na próxima seleção ([46 §I](46_Depth_Replayability.md)) |
| `Shop.RemoveEco` | Purga | 10 | remove 1 Eco da build (P2) |

> Sorteia 3 ofertas por visita; peso por `MinDepth` + raridade. Data-driven — designer adiciona linha, não código.

---

## 5. UI mínima (W_Shop)

```
┌─ LOJA DO CAÍDO ────────────────────────┐
│  Fragmentos: 🪙 47                      │
├────────────────────────────────────────┤
│  [1] Orbe de Sangue      🪙 20  [Comprar]│
│  [2] Eco Raro            🪙 70  [Comprar]│
│  [3] Reroll              🪙 15  [Comprar]│
├────────────────────────────────────────┤
│  [Sair] → continua run                  │
└────────────────────────────────────────┘
```

- Input mode: UI + gamepad navegação ([23](../ui/23_UI_Overview.md))
- `FText` em nomes ([38](../systems/38_Localization.md))
- Compra falha se `Gold < Cost` — feedback sonoro negativo

---

## 6. Integração com Ecos

Comprar Eco na loja **reusa** o pipeline existente:

```cpp
void UDDRShopSubsystem::PurchaseEco(UDDREcoData* Eco, int32 Cost)
{
    if (GetGold() < Cost) return;
    ApplyGoldDelta(-Cost);
    PlayerEcoComponent->ApplyEco(Eco);   // mesmo path que seleção pós-sala (40 §5)
}
```

> Não duplique lógica de granting — `ApplyEco` já valida sinergias e tags ([03b §6](03b_Reward_System.md)).

---

## 7. Balanceamento inicial

| Knob | Valor | Nota |
|---|---|---|
| Gold médio por sala combate | ~12-18 | 4 grunts ≈ 12 |
| Custo heal pequeno | 20 | ~1 sala de farm |
| Custo Eco raro | 70 | ~4-5 salas — decisão real |
| Salas Loja por run | 0-1 | preview de porta escolhe |

> Loja **opcional** na rota — quem evita loja ganha mais Ecos em salas Combate ([47 §reconciliação](47_Room_Types_Routing.md)).

---

## 8. Prioridade

| Item | Quando |
|---|---|
| Atributo `Gold` run-scoped | 🟡 com Loja |
| Drops em inimigos | 🟡 |
| `DT_ShopInventory` + W_Shop | 🟡 M4-M5 polish |
| Sala Tesouro (Gold burst) | 🟡 com tipos de sala |
| Remove Eco / pity avançado | 🔵 P2 |

---

## 9. Checklist

- [ ] `Gold` no AttributeSet — reset no `RunStart`, **não** no SaveGame meta
- [ ] `GoldDrop` por inimigo ou GE no death
- [ ] `RoomType.Shop` + arena simples ([52](../world/52_Arena_Level_Design.md))
- [ ] `DT_ShopInventory` com ≥6 ofertas
- [ ] W_Shop + fluxo compra → `ApplyEco` / heal GE
- [ ] Preview de porta mostra 🪙 Loja ([47](47_Room_Types_Routing.md))
- [ ] Distinção clara Fragmentos vs Essência na UI

---

## 10. Próximo passo

→ [47 — Tipos de Sala](47_Room_Types_Routing.md) · [46 — Profundidade](46_Depth_Replayability.md) · [29 — Run Menus](../ui/29_Run_Flow_Menus.md) · [42 — Run Manager](../systems/42_Run_Room_Manager.md).
