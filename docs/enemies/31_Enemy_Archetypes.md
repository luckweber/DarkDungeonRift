# 31 — Arquétipos de Inimigo (o Roster) · 🟢 P0 (M3)

> A classe base, o modelo **data-driven** de inimigo, e os arquétipos do MVP (+ eixos de variedade pra depois). Pré-req: [30 — AI Overview](30_AI_Overview.md).

> 🥊 **Princípio:** o inimigo é o **canvas do pilar**. Um combo aéreo só é divertido se o inimigo for **gostoso de combar** — boas reações, poise legível, knockdown com peso ([18 §5](../combat/18_Combat_System_Deep.md); os 49 clips de Hit, [22](../22_Animation_Inventory.md)). Projete inimigos como *punching bags satisfatórios*, não só ameaças.

---

## 1. Classe base + modelo data-driven

```
ADDREnemyCharacter : ADDRCharacterBase
   • ASC + UDDREnemyAttributeSet (Health, Poise, MoveSpeed, AttackPower)
   • AIController + BehaviorTree (doc 30)
   • UDDREnemyData (DataAsset) ← define tudo abaixo
```

```cpp
// UDDREnemyData — uma planilha por inimigo, não código por inimigo
UCLASS()
class UDDREnemyData : public UPrimaryDataAsset {
  TSoftObjectPtr<USkeletalMesh> Mesh;          // visual
  float Health, Poise, MoveSpeed;              // stats
  float Mass;                                  // cap de launch (18 §5.4) — gigante não voa fácil
  TArray<TSubclassOf<UGameplayAbility>> Abilities; // ataques (GAS)
  TSoftObjectPtr<UBehaviorTree> BehaviorTree;  // a mente
  FDDRTelegraphConfig Telegraph;               // cor/decal/tempo (32 §1)
  EEnemyRole Role;                             // Grunt / Elite / Ranged / Boss
};
```

> 🗂️ **Data-driven = variedade barata.** Um novo inimigo = um novo DataAsset (stats + abilities + BT), **não** uma classe C++. Trocar mesh/stats/ataque gera arquétipos sem recompilar.

---

## 2. Os arquétipos do MVP (2 + 1)

O [escopo MVP (02 §2)](../design/02_MVP_Scope.md) pede **2 tipos**. Adicione 1 elite pra ensinar poise:

| Arquétipo | Papel | Comportamento | Poise | Combo-victim |
|---|---|---|---|---|
| **🗡️ Grunt (melee)** | a base | persegue → telegrafe curto → golpe → recua | **baixo** → lança fácil | excelente (a cobaia do pilar) |
| **🏹 Atirador (ranged)** | pressão à distância | mantém distância → telegrafe de projétil → atira | médio | precisa fechar distância p/ combar |
| **🛡️ Brutamontes (elite)** | ensina respeito | lento, golpe forte com **hyperarmor** | **alto** → exige combo p/ quebrar | recompensa quem trabalha o poise |

> 🎯 O **Grunt** é o inimigo onde você *aprende e treina o combo aéreo* (poise baixo, lança fácil). O **Brutamontes** é onde o combo *importa* (poise alto → você precisa quebrar a guarda antes de lançar, [18 §5.4](../combat/18_Combat_System_Deep.md)). Os dois juntos já ensinam a profundidade.

---

## 3. Eixos de variedade (pós-MVP, mas planeje)

Variedade barata = combinar eixos, não criar inimigos do zero (referência: Hades/Death Must Die):

| Eixo | Valores |
|---|---|
| **Movimento** | lento-tanque · rápido-enxame · teleporte · voador |
| **Ataque** | melee · ranged · AoE · investida (charger) · invocador |
| **Papel** | grunt · elite · suporte (buffa aliados) · explosivo (kamikaze) |

> 3 eixos × poucos valores = dezenas de inimigos sentidos como distintos, todos via DataAsset + BT compartilhado. **Não** construa 20 classes; construa 1 base + dados.

---

## 4. Stats por arquétipo (ponto de partida — tune)

| Stat | Grunt | Atirador | Brutamontes |
|---|---|---|---|
| **Health** | baixo | baixo | alto |
| **Poise** | ~20 (lança em 1-2 hits) | ~40 | ~120 (lança só após combo) |
| **MoveSpeed** | médio | médio (recua) | lento |
| **Mass** (cap launch) | leve | leve | pesado |
| **Dano** | baixo | médio | alto (telegrafado) |

> Números são chutes — calibre no playtest do M3. Exponha tudo no DataAsset.

---

## 5. Reações & "ser combado" (o canvas do pilar)

Cada arquétipo precisa das reações que vendem o combo ([18 §5.1](../combat/18_Combat_System_Deep.md), clips em [22 §2](../22_Animation_Inventory.md)):

- **Flinch direcional** (4-way) ao tomar hit → leitura topdown de "acertei daqui".
- **Stagger** ao quebrar poise → janela garantida pro combo/launch.
- **State machine aérea** (Knockback→Float→SlamDown→Stun) quando `Airborne` ([16 §3.1](../combat/16_Aerial_Combos.md)) — IA off ([30 §6](30_AI_Overview.md)).
- **Knockdown + Getup** pós-slam.

> Um inimigo sem boas reações é um saco de pancada *sem peso* — o combo aéreo perde a alma. As reações são metade do juice do pilar.

---

## 6. Prioridade MVP

| Item | Prioridade |
|---|---|
| `ADDREnemyCharacter` + `UDDREnemyData` | 🟢 P0 |
| Grunt (melee) + Atirador (ranged) | 🟢 P0 (M3) |
| Brutamontes elite (ensina poise) | 🟡 P1 |
| Reações completas (flinch/stagger/aéreo/knockdown) | 🟢 P0 (pro pilar) |
| Eixos de variedade / +arquétipos | 🔵 P2 |

---

## 7. Checklist

- [ ] `ADDREnemyCharacter` base com ASC + Poise
- [ ] `UDDREnemyData` (DataAsset) dirige stats/abilities/BT/telegrafe
- [ ] Grunt (melee, poise baixo) + Atirador (ranged)
- [ ] Brutamontes elite (poise alto, hyperarmor) — P1
- [ ] Reações: flinch 4-way + stagger + SM aérea + knockdown
- [ ] Poise calibrado (grunt lança fácil; elite exige combo)
- [ ] Variedade futura = novos DataAssets, não novas classes

---

## 8. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Inimigos sentem "iguais" | Tudo na mesma classe sem dados | §1 — DataAsset por arquétipo |
| Combo aéreo sem peso | Reações pobres | §5 — flinch/stagger/aéreo |
| Elite lança fácil demais (sem desafio) | Poise baixo | §4 — subir Poise do elite |
| Grunt não lança (frustra) | Poise alto demais no trash | §4 — Poise ~20 no grunt |

---

## 9. Próximo passo

→ **[51 — Catálogo Inimigos MVP](51_Enemy_Catalog_MVP.md)** (stats, abilities e encontros concretos) · [32 — Comportamento de Combate](32_Enemy_Combat_Behavior.md): telegrafe, hyperarmor, spacing.
