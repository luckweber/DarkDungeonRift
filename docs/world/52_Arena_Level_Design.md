# 52 — Arena & Level Design (playbook) · 🟢 P0 (M3) / 🟡 P1 (M4)

> **Como montar as salas do MVP:** 6 arenas de combate + 1 sala de boss, spawn points, portas, NavMesh e pareamento com encontros. Materializa o *"procedural sem procedural"* ([03 §2](../design/03_Core_Loop_Roguelike.md)) e alimenta `DT_RunLayout` ([42 §3](../systems/42_Run_Room_Manager.md)). Inimigos concretos: **[51 — Catálogo](../enemies/51_Enemy_Catalog_MVP.md)**.

> 🧭 **Por que este doc existe:** spawn/colocação aparecia em 5 docs (03, 33, 34, 42, 44) sem um **playbook único** de level design. Aqui vive o *como montar*; os outros referenciam daqui.

---

## 1. Princípios de arena topdown

| Regra | Por quê |
|---|---|
| **Chão plano** no MVP | Foot IK desnecessário ([14](../locomotion/14_Foot_Leg_IK.md)); leitura de telegrafe limpa |
| **Silhueta clara** | Paredes/obstáculos low-poly; sem clutter que esconde decals |
| **Espaço central livre** | Zona de combate ~70% da área — margem pras spawn points |
| **Spawn longe do player** | Nunca no colo ([33 §3](../enemies/33_Spawning_Encounters.md)) |
| **Portas visíveis** | Player sabe onde entrou/sai — preview de porta futuro ([47](../design/47_Room_Types_Routing.md)) |
| **NavMesh completo** | IA persegue sem ficar presa ([30](../enemies/30_AI_Overview.md)) |

> 🎯 Arena boa = **legível de cima**. O jogador lê telegrafe no chão, vê spawns chegando, tem espaço pra dash. Beleza é P2.

---

## 2. Anatomia de uma arena (actors & markers)

```
Arena Level (sublevel ou mapa dedicado)
├── DDR_EncounterManager          ← ADDREncounterManager (33 §2)
├── DDR_PlayerStart               ← onde o player entra
├── SpawnPoints/                  ← TargetPoints tagueados
│   ├── Spawn_N, Spawn_S, Spawn_E, Spawn_W
├── Doors/
│   ├── Door_Exit                 ← ADDRRunDoor (tranca/abre)
│   └── Door_Entrance (opcional)
├── NavMeshBoundsVolume           ← cobre 100% do chão jogável
├── Lighting / PostProcess (mínimo)
└── (P1) EcoSelectTrigger / RewardVolume
```

**Tags de spawn (convenção):**

| Tag | Uso |
|---|---|
| `Spawn_N` / `S` / `E` / `W` | cardinal — longe do centro |
| `Spawn_Far` | onda 2+ — oposto ao player |
| `Spawn_Elite` | reservado p/ Brutamontes (não no colo) |
| `Boss_Center` | só arena de boss |

```cpp
// EncounterManager resolve spawn por tag:
AActor* FindSpawnPoint(FName Tag) {
    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, Found);
    return Found.Num() ? Found[FMath::RandRange(0, Found.Num()-1)] : nullptr;
}
```

---

## 3. Catálogo de arenas MVP

| RoomId | Nome (dev) | Tamanho | Forma | Encounter default | Depth |
|---|---|---:|---|---|:---:|
| `Arena_Crypt_01` | Cripta estreita | 18×14 m | retângulo | `DA_Encounter_Crypt_Easy` | 1 |
| `Arena_Rift_02` | Fenda aberta | 22×22 m | quadrado | `DA_Encounter_Rift_Mixed` | 2 |
| `Arena_Catacombs_03` | Catacumbas | 20×16 m | L-shape | `DA_Encounter_Rift_Mixed` | 2-3 |
| `Arena_Chamber_04` | Câmara central | 24×24 m | quadrado + 4 pilares | `DA_Encounter_Chamber_Pressure` | 3 |
| `Arena_Pit_05` | Poço raso | 16×16 m | circular | `DA_Encounter_Pit_Elite` | 3-4 |
| `Arena_Spire_06` | Torre quebrada | 20×18 m | retângulo + corredor | `DA_Encounter_Chamber_Pressure` | 2-4 |
| `Arena_Boss_Guardian` | Trono do Guardião | 28×28 m | circular grande | `DA_Encounter_Boss` | Boss |

> 6 arenas de combate embaralhadas + 1 boss fixa no fim ([42 §5](../systems/42_Run_Room_Manager.md)). Variação **sem** gerador procedural.

### Layout por arena (notas de design)

**Crypt_01 (tutorial feel):**
- 4 spawn points nos cantos · 1 onda só · poucos obstáculos
- Porta de saída ao norte · player start ao sul

**Rift_02 (padrão):**
- 4 spawns cardinais · 2 ondas · espaço aberto pro kiting do Archer

**Chamber_04 (obstáculos):**
- 4 pilares centrais (cover visual, não bloqueia NavMesh)
- Spawns nas bordas · onda 2 entra por `Spawn_Far`

**Pit_05 (elite):**
- Menor · força proximidade · spawn elite em `Spawn_Elite` distante
- Ideal p/ sala Elite ([47](../design/47_Room_Types_Routing.md)) pós-MVP

**Boss_Guardian:**
- **Sem** outros inimigos no MVP
- Espaço extra p/ esquivar Charge/AoE ([34 §5](../enemies/34_MiniBoss.md))
- `Boss_Center` spawn · player start na borda sul

---

## 4. `DT_RunLayout` — linhas exemplo

```cpp
// FDDRRoomDefinition — ver 42 §3
Row: Arena_Crypt_01
  RoomId = Arena_Crypt_01
  Level = /Game/Maps/Arenas/Arena_Crypt_01
  Encounter = DA_Encounter_Crypt_Easy
  Depth = 1
  RoomType = Combat
  bOffersEco = true

Row: Arena_Boss_Guardian
  RoomId = Arena_Boss_Guardian
  Level = /Game/Maps/Arenas/Arena_Boss_Guardian
  Encounter = DA_Encounter_Boss
  Depth = 5
  RoomType = Boss
  bOffersEco = false   // boss dá Essência, não Eco
```

**Embaralhamento no `RunStart`** ([42 §5](../systems/42_Run_Room_Manager.md)):

```
Pool = [Crypt, Rift, Catacombs, Chamber, Pit, Spire]  // 6 arenas
Sorteia 4 sem repetição → insere Boss_Guardian no fim
Run = [ex.: Rift → Crypt → Chamber → Pit → Boss]
```

---

## 5. Fluxo técnico sala a sala

```
1. RunManager carrega sublevel da FDDRRoomDefinition
2. EncounterManager.BeginPlay → bind OnPlayerEnter
3. Player cruza trigger / entra volume → StartEncounter()
4. Portas Locked (tag State.Run.Locked)
5. Waves spawn nos SpawnPoints tagueados ([51 §5](../enemies/51_Enemy_Catalog_MVP.md))
6. Última onda morre → UnlockDoors → OnRoomCleared
7. (M4) EcoSelect UI OU preview de 2 portas (47)
8. RunManager AdvanceToNextRoom → OpenLevel/stream próxima arena
```

> Transição rápida entre salas: **streaming** ou sublevels no mesmo mapa pai ([26 §4](../ui/26_Loading_System.md)). Loading screen só Main Menu ↔ Run.

---

## 6. Checklist por arena (antes de dar pronto)

- [ ] NavMesh buildado e cobre todo chão jogável
- [ ] ≥4 spawn points tagueados, longe do `PlayerStart`
- [ ] `ADDREncounterManager` referencia `UDDREncounterData` correto
- [ ] `ADDRRunDoor` tranca/destranca com evento do manager
- [ ] Telegrafe visível no chão (material decal testado em -55°)
- [ ] Player consegue dashar de ponta a ponta sem clip
- [ ] Sem spawn dentro de parede (teste `ProjectPointToNavigation`)
- [ ] Linha no `DT_RunLayout` + teste no embaralhamento
- [ ] (Boss) barra HUD + espaço p/ charge/AoE

---

## 7. Streaming vs OpenLevel (decisão MVP)

| Abordagem | Prós | Contras | MVP |
|---|---|---|---|
| **OpenLevel por sala** | simples | loading entre salas | ✅ aceitável M3 |
| **Persistent + sublevels** | transição rápida | setup mais complexo | 🟡 M4 polish |
| **Tudo num mapa** | zero load | memória · menos variedade visual | ❌ |

> Comece com **OpenLevel** no M3; migre pra streaming quando o loop M4 existir e a trava incomodar.

---

## 8. Pastas de conteúdo

```
Content/
├── Maps/
│   ├── L_MainMenu.uasset
│   ├── L_Hub_Limiar.uasset          (P1 — 29 §5)
│   └── Arenas/
│       ├── Arena_Crypt_01.umap
│       ├── ...
│       └── Arena_Boss_Guardian.umap
├── Data/
│   ├── Run/
│   │   ├── DT_RunLayout.uasset
│   │   └── DA_RunConfig_MVP.uasset
│   └── Enemies/Encounters/          (51 §3)
└── Blueprints/World/
    ├── BP_EncounterManager.uasset
    └── BP_RunDoor.uasset
```

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Inimigos nascem no colo | Spawn point perto do start | §2 — distância mín. 800cm |
| IA não move | NavMesh faltando/buraco | §6 checklist NavMesh |
| Porta não abre | EncounterManager não conta mortes | [33 §10](33_Spawning_Encounters.md) |
| Runs iguais | Sem embaralhar pool | §4 embaralhamento |
| Telegrafe ilegível | Decal escuro / chão busy | §1 — chão plano + contraste |
| Boss apertado | Arena pequena | Boss ≥28×28m §3 |

---

## 10. Próximo passo

→ [51 — Catálogo Inimigos](../enemies/51_Enemy_Catalog_MVP.md) · [33 — Spawning](../enemies/33_Spawning_Encounters.md) · [42 — Run Manager](../systems/42_Run_Room_Manager.md) · [47 — Tipos de Sala](../design/47_Room_Types_Routing.md).
