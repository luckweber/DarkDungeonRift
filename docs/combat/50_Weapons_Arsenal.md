# 50 — Armas & Arsenal · 🔵 P2 (sistema) / 🟢 P0 (1 arma no MVP)

> Sistema de **arma = identidade**: cada arma muda **(1) habilidades próprias**, **(2) animações de locomoção próprias** (via Linked Anim Layers) e **(3) ataques/move-set próprios**. Estende [18 — Combate Profundo](18_Combat_System_Deep.md) (`UDDRAttackSet` já é "por arma"), [08 — Locomoção](../locomotion/08_Locomotion_Overview.md) e [19 — Abilities](19_Abilities_Deep.md).

> 🧭 **Disciplina de escopo:** o **MVP tem 1 arma fixa** ([02](../design/02_MVP_Scope.md)) — mas este doc desenha o sistema pra que **arma #2 custe pouco**: novo DataAsset + camada de anim + attack set, **zero rearquitetura**. Arsenal completo é **pós-MVP** (eixo de replay, [46](../design/46_Depth_Replayability.md)).

---

## 0. O que uma arma muda (os 3 eixos que você pediu)

| Eixo | Muda o quê | Mecanismo UE |
|---|---|---|
| **1. Habilidades próprias** | combo/launcher/slam/**especial** diferentes | GAS — set de abilities por arma ([§3](#3)) |
| **2. Locomoção própria** | idle/walk/run/sprint/jump diferentes | **Linked Anim Layers** ([§2](#2)) |
| **3. Ataques próprios** | montages, frame data, alcance, combo | `UDDRAttackSet` por arma ([§4](#4)) |

> 🎯 É isso que faz uma espada **sentir** diferente de uma lança — não é só dano, é como o personagem **anda, ataca e se move**. Identidade completa.

---

## 1. O que uma `UDDRWeaponData` define

```cpp
UCLASS()
class UDDRWeaponData : public UPrimaryDataAsset
{
    // identidade
    UPROPERTY(EditDefaultsOnly) FGameplayTag WeaponId;        // Weapon.Blade
    UPROPERTY(EditDefaultsOnly) FText DisplayName;            // FText (38)
    UPROPERTY(EditDefaultsOnly) FGameplayTag FamilyAffinity;  // Eco.Family.Tempest (lore, §9)

    // visual
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<USkeletalMesh> WeaponMesh;
    UPROPERTY(EditDefaultsOnly) FName AttachSocket = "hand_r";

    // 🔑 2. ANIMAÇÃO DE LOCOMOÇÃO (Linked Anim Layer — §2)
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UAnimInstance> LocomotionLayer; // ABP_WeaponLayer_Blade

    // 🔑 3. ATAQUES (move-set — §4)
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UDDRAttackSet> AttackSet;        // combo frame data + montages (18 §1)

    // 🔑 1. HABILIDADES (§3)
    UPROPERTY(EditDefaultsOnly) TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities; // ex.: GA_Blade_Special
    UPROPERTY(EditDefaultsOnly) FDDRWeaponFeel Feel;          // attack speed, range, poise mult

    // VFX/SFX
    UPROPERTY(EditDefaultsOnly) TSoftObjectPtr<UNiagaraSystem> TrailFX;     // NS_Slash_Trail por arma (36)
    UPROPERTY(EditDefaultsOnly) TObjectPtr<USoundBase> SwingSFX;            // por arma (35)
};
```

> 🗂️ **Data-driven** ([44](../systems/44_Data_Driven.md)): uma arma = um DataAsset. Trocar/adicionar arma não toca C++.

---

## 2. 🔑 Animações de locomoção próprias = Linked Anim Layers {#2}

> **Este é o coração do "cada arma tem locomoção própria".** É o padrão UE5 padrão (e o que você já usa no DungeonForged: `ABP_TestLayerBase` / weapon layer).

**A ideia:** a **lógica** de locomoção é compartilhada (uma só); as **poses** vêm de uma **camada linkada por arma**.

```
UDDRAnimInstance (base do player, doc 08)
  • DETÉM A LÓGICA: FDDRLocomotionState, state machines, cálculo (08 §2)
  • implementa um Anim Layer Interface: ALI_DDRLocomotion
        ↓ (LinkAnimClassLayers no equip)
ABP_WeaponLayer_Blade / _Spear / _Fists  (1 por arma)
  • DETÉM AS POSES: idle/walk/run/sprint/jump DAQUELA arma
  • implementa ALI_DDRLocomotion com os clips próprios
```

**No equip** ([§6](#6)):
```cpp
Mesh->LinkAnimClassLayers(WeaponData->LocomotionLayer);   // troca as poses pra as da arma
// (UnlinkAnimClassLayers / LinkAnimClassLayers da nova ao trocar)
```

| O que fica na BASE (compartilhado) | O que fica na CAMADA (por arma) |
|---|---|
| `FDDRLocomotionState` (Speed, Direction, Gait, bIsFalling — [08 §2](../locomotion/08_Locomotion_Overview.md)) | as **animações** de idle/walk/run/sprint/jump |
| state machines, distance match, warping | os **clips** que cada estado toca |
| a decisão "estou correndo pra NE" | *como* essa arma corre pra NE |

> 🔑 Isto É o **"Clean Extendable Logic"** do [08 §2](../locomotion/08_Locomotion_Overview.md): adicionar uma arma = **1 nova camada de poses**, sem tocar a lógica. A base calcula; a camada veste. Escala pra N armas sem reescrever o AnimGraph.

> ⚠️ Por isso o [08 §3](../locomotion/08_Locomotion_Overview.md) nota que o bespoke "escala mal pra N armas" **se** você duplicar a lógica — Linked Anim Layers é a resposta: lógica única, poses por camada.

---

## 3. Habilidades próprias por arma (GAS) {#3}

Duas formas, combinadas (híbrido recomendado):

| Padrão | Como | Use para |
|---|---|---|
| **A — Abilities compartilhadas lendo a arma** | um `GA_Attack_Light` único lê o `AttackSet` + montages da arma equipada | os **verbos comuns** (combo, launcher, air, slam) — menos proliferação |
| **B — Ability concedida por arma** | a arma concede `GA_<Weapon>_Special` (mecânica única) | a **assinatura** de cada arma |

```
No equip:
  • abilities comuns (Attack/Launcher/AirAttack/AirSlam, doc 19) já estão concedidas
    → elas leem WeaponData->AttackSet (qual montage/frame data tocar)
  • + concede WeaponData->GrantedAbilities (a especial da arma)
No unequip: ClearAbility das GrantedAbilities da arma anterior
```

> 🎯 **Input não muda, ability muda.** `IA_Attack` é sempre o mesmo botão ([07](../systems/07_Input.md)); o que ele faz depende da arma equipada (move-set) + a especial é exclusiva. Mesmo padrão de roteamento por tag do [19 §2](19_Abilities_Deep.md).

> 🔗 **Eco × arma:** Ecos ([40](../design/40_Eco_Pool_Catalog.md)) continuam funcionando — modificam as abilities comuns. Um Eco "+1 juggle" vale pra qualquer arma. Ecos da **família afim** da arma ([§9](#9)) podem ter sinergia extra.

---

## 4. Ataques / move-set próprios (`UDDRAttackSet`) {#4}

O [18 §1](18_Combat_System_Deep.md) **já** prevê isto: `FDDRAttackData` mora num **`UDDRAttackSet` por arma**. Cada arma define:

| Por arma | Exemplo |
|---|---|
| **Estrutura do combo** | espada: 4 golpes rápidos · montante: 3 golpes pesados |
| **Frame data** | startup/active/recovery por golpe ([18 §1](18_Combat_System_Deep.md)) |
| **Alcance/forma do hitbox** | lança: longo/fino · manoplas: curto/largo |
| **Poise damage** | montante quebra guarda rápido; espada não |
| **Launcher/slam próprios** | cada arma sobe/desce o alvo do seu jeito |
| **Montages** | `AM_Blade_Combo`, `AM_Spear_Combo`... |

A ability comum (`GA_Attack_Light`) indexa `EquippedWeapon->AttackSet` por `ComboIndex` ([19 §4](19_Abilities_Deep.md)).

---

## 5. Arquétipos de arma (variedade — pós-MVP) {#7}

Cada arma muda **como você comba** (não só números). Exemplos topdown + pilar aéreo:

| Arma | Identidade | Locomoção | Aéreo (o pilar) | Afinidade |
|---|---|---|---|---|
| **🗡️ Lâmina** (MVP) | equilibrada, combo rápido | ágil, padrão | juggle versátil, vários hits | Tempestade |
| **⚔️ Montante** | lenta, pesada, poise alto | passos largos, "peso" | launcher/slam fortes, poucos hits grandes | Fúria |
| **🔱 Lança** | alcance, espeta à distância | postura de guarda | launch vertical longo, mantém alvo alto | Vazio |
| **🥊 Manoplas** | rápida, muitos hits, mobilidade | leve, dash-combo | juggle longo estilo DMC (Gilgamesh) | Tempestade |
| **⛓️ Foice/Corrente** | alcance + puxão, controle de grupo | fluida | **pull-launch** (puxa e sobe) | Carniça/Vazio |

> 🎨 Cada uma = **camada de locomoção própria + attack set + especial + afinidade**. A variedade vem de *como joga*, não de stats. (Referência: armas do Hades.)

---

## 6. Fluxo de equipar {#6}

```
EquipWeapon(UDDRWeaponData* W):
  1. (async load) W->WeaponMesh → attach no AttachSocket
  2. Mesh->LinkAnimClassLayers(W->LocomotionLayer)     ← poses da arma (§2)
  3. EquippedWeapon = W; ComboComponent->AttackSet = W->AttackSet   (§4)
  4. ASC: ClearAbility(arma anterior) + GiveAbility(W->GrantedAbilities)  (§3)
  5. aplica W->Feel (attack speed/range) + VFX/SFX overrides
  6. (opcional) toca anim de saque (equip, doc 22) — P2
```

- **MVP:** 1 arma, equipada no `BeginPlay`. Sem troca.
- **Pós-MVP:** escolha de arma no **Limiar/hub** ([29 §5](../ui/29_Run_Flow_Menus.md)) antes da run; troca in-run é opcional (e mais complexa).
- Anim de **saque/troca** (equip/sheathe) está nos assets ([22](../22_Animation_Inventory.md)) — P2.

---

## 7. Aspectos (variantes estilo Hades) — pós-MVP {#8}

Cada arma pode ter **Aspectos**: variantes que mudam **uma regra** do move-set (rejogabilidade enorme, baixo custo de asset).

| Exemplo | Aspecto |
|---|---|
| Lâmina | "o 4º golpe vira 2 launchers" |
| Montante | "slam não tem recovery — encadeia" |
| Manoplas | "perfect-dodge recarrega o juggle" |

> Desbloqueado por Essência no hub ([40 §6](../design/40_Eco_Pool_Catalog.md)). É o **eixo de profundidade de longo prazo** das armas — casa com o [Pacto (48)](../design/48_Pact_Heat.md) e o [menu de replay (46)](../design/46_Depth_Replayability.md).

---

## 8. Lore — armas são relíquias dos caídos {#9}

> Cada arma é uma **relíquia de um caído** / forjada na Fenda ([45](../design/45_World_Lore.md)). Equipá-la é empunhar o legado de uma entidade caída → a **afinidade** liga a arma a uma família de Eco.

- A **Lâmina** ecoa o Trovejante (Tempestade); o **Montante**, o Campeão Quebrado (Fúria); etc.
- Desbloquear uma arma = recuperar uma relíquia (gancho de codex, [49](../design/49_Codex_Limiar.md)).
- Dá **identidade narrativa** a cada arma, não só mecânica.

---

## 9. Integração & disciplina MVP {#10}

| Sistema | Encaixe |
|---|---|
| `UDDRAttackSet` | **já é por arma** ([18 §1](18_Combat_System_Deep.md)) — zero mudança |
| Linked Anim Layers | base lógica ([08](../locomotion/08_Locomotion_Overview.md)) + camada por arma ([§2](#2)) |
| GAS | abilities comuns leem a arma + especial concedida ([19](19_Abilities_Deep.md)) |
| Ecos | funcionam com qualquer arma; afinidade dá sinergia ([40](../design/40_Eco_Pool_Catalog.md)) |
| Data-driven | `UDDRWeaponData` ([44](../systems/44_Data_Driven.md)) |
| Lore/Replay | relíquias ([45](../design/45_World_Lore.md)) + aspectos ([46](../design/46_Depth_Replayability.md)) |

> 🎯 **No MVP:** **1 arma (Lâmina)**, mas montada **já como `UDDRWeaponData` + 1 Linked Anim Layer**. Assim arma #2 = só conteúdo (DataAsset + camada + attack set), não engenharia. **Faça o sistema certo com 1 arma**; colha o arsenal depois.

---

## 10. Prioridade

| Item | Prioridade |
|---|---|
| 1 arma como `UDDRWeaponData` + Linked Anim Layer | 🟢 P0 (arquitetura MVP-ready) |
| `UDDRAttackSet` por arma ([18](18_Combat_System_Deep.md)) | 🟢 P0 (já previsto) |
| Abilities comuns lendo a arma + 1 especial | 🟡 P1 |
| 2ª+ arma (arquétipos §5) | 🔵 P2 |
| Troca de arma no Limiar + anim de saque | 🔵 P2 |
| Aspectos (§7) + afinidade de Eco | 🔵 P2 |

---

## 11. Checklist

- [ ] `UDDRWeaponData` (DataAsset) com mesh/layer/attackset/abilities/feel
- [ ] **Anim Layer Interface** `ALI_DDRLocomotion` na base ([08](../locomotion/08_Locomotion_Overview.md))
- [ ] 1 `ABP_WeaponLayer_*` (poses da Lâmina) linkado no equip
- [ ] Base **só lógica**, camada **só poses** (não duplicar lógica)
- [ ] `UDDRAttackSet` da arma indexado pela `GA_Attack` ([19 §4](19_Abilities_Deep.md))
- [ ] Abilities comuns leem a arma + especial concedida no equip
- [ ] `EquipWeapon` faz `LinkAnimClassLayers` + grant + mesh + set
- [ ] Sistema montado com **1 arma** (arma #2 = só conteúdo)
- [ ] Afinidade de família ([45](../design/45_World_Lore.md)) na `UDDRWeaponData`

---

## 12. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Locomoção não muda ao trocar arma | Não linkou a camada | §6 — `LinkAnimClassLayers(W->LocomotionLayer)` |
| Poses "somem"/T-pose ao equipar | Camada não implementa o Anim Layer Interface | §2 — implementar `ALI_DDRLocomotion` |
| Combo da arma errada | `AttackSet` não trocado no equip | §6 passo 3 |
| Especial da arma anterior persiste | Faltou `ClearAbility` no unequip | §3 |
| Lógica duplicada por arma (caro) | Pôs lógica na camada | §2 — lógica na base, poses na camada |
| Adicionar arma exige mexer no AnimGraph | Não usou Linked Layers | §2 — é o padrão pra escalar |

---

## 13. Próximo passo

→ [18 — Combate Profundo](18_Combat_System_Deep.md) (`UDDRAttackSet`) · [08 — Locomoção](../locomotion/08_Locomotion_Overview.md) (base lógica das camadas) · [19 — Abilities](19_Abilities_Deep.md) · [46 — Profundidade](../design/46_Depth_Replayability.md) (armas como eixo de replay).
