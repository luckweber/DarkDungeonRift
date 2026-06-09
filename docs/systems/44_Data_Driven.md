# 44 — Arquitetura Data-Driven · 🟢 P0

> **Regra do projeto:** gameplay tunável vive em **DataTables** e **DataAssets** — não em constantes C++. Código define *como* ler e *como* aplicar; designers definem *quanto*, *quando* e *qual* conteúdo. Aplica a combate, Ecos, inimigos, encontros, run e meta.

> 🔑 **Por quê:** roguelike = centenas de combinações de run. Sem data-driven, cada Eco/inimigo/onda vira recompile + PR.

---

## 1. Quando usar cada tipo

| Tipo UE | Use para | Exemplos DDR | Não use para |
|---|---|---|---|
| **DataTable** (`FTableRowBase`) | **Muitas linhas** homogêneas; lookup por `RowName`; CSV export | `DT_EcoPool`, `DT_RunLayout`, `DT_AttackData`, `DT_LootWeights` | Um único boss com 40 campos únicos |
| **DataAsset** (`UDataAsset`) | **Um asset = uma entidade** com referências (BP, GE, montage) | `UDDREcoData`, `UDDREnemyData`, `UDDREncounterData` | 200 linhas de stat idêntico |
| **PrimaryDataAsset** | Asset com **ID estável** + Asset Manager / soft load | `UDDREcoData`, `UDDREnemyData` | — |
| **CurveTable** | Valores por **nível/profundidade** contínuos | HP scale por sala | Texto de UI |
| **String Table** | Localização | nomes de Ecos | lógica |
| **GameplayTags INI** | Identificadores de estado/regra | [41](41_Gameplay_Tags.md) | números de dano |

### Árvore de decisão

```
É uma LISTA grande de entradas parecidas?
  SIM → DataTable (linha) + opcional DataAsset por linha (referência)
  NÃO → É UMA entidade com refs a BP/GE/montage?
          SIM → Primary Data Asset
          NÃO → CurveTable (escala) ou C++ const (só se imutável)
```

---

## 2. Catálogo de dados do MVP

| Asset / Tabela | Struct / Classe | Conteúdo | Doc |
|---|---|---|---|
| `DT_AttackData` | `FDDRAttackData` | frame data, dano, poise, cancel | [18](../combat/18_Combat_System_Deep.md) |
| `DA_Eco_*` (×18) | `UDDREcoData` | cada Eco | [40](../design/40_Eco_Pool_Catalog.md) |
| `DT_EcoPool` | `FDDREcoPoolRow` | pool rolável + unlock flag | [40](../design/40_Eco_Pool_Catalog.md) |
| `DT_EcoWeightsByDepth` | `FDDREcoWeightRow` | raridade por sala | [42](42_Run_Room_Manager.md) |
| `DT_RunLayout` | `FDDRRoomDefinition` | arenas da run | [42](42_Run_Room_Manager.md) |
| `DA_RunConfig_MVP` | `UDDRRunConfigData` | salas/run, boss, essência | [42](42_Run_Room_Manager.md) |
| `DA_Encounter_*` | `UDDREncounterData` | ondas por arena | [33](../enemies/33_Spawning_Encounters.md) |
| `DA_Enemy_*` | `UDDREnemyData` | stats, BT, mesh, attacks | [51](../enemies/51_Enemy_Catalog_MVP.md) |
| `DT_ShopInventory` | `FDDRShopOffer` | ofertas da loja in-run | [53](../design/53_In_Run_Economy_Shop.md) |
| `DT_ProjectileData` | `FDDRProjectileData` | Atirador M3 | [37](../combat/37_Projectiles.md) |
| `DA_MetaUpgrades` | `UDDRMetaUpgradeData` | hub | [03b §7](../design/03b_Reward_System.md) |

---

## 3. Structs principais

### 3.1 Ataque (DataTable)

```cpp
USTRUCT(BlueprintType)
struct FDDRAttackData : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) FName AttackId;
    UPROPERTY(EditAnywhere) float BaseDamage = 10.f;
    UPROPERTY(EditAnywhere) float PoiseDamage = 15.f;
    UPROPERTY(EditAnywhere) float HitStopFrames = 3.f;
    UPROPERTY(EditAnywhere) FGameplayTag AttackTag;          // Ability.Attack.Light
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimMontage> Montage;
    UPROPERTY(EditAnywhere) TSubclassOf<UGameplayEffect> DamageEffect;
};
```

> Abilities leem `DT_AttackData` por `RowName` — designer troca dano sem tocar C++.

### 3.2 Eco (Primary Data Asset)

```cpp
UCLASS(BlueprintType)
class UDDREcoData : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Eco", GetFName());
    }

    UPROPERTY(EditDefaultsOnly) FGameplayTag EcoId;
    UPROPERTY(EditDefaultsOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly) FText Description;
    UPROPERTY(EditDefaultsOnly) FGameplayTag FamilyTag;
    UPROPERTY(EditDefaultsOnly) EDDRRarity Rarity;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayEffect> GameplayEffect;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayAbility> GrantedAbility;
    UPROPERTY(EditDefaultsOnly) TArray<FGameplayTag> SynergyRequires;
};
```

### 3.3 Inimigo (Primary Data Asset)

```cpp
UCLASS()
class UDDREnemyData : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly) float MaxHealth = 50.f;
    UPROPERTY(EditDefaultsOnly) float MoveSpeed = 350.f;
    UPROPERTY(EditDefaultsOnly) float Poise = 30.f;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<APawn> PawnClass;
    UPROPERTY(EditDefaultsOnly) UBehaviorTree* BehaviorTree;
    UPROPERTY(EditDefaultsOnly) TArray<FDataTableRowHandle> Attacks;  // → DT_AttackData
};
```

### 3.4 Pool row (DataTable → aponta para DA)

```cpp
USTRUCT()
struct FDDREcoPoolRow : public FTableRowBase
{
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UDDREcoData> Eco;
    UPROPERTY(EditAnywhere) float Weight = 1.f;
    UPROPERTY(EditAnywhere) int32 MinDepth = 1;
    UPROPERTY(EditAnywhere) bool bEnabledByDefault = true;
};
```

---

## 4. Pastas Content

```
Content/DarkDungeonRift/Data/
├── Tables/
│   ├── DT_AttackData.uasset
│   ├── DT_EcoPool.uasset
│   ├── DT_EcoWeightsByDepth.uasset
│   ├── DT_RunLayout.uasset
│   └── DT_ProjectileData.uasset
├── Ecos/
│   ├── DA_Eco_Tempest_JugglePlus.uasset
│   └── ... (18 assets — doc 40)
├── Enemies/
│   ├── DA_Enemy_Grunt.uasset
│   ├── DA_Enemy_Ranged.uasset
│   └── DA_Enemy_Elite.uasset
├── Encounters/
│   ├── DA_Encounter_Crypt_01.uasset
│   └── ...
├── Run/
│   └── DA_RunConfig_MVP.uasset
└── Meta/
    └── DA_MetaProgression.uasset
```

> 🗂️ Espelha `Source/DarkDungeonRift/` ([04 §3](04_Project_Setup.md)). **Nunca** misture dados de gameplay com meshes de personagem na mesma pasta.

---

## 5. Padrões de leitura no código

### Lookup de DataTable

```cpp
const FDDRAttackData* Row = AttackTable->FindRow<FDDRAttackData>(RowName, TEXT("ApplyDamage"));
if (Row)
{
    ApplyDamageFromRow(*Row, Target);
}
```

### Soft load de Eco (Asset Manager)

```cpp
// DefaultGame.ini — opcional M4+
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="Eco",AssetBaseClass=/Script/DarkDungeonRift.DDREcoData,bHasBlueprintClasses=false)

// Runtime
StreamableManager.RequestAsyncLoad(EcoPtr.ToSoftObjectPath(), []{ ... });
```

### DataAsset no Encounter

```cpp
UPROPERTY(EditAnywhere, Category="Data")
TObjectPtr<UDDREncounterData> EncounterData;   // designer arrasta no BP_EncounterManager
```

> ⚠️ **Evite** `ConstructorHelpers::FObjectFinder` em runtime — só editor defaults ou soft refs.

---

## 6. Workflow do designer

| Tarefa | Onde | Passos |
|---|---|---|
| Novo Eco | `DA_Eco_*` + linha `DT_EcoPool` | Duplicar template, setar GE/Ability, adicionar row |
| Buff de dano do grunt | `DA_Enemy_Grunt` | Editar `MaxHealth` / attack row |
| Nova onda na arena | `DA_Encounter_*` | Adicionar `FDDRWave`, spawn tags |
| Nova arena na run | `DT_RunLayout` | Nova row + level sublevel |
| Traduzir nome Eco | String Table | `FText` key no `UDDREcoData` ([38](38_Localization.md)) |

**Validação no editor (P1):** `UDataValidation` em `UDDREcoData` — checa `EcoId` único, `GE` não nulo, tag registrada.

---

## 7. O que NÃO vai em data (fica em C++)

| Fica em código | Por quê |
|---|---|
| Pipeline de dano (ordem de ops) | regra de engine, não tuning |
| Slots de tags GAS | contrato fixo ([41](41_Gameplay_Tags.md)) |
| RootMotionSource do juggle | comportamento complexo ([16](../combat/16_Aerial_Combos.md)) |
| State machine da run | orquestração ([42](42_Run_Room_Manager.md)) |

**Números** dentro desses sistemas → **sim** em data (`DT_AttackData`, curves).

---

## 8. GAS + data-driven

| Conceito | Data | Código |
|---|---|---|
| Dano do 2º golpe | `DT_AttackData` row `Combo2` | `GA_Attack` lê row por seção |
| Eco "+1 juggle" | `DA_Eco` → `GE_Infinite` | `GA_AirAttack` lê `MaxJuggleHits` |
| Cooldown dash | `GE_Cooldown` duration no asset | `GA_Dash` `ActivationBlockedTags` |
| Conceder ability | `GrantedAbility` no `UDDREcoData` | `UDDREcoComponent::ApplyEco` |

> Runtime granting de 20× `GE` Infinite por run — valide no M1 ([03b §6](../design/03b_Reward_System.md)).

---

## 9. Naming conventions

| Tipo | Padrão | Exemplo |
|---|---|---|
| DataTable | `DT_<Domain>` | `DT_EcoPool` |
| DataAsset | `DA_<Type>_<Name>` | `DA_Eco_Tempest_JugglePlus` |
| RowName | `PascalCase` ou `Snake_Case` consistente | `Combo_Light_2` |
| PrimaryAssetType | curto | `Eco`, `Enemy` |

---

## 10. Checklist

- [ ] Decisão DataTable vs DataAsset documentada (§1)
- [ ] Pasta `Content/DarkDungeonRift/Data/` criada
- [ ] `FDDRAttackData`, `UDDREcoData`, `UDDREnemyData` definidos
- [ ] Nenhum balanceamento crítico hardcoded em `.cpp`
- [ ] GameplayTags em INI, não em DataTable
- [ ] `FText` em todo campo visível ao jogador
- [ ] Asset Manager configurado antes de M4 (soft load de Ecos)
- [ ] Validação de assets no editor (P1)

---

## 11. Próximo passo

→ [40 — Pool de Ecos](../design/40_Eco_Pool_Catalog.md) · [42 — Run Manager](42_Run_Room_Manager.md) · [04 — Setup](04_Project_Setup.md) · [05 — GAS](05_GAS_Architecture.md).
