# 40 — Catálogo de Ecos (conteúdo) · 🟢 P0 (design) / 🟡 P1 (assets)

> **O pool completo do MVP:** 18 Ecos em 4 famílias, com nome, efeito, implementação GAS, raridade e sinergias. Materializa o [03b — Sistema de Ecos](03b_Reward_System.md) em conteúdo jogável. Cada linha vira um **`UDDREcoData`** (DataAsset) ou linha em **`DT_EcoPool`** ([44 — Data-Driven](../systems/44_Data_Driven.md)).

> 🔑 **Regra de ouro ([03b §1](03b_Reward_System.md)):** maioria **modifica mecânica**, não só stat. Stat puro = tempero (marcados abaixo).

---

## 1. Resumo do pool

| Família | Tag | Ecos | Foco |
|---|---|:---:|---|
| **Tempestade** | `Eco.Family.Tempest` | 5 | Combo aéreo, juggle, slam |
| **Carniça** | `Eco.Family.Carrion` | 4 | Sustain, lifesteal, cura no ar |
| **Fúria** | `Eco.Family.Fury` | 5 | Dano, risco, dash ofensivo |
| **Vazio** | `Eco.Family.Void` | 4 | Controle, poise, stun |
| **Total** | | **18** | Meta do [03b §2](03b_Reward_System.md) |

### Distribuição de raridade (pool inteiro)

| Tier | Qtd | % do pool |
|---|---|---|
| Comum | 7 | ~39% |
| Raro | 8 | ~44% |
| Lendário | 3 | ~17% |

---

## 2. Tabela mestra — todos os Ecos

| ID | Nome (UI) | Família | Tier | Tipo | Muda o combo? |
|---|---|---|---|---|---|
| `Eco.Tempest.JugglePlus` | **+1 Golpe no Ar** | Tempestade | Raro | Mecânica | ✅ |
| `Eco.Tempest.JumpReset` | **Pulo Recarrega Juggle** | Tempestade | Raro | Mecânica | ✅ |
| `Eco.Tempest.SlamLightning` | **Slam Elétrico** | Tempestade | Lendário | Mecânica | ✅ |
| `Eco.Tempest.PopBoost` | **Impulso Aéreo** | Tempestade | Comum | Stat leve | parcial |
| `Eco.Tempest.ChainLaunch` | **Corrente de Lançamento** | Tempestade | Raro | Mecânica | ✅ |
| `Eco.Carrion.AirHeal` | **Sangria Invertida** | Carniça | Raro | Condicional | ✅ |
| `Eco.Carrion.SlamHeal` | **Slam Restaurador** | Carniça | Comum | Condicional | ✅ |
| `Eco.Carrion.Lifesteal3` | **Terceiro Golpe Rouba Vida** | Carniça | Comum | Condicional | parcial |
| `Eco.Carrion.AirKillDrop` | **Queda Vital** | Carniça | Raro | Condicional | ✅ |
| `Eco.Fury.LowHP` | **Fúria Desesperada** | Fúria | Raro | Condicional | parcial |
| `Eco.Fury.DashStrike` | **Dash Cortante** | Fúria | Raro | Mecânica | ✅ |
| `Eco.Fury.CritHitstop` | **Golpe Crítico Pesa** | Fúria | Comum | Mecânica | parcial |
| `Eco.Fury.GlassCannon` | **Canhão de Vidro** | Fúria | Lendário | Stat+custo | parcial |
| `Eco.Fury.AttackUp` | **Lâmina Afiada** | Fúria | Comum | Stat | ❌ (tempero) |
| `Eco.Void.SlamStun` | **Slam Atordoante** | Vazio | Raro | Mecânica | ✅ |
| `Eco.Void.PoiseUp` | **Quebra de Guarda** | Vazio | Comum | Stat leve | parcial |
| `Eco.Void.LauncherVacuum` | **Vácuo do Uppercut** | Vazio | Raro | Mecânica | ✅ |
| `Eco.Void.SlowWorld` | **Ritmo Lento** | Vazio | Lendário | Mecânica | parcial |

---

## 3. Fichas por família

> 🔗 **Os atributos que os Ecos modificam** (`MaxJuggleHits`, `PopHeightMult`, `PoiseDamageMult`, `BonusHitStopFrames`, `AttackPower`…) são declarados no **registro canônico [05 §3.1](../systems/05_GAS_Architecture.md)**. As fichas abaixo **referenciam** esses atributos — não os redefinem.

### ⚡ Tempestade — `Eco.Family.Tempest`

| ID | Descrição (UI) | Implementação GAS | Sinergia com |
|---|---|---|---|
| **JugglePlus** | "+1 hit máximo no combo aéreo." | `GE_Infinite` → `MaxJuggleHits += 1` (atributo lido por `GA_AirAttack`, [19 §4](../combat/19_Abilities_Deep.md)) | JumpReset → **Furacão** |
| **JumpReset** | "Pulo durante o juggle reseta o decay de altura." | Flag no `GA_Jump` / jump-cancel: zera `decayFactor` do alvo ([16 §3](../combat/16_Aerial_Combos.md)) | JugglePlus → **Furacão** |
| **SlamLightning** | "Slam libera descarga elétrica em área." | `GE` condicional em `GA_AirSlam` → segundo overlap + `GameplayCue.Slam.Lightning` | SlamStun → **Trovão Final** |
| **PopBoost** | "+15% altura dos pops aéreos." | `GE_Infinite` → `PopHeightMult += 0.15` | — |
| **ChainLaunch** | "Launcher puxa inimigos num raio de 3m." | `GA_Launcher` → overlap pré-hit aplica mini-knock toward player | LauncherVacuum (Vazio) |

### 🩸 Carniça — `Eco.Family.Carrion`

| ID | Descrição (UI) | Implementação GAS | Sinergia com |
|---|---|---|---|
| **AirHeal** | "Cada hit aéreo cura 3 HP." | `GE` condicional: tag `State.Combat.InAir` no **atacante** + evento `Combat.Hit` → `GE_Heal` | JugglePlus → **Sanguessuga Aérea** |
| **SlamHeal** | "Slam restaura 8% da vida máxima." | `GE` no impacto de `GA_AirSlam` → heal SetByCaller | — |
| **Lifesteal3** | "O 3º golpe de cada combo rouba vida." | Contador de combo no ASC; no 3º hit → `GE_Damage` reverso como heal | LowHP → **Vampiro Frágil** |
| **AirKillDrop** | "Inimigos mortos no ar dropam orbe de cura." | On death com tag `State.Combat.Airborne` → spawn pickup heal | — |

### 🔥 Fúria — `Eco.Family.Fury`

| ID | Descrição (UI) | Implementação GAS | Sinergia com |
|---|---|---|---|
| **LowHP** | "+1% dano por 10% de vida perdida (máx +8%)." | `GE_Infinite` + curve lida no pipeline de dano ([18 §3](../combat/18_Combat_System_Deep.md)) | Lifesteal3 → **Vampiro Frágil** |
| **DashStrike** | "Dash causa dano em quem cruza o caminho." | Concede `GA_Dash_Strike` ou modifica `GA_Dash` via `UDDREcoComponent` ([19 §6](../combat/19_Abilities_Deep.md)) | — |
| **CritHitstop** | "Críticos estendem hit-stop em +1 frame." | Atributo `BonusHitStopFrames` lido pelo hit-stop ([21 §3](../feel/21_Juice_FX.md)) | — |
| **GlassCannon** | "+30% dano. -15% vida máxima." | `GE_Infinite` dois modifiers + tag `Eco.Cost.Glass` | LowHP (alto risco/recompensa) |
| **AttackUp** | "+12% dano de ataque." | `GE_Infinite` → `AttackPower` | — (stat puro — tempero) |

### 🌑 Vazio — `Eco.Family.Void`

| ID | Descrição (UI) | Implementação GAS | Sinergia com |
|---|---|---|---|
| **SlamStun** | "Slam atordoa inimigos na área por 1.2s." | `GE_Stun` AoE no impacto slam + tag `State.Combat.Stunned` | SlamLightning → **Trovão Final** |
| **PoiseUp** | "+20% dano de poise." | `GE_Infinite` → `PoiseDamageMult` | — |
| **LauncherVacuum** | "Uppercut puxa inimigos próximos antes de subir." | Mesmo hook que ChainLaunch (pode stackar raio) | ChainLaunch |
| **SlowWorld** | "Perfect-dodge estende witch-time em +0.3s." | Modifica duração do slow-mo em `GA_Dash` perfect ([19](../combat/19_Abilities_Deep.md)) | — |

---

## 4. Sinergias explícitas (≥4)

| Nome | Ecos necessários | Efeito extra | Tag de sinergia |
|---|---|---|---|
| **Furacão** | JugglePlus + JumpReset | Juggle quase infinito *controlado por skill* (decay reseta no pulo) | `Eco.Synergy.Hurricane` |
| **Sanguessuga Aérea** | AirHeal + JugglePlus | Mais hits = mais cura; sustain de build aérea | `Eco.Synergy.AerialLeech` |
| **Trovão Final** | SlamLightning + SlamStun | Slam vira limpa-tela com dano + stun + VFX elétrico | `Eco.Synergy.ThunderFinale` |
| **Vampiro Frágil** | LowHP + Lifesteal3 | Build de corda bamba: forte e perigosa | `Eco.Synergy.FragileVampire` |

> 🧩 Na UI de seleção ([29 §3](../ui/29_Run_Flow_Menus.md)), se o jogador já tem um Eco da sinergia, o card do par mostra **"⚡ sinergia!"**.

---

## 5. Como vira asset (Data-Driven)

Cada Eco = uma linha referenciando um **`UDDREcoData`**:

```cpp
// UDDREcoData — Primary Data Asset (ver 44 §3)
UCLASS()
class UDDREcoData : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly) FGameplayTag EcoId;           // Eco.Tempest.JugglePlus
    UPROPERTY(EditDefaultsOnly) FText DisplayName;
    UPROPERTY(EditDefaultsOnly) FText Description;
    UPROPERTY(EditDefaultsOnly) FGameplayTag FamilyTag;       // Eco.Family.Tempest
    UPROPERTY(EditDefaultsOnly) EDDRRarity Rarity;
    UPROPERTY(EditDefaultsOnly) EDDRApplyType ApplyType;      // Stat / Ability / Conditional
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayEffect> EffectClass;
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayAbility> GrantedAbility; // opcional
    UPROPERTY(EditDefaultsOnly) TArray<FGameplayTag> SynergyTags; // Eco.Synergy.*
    UPROPERTY(EditDefaultsOnly) bool bRequiresUnlock = false;   // meta-progressão
};
```

**Pool rolável:** `DT_EcoPool` (DataTable) lista `FDataTableRowHandle` → `UDDREcoData` por sala/profundidade. O `DDRRunManager` sorteia 3 opções da tabela filtrada ([42 §4](../systems/42_Run_Room_Manager.md)).

```
Content/DarkDungeonRift/Data/
  Ecos/
    DA_Eco_Tempest_JugglePlus.uasset
    DA_Eco_Tempest_JumpReset.uasset
    ...
  Tables/
    DT_EcoPool.uasset          ← pool completo MVP
    DT_EcoPoolByDepth.uasset   ← pesos por sala (opcional)
```

---

## 6. Desbloqueios meta (hub)

Ecos com `bRequiresUnlock = true` só entram no pool após compra no hub ([29 §5](../ui/29_Run_Flow_Menus.md)):

| Eco | Custo Essência | Por quê destrava escolha |
|---|---|---|
| SlamLightning | 150 | Lendário — cobiça |
| GlassCannon | 120 | Build de risco |
| SlowWorld | 100 | Perfect-dodge build |
| Família Vazio inteira | 80 | Nova família no pool |

> 🪞 Meta destrava **mecânica**, não só +5% ([03b §7](03b_Reward_System.md)).

---

## 7. Prioridade MVP

| Item | Prioridade |
|---|---|
| 18 Ecos documentados (este doc) | 🟢 P0 |
| `UDDREcoData` + 18 assets | 🟡 P1 (M4) |
| `DT_EcoPool` + sorteio 1-de-3 | 🟡 P1 |
| Sinergias na UI | 🟡 P1 |
| Desbloqueios meta | 🟡 P1 |

---

## 8. Checklist

- [ ] 18 `UDDREcoData` criados a partir desta tabela
- [ ] `DT_EcoPool` referencia todos os Ecos
- [ ] Maioria passa no teste "muda como jogo?" ([03b §1](03b_Reward_System.md))
- [ ] ≥4 sinergias com tag `Eco.Synergy.*`
- [ ] Pelo menos 6 Ecos tocam o pilar aéreo diretamente
- [ ] Nomes/descrições em `FText` ([38](../systems/38_Localization.md))
- [ ] `UDDREcoComponent` aplica no pick ([19 §6](../combat/19_Abilities_Deep.md))

---

## 9. Próximo passo

→ [41 — Gameplay Tags](../systems/41_Gameplay_Tags.md) · [44 — Data-Driven](../systems/44_Data_Driven.md) · [42 — Run Manager](../systems/42_Run_Room_Manager.md) · [03b — Regras de Eco](03b_Reward_System.md).
