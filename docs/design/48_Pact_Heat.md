# 48 — Sistema de Pacto / Heat (Dificuldade Customizada) · 🔵 P2 (mas desenhe agora)

> Detalha a mecânica **D** do [46 — Profundidade](46_Depth_Replayability.md): o jogador **escolhe modificadores de dificuldade** por **mais recompensa e desbloqueios**. É **o rei da retenção de longo prazo** — o que dá centenas de horas quando o conteúdo "acaba". Refs: Hades (*Pacto de Punição*), STS (*Ascensão*), Returnal, Death Must Die.

> 🧭 **Por que P2 mas "desenhe agora":** não é MVP — mas a **DataTable de modificadores** custa pouco no papel e define a arquitetura. Plantar o gancho cedo evita reescrita. A implementação vem pós-jogável.

---

## 1. O conceito — Calor

Antes de descer (no [Limiar](45_World_Lore.md)), o jogador liga **modificadores de Pacto**. Cada um adiciona **Calor (Heat)**. O Calor total escala **recompensa** e destrava **conteúdo**.

```
PACTO DA FENDA          Calor: 7
  ☑ Inimigos +25% vida           +1 🔥
  ☑ Boss ganha fase extra        +2 🔥
  ☑ Você cura 50% menos          +2 🔥
  ☐ Sem reroll de Eco            +1 🔥
  ☑ Inimigos mais rápidos        +2 🔥
                          ─────────────
  Recompensa: +70% Essência · destrava "Eco da Fúria Verdadeira"
              [ DESCER ]
```

> 🪝 **A genialidade:** o jogador **se desafia** e é **recompensado por isso**. O jogo nunca "acaba" — sempre há mais Calor pra subir. É autodirigido, então cada jogador acha sua curva.

---

## 2. Categorias de modificadores

Equilibre entre **buffar inimigo**, **nerfar player** e **apertar economia** — variedade evita "só HP a mais":

| Categoria | Exemplos | Calor |
|---|---|---|
| **Inimigo buff** | +25% vida · +20% dano · +velocidade · +1 onda | +1–2 cada |
| **Boss** | fase extra · enrage por tempo · adds | +2–3 |
| **Player nerf** | cura −50% · i-frame do dash −30% · sem flinch-cancel | +1–2 |
| **Economia** | −30% Essência drop · sem reroll · loja mais cara · −1 escolha de Eco | +1–2 |
| **Tempo/pressão** | timer por sala · poise inimigo regenera mais rápido | +2 |
| **Caos** | mix de elite forçado · santuários só com maldição | +2–3 |

```cpp
// DT_PactModifiers — DataTable (44)
USTRUCT() struct FDDRPactModifier : public FTableRowBase {
    FGameplayTag ModifierId;       // Pact.Enemy.HealthUp
    FText DisplayName;             // FText (38)
    FText Description;
    int32 HeatCost = 1;
    int32 MaxRank = 1;             // alguns escalam (ex.: +vida nível 1/2/3)
    TSubclassOf<UGameplayEffect> RunGlobalEffect;   // aplica na run
    int32 MinHeatToUnlock = 0;     // mods difíceis só com Calor prévio
};
```

---

## 3. Economia do Calor (recompensa & gating)

| Calor total | Recompensa (Essência) | Desbloqueio |
|---|---|---|
| 0 | base | — (sempre disponível) |
| 1–4 | +X% | cosmético / Eco fácil |
| 5–9 | +Y% | Eco/relíquia de meio |
| 10–15 | +Z% | Eco Lendário / família secreta |
| 16+ | máx | título/skin de maestria |

- **Recompensa escala com Calor** (curva em `CurveTable`, [44](../systems/44_Data_Driven.md)).
- **Gating estilo Hades:** certos desbloqueios **exigem vencer com Calor ≥ N** — dá metas claras de longo prazo.
- **Calor mínimo 0 sempre disponível** — nunca obriga dificuldade (anti-frustração).

---

## 4. Como aplica na run (GAS + run config)

```
Ao DESCER com Pacto ativo:
  1. RunManager soma HeatTotal dos mods ligados
  2. Para cada mod → aplica seu RunGlobalEffect:
       - inimigo buff  → GE aplicado no spawn de cada inimigo (33)
       - player nerf   → GE Infinite no player (cura/iframe)
       - economia      → flags no RunConfig (drop rate, reroll off)
  3. Recompensa final ×= HeatRewardMultiplier(HeatTotal)
```

- Reusa **GAS** ([05](../systems/05_GAS_Architecture.md)) — modificadores são `GameplayEffect`s globais da run. Zero sistema novo de "stats".
- Persistido no `UDDRSaveGame` (qual Calor o jogador já venceu, [25](../ui/25_Save_Load_Slots.md)).

---

## 5. UI do Pacto (no Limiar)

| Elemento | Comportamento |
|---|---|
| Lista de modificadores | checkbox + rank (alguns escalam) |
| **Calor total** (ao vivo) | atualiza ao marcar |
| **Preview de recompensa** | mostra +% e desbloqueios no Calor atual |
| Modificadores travados | acinzentados com "requer Calor ≥ N vencido" |
| Botão Descer | confirma o Pacto |

> Tela acessível no [Limiar/hub](../ui/29_Run_Flow_Menus.md) antes de iniciar a run. Navegável por gamepad ([23](../ui/23_UI_Overview.md)).

---

## 6. Lore — o Pacto é diegético ([45](45_World_Lore.md))

> O **Pacto** é uma **barganha com a Fenda**: você pede mais poder/Essência, e a Fenda **empurra de volta com mais força**. "A Voz" ([49](49_Codex_Limiar.md)) comenta seus Pactos — quanto mais Calor, mais ela *se interessa* por você.

Cada modificador pode ter um fragmento de fala da Voz → liga o sistema à narrativa.

---

## 7. Balanceamento & cuidados

| Risco | Mitigação |
|---|---|
| Mod "obrigatório" (sempre o melhor custo/benefício) | revise custos de Calor no playtest; nenhum domina |
| Calor injogável cedo | `MinHeatToUnlock` esconde os brutais até o jogador estar pronto |
| Recompensa trivializa | curva de recompensa sublinear (não dobra Essência fácil) |
| Frustração | Calor 0 sempre disponível; Pacto é **opt-in** total |

---

## 8. Prioridade

| Item | Prioridade |
|---|---|
| `DT_PactModifiers` desenhada (no papel) | 🟡 P1 (planejar) |
| Aplicação via GE na run | 🔵 P2 |
| UI do Pacto no Limiar | 🔵 P2 |
| Gating de desbloqueio por Calor | 🔵 P2 |
| Comentários da Voz por Pacto | 🔵 P2 |
| Curva de recompensa por Calor | 🔵 P2 |

> 🎯 **Pré-condição:** o jogo precisa estar **divertido no Calor 0** primeiro (MVP). Pacto **multiplica** diversão existente — não conserta um combate fraco.

---

## 9. Checklist

- [ ] `DT_PactModifiers` com ~10-15 modificadores em 5+ categorias
- [ ] Custos de Calor balanceados (nenhum domina)
- [ ] `MinHeatToUnlock` nos mais brutais
- [ ] Aplicação via `GameplayEffect` global da run (GAS)
- [ ] Curva de recompensa por Calor (`CurveTable`)
- [ ] Gating de desbloqueios por "Calor vencido" (save)
- [ ] UI do Pacto no Limiar (preview de recompensa ao vivo)
- [ ] Calor 0 sempre jogável (opt-in)

---

## 10. Próximo passo

→ [46 — Profundidade](46_Depth_Replayability.md) (o menu completo) · [47 — Tipos de Sala](47_Room_Types_Routing.md) · [40 — Ecos](40_Eco_Pool_Catalog.md) (desbloqueios gated por Calor) · [49 — Voz/Codex](49_Codex_Limiar.md).
