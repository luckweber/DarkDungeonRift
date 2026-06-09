# 42 — Run & Room Manager · 🟢 P0 (design) / 🟡 P1 (código M4)

> Orquestra o **loop da run** ([03](../design/03_Core_Loop_Roguelike.md)): sala → ondas → limpar → Eco → próxima sala → morte/vitória. Tudo **data-driven** — salas, ondas e recompensas vêm de DataTables/DataAssets ([44](44_Data_Driven.md)), não de hardcode.

---

## 1. Visão geral

```
┌─────────────────────────────────────────────────────────────┐
│  DDRRunManager (GameStateComponent ou Subsystem)            │
│    • estado da run (sala atual, Ecos escolhidos, Essência)  │
│    • sorteia ordem de arenas (DT_RunLayout)                 │
│    • dispara UI de Eco (1 de 3)                             │
│    • meta ao fim (vitória/morte)                            │
└──────────────────────────┬──────────────────────────────────┘
                           │ carrega arena
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  ADDREncounterManager (Actor na arena — doc 33)             │
│    • ondas (FDDRWave de DT_Encounter)                       │
│    • portas trancam/abrem                                   │
│    • OnRoomCleared → RunManager                             │
└─────────────────────────────────────────────────────────────┘
```

| Classe | Onde vive | Responsabilidade |
|---|---|---|
| **`UDDRRunManager`** | `GameState` ou `UGameInstanceSubsystem` | Run inteira: progressão, Ecos, fim |
| **`ADDREncounterManager`** | Level da arena | Uma sala: spawn, ondas, clear |
| **`UDDREcoComponent`** | Player Character | Ecos ativos na run (GAS) |
| **`ADDRRunDoor`** | Actor na arena | Tranca/abre com tag `State.Run.Locked` |

> 📛 Código em `Source/DarkDungeonRift/Run/` ([04 §3](04_Project_Setup.md)).

---

## 2. Máquina de estados da run

```
[RunStart]
    → LoadRoom(0)
    → [RoomActive]     portas trancadas, combate
    → [RoomCleared]    última onda morta
    → [EcoSelect]      UI 1-de-3 (pula boss room se configurado)
    → [RoomAdvance]    sala++
    → (sala < N) ? LoadRoom(sala) : [BossRoom]
[BossRoom] → [RunVictory] ou [RunDeath]
    → [MetaReward]     Essência → hub
```

```cpp
UENUM()
enum class EDDRRunPhase : uint8
{
    Starting,
    RoomActive,
    RoomCleared,
    EcoSelect,
    Advancing,
    Boss,
    Victory,
    Death
};
```

---

## 3. Dados da run (structs)

```cpp
USTRUCT(BlueprintType)
struct FDDRRunState
{
    int32 CurrentRoomIndex = 0;
    int32 RoomsCleared = 0;
    TArray<TObjectPtr<UDDREcoData>> ChosenEcos;
    TArray<FGameplayTag> ActiveEcoTags;      // Eco.Tempest.* etc.
    int32 EssenceEarnedThisRun = 0;
    EDDRRunPhase Phase = EDDRRunPhase::Starting;
};

USTRUCT(BlueprintType)
struct FDDRRoomDefinition : public FTableRowBase
{
    FName RoomId;                            // "Arena_Crypt_01"
    TSoftObjectPtr<UWorld> Level;            // sublevel ou mapa
    TObjectPtr<UDDREncounterData> Encounter;   // DataAsset de ondas
    int32 Depth = 1;                         // 1-4 para escalada (33 §4)
    bool bOffersEco = true;                  // false na sala pré-boss
    bool bIsBossRoom = false;
};
```

---

## 4. Fluxo sala a sala (detalhe)

### 4.1 Entrada na sala

1. `RunManager` carrega `FDDRRoomDefinition` da linha `DT_RunLayout` (ordem embaralhada no `RunStart`).
2. `EncounterManager` recebe `UDDREncounterData` (waves, spawn points).
3. Portas → `Locked`; player trigger → `StartEncounter()`.

### 4.2 Combate

- Spawn por onda ([33](../enemies/33_Spawning_Encounters.md)).
- Token director na sala.
- Morte do player → `RunManager::OnPlayerDeath()` → `Phase = Death`.

### 4.3 Sala limpa

```cpp
void ADDREncounterManager::OnWaveComplete()
{
    if (bAllWavesDone)
    {
        UnlockDoors();
        OnRoomCleared.Broadcast();
        RunManager->NotifyRoomCleared(CurrentRoomDef);
    }
}
```

### 4.4 Seleção de Eco

```cpp
void UDDRRunManager::BeginEcoSelect()
{
    const TArray<UDDREcoData*> Options = RollEcoOptions(
        /* count */ 3,
        /* filter */ GetUnlockedPool(),
        /* weights */ GetDepthWeights(CurrentDepth)
    );
    EcoSelectUI->Show(Options);   // doc 29
}

void UDDRRunManager::OnEcoChosen(UDDREcoData* Eco)
{
    PlayerEcoComponent->ApplyEco(Eco);   // GAS — doc 05, 40
    ChosenEcos.Add(Eco);
    AdvanceToNextRoom();
}
```

**Sorteio data-driven:** `DT_EcoPool` + `DT_EcoWeightsByDepth` ([44 §4](44_Data_Driven.md)).

### 4.5 Avanço

- `CurrentRoomIndex++`
- Se `bIsBossRoom` na próxima → `Phase = Boss`
- Senão → `LoadRoom(CurrentRoomIndex)`

---

## 5. Layout da run ("procedural sem procedural")

| Asset | Tipo | Conteúdo |
|---|---|---|
| `DT_RunLayout` | DataTable | 6-8 arenas possíveis (`FDDRRoomDefinition`) |
| `DA_RunConfig_MVP` | DataAsset | `RoomsPerRun = 4`, `BossRoomId`, seed |
| `DT_Encounter_*` | DataTable | Waves por arena (ou `UDDREncounterData` por arena) |

**Embaralhamento no `RunStart`:**

```cpp
TArray<FName> AllRooms = GetAllRoomIdsFromTable();
Shuffle(AllRooms, RunSeed);
PickFirstN(AllRooms, RunConfig->RoomsPerRun);
AppendBoss(RunConfig->BossRoomId);
```

> 🎲 Jogador sente variação sem gerador procedural ([03 §2](../design/03_Core_Loop_Roguelike.md)).

---

## 6. Integração com outros sistemas

| Sistema | Hook | Doc |
|---|---|---|
| **GAS** | `UDDREcoComponent::ApplyEco` | [05](05_GAS_Architecture.md), [40](../design/40_Eco_Pool_Catalog.md) |
| **UI** | `WBP_EcoSelect`, `WBP_RunHUD` | [29](../ui/29_Run_Flow_Menus.md), [28](../ui/28_HUD.md) |
| **Spawn** | `EncounterManager` + `UDDREnemyData` | [33](../enemies/33_Spawning_Encounters.md) |
| **Save** | `FDDRRunState` **não** persiste entre runs; meta sim | [25](../ui/25_Save_Load_Slots.md) |
| **Loading** | travel entre sublevels | [26](../ui/26_Loading_System.md) |

---

## 7. Escalada por profundidade

`Depth` na `FDDRRoomDefinition` alimenta:

| Sistema | Uso |
|---|---|
| Encounter | mais inimigos/ondas ([33 §4](../enemies/33_Spawning_Encounters.md)) |
| Eco roll | peso maior em Raro/Lendário (`DT_EcoWeightsByDepth`) |
| Inimigos | `UDDREnemyData` com multiplier de HP/dano |

```cpp
// DT_EcoWeightsByDepth — exemplo de linha
struct FDDREcoWeightRow : FTableRowBase
{
    int32 Depth;
    float CommonWeight = 1.f;
    float RareWeight = 0.3f;
    float LegendaryWeight = 0.05f;
};
```

---

## 8. Essência e fim de run

| Evento | Essência |
|---|---|
| Sala limpa | +10 (tunable em `DA_RunConfig`) |
| Inimigo morto | +1 |
| Boss morto | +50 |
| Morte | mantém Essência **já ganha** na run |

`OnRunEnd` → persiste em `USaveGame` meta → hub ([03 §5](../design/03_Core_Loop_Roguelike.md)).

---

## 9. API mínima (C++)

```cpp
UCLASS()
class UDDRRunManager : public UGameStateComponent
{
public:
    void StartRun(const UDDRRunConfigData* Config);
    void NotifyRoomCleared(const FDDRRoomDefinition& Room);
    void OnEcoChosen(UDDREcoData* Eco);
    void OnPlayerDeath();
    FDDRRunState GetRunState() const;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnRunPhaseChanged, EDDRRunPhase);
    FOnRunPhaseChanged OnRunPhaseChanged;
};
```

---

## 10. Checklist

- [ ] `UDDRRunManager` + `ADDREncounterManager` esboçados
- [ ] `DT_RunLayout` com 6-8 arenas MVP
- [ ] `DT_EcoPool` + sorteio 1-de-3
- [ ] Portas trancam/abrem no clear
- [ ] Morte interrompe run e salva Essência
- [ ] Boss room no fim (4 salas + boss)
- [ ] Tudo tunável via DataAssets ([44](44_Data_Driven.md))
- [ ] Sem magic numbers de spawn/recompensa no C++

---

## 11. Próximo passo

→ [44 — Data-Driven](44_Data_Driven.md) · [33 — Spawning](../enemies/33_Spawning_Encounters.md) · [40 — Ecos](../design/40_Eco_Pool_Catalog.md) · [17 — Roadmap](../17_Implementation_Roadmap.md).
