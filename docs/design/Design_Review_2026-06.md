# Revisão de Design — Mesa-Redonda 2026-06

> Registro de uma revisão de design do **Dark Dungeon Rift** conduzida por 4 agentes-especialistas, cada um com uma persona/viés distinto, lendo a documentação completa. Este doc preserva o **porquê** das mudanças aplicadas ao plano. Modelo: revisão multi-perspectiva (estilo `2026-05_AAA_Review` do projeto irmão DungeonForged).

---

## 1. O painel

| Cadeira | Persona | Lente / viés |
|---|---|---|
| ⚔️ Combate / Game Feel | **Kael** | Escola character-action (DMC/Bayonetta): profundidade de combo, juice, skill ceiling |
| 🎲 Sistemas Roguelike | **Mara** | Escola Hades/Death Must Die: loop, build, meta, rejogabilidade |
| 🔧 Técnico UE5/GAS | **Davi** | Viabilidade, risco técnico, arquitetura, networking |
| 🎯 Produto / Jogador | **Sofia** | Fun real, diferenciação de mercado, escopo honesto |

---

## 2. Posições de abertura (resumo)

- **Kael:** o plano descreve um *sistema de hits*, não uma *gramática de combo*. A malha de cancelamentos — que É o jogo character-action — está como TODO ("decisão"). Hit-stop como pausa de montage vai "vazar" no aéreo.
- **Mara:** combate excelente pendurado num roguelike de meia página. No gênero, **o loop é o produto**. Pool de upgrade raso, sem sinergias, sem raridade → vício não se prova. Meta "+5% vida" é dopamina invisível.
- **Davi:** plano honesto na arquitetura, mas o juggle é **prosa de game feel, não contrato técnico**. `LaunchCharacter` em dois corpos com gravidades diferentes diverge na 2ª pancada. State machine do alvo indefinida. Networking é dívida com juros.
- **Sofia:** docs lindos são scope creep sofisticado — zero frame jogável ainda. A suposição mais arriscada do projeto (combo aéreo *fun E legível* em topdown) **nunca foi testada**. Pare de escrever, construa o spike.

> Posições completas preservadas no histórico da sessão que gerou esta revisão.

---

## 3. Consenso forte (unânime ou quase)

1. **A priorização macro está certa** — M2 como teste de fun, 8/12 recursos de locomoção em P2, "procedural sem procedural". Ninguém brigou com a espinha.
2. **GAS para upgrades é a fundação correta** (Mara + Davi).
3. **🔥 Validar o pilar ANTES da infra** — Davi (risco técnico) e Sofia (risco de fun) convergiram **independentemente** num **spike descartável** do combo aéreo antes do M0. Maior consenso + maior ROI da mesa.

---

## 4. Tensões (o debate)

| # | Tensão | Cadeiras | Resolução adotada |
|---|---|---|---|
| 1 | **Profundidade vs Escopo** | Kael & Mara (mais profundidade) × Sofia (menos, construa) | Sequência, não contradição: **spike primeiro** (Sofia define *quando*); profundidade de combate/build recebe orçamento *depois* que o núcleo se provar (Kael/Mara definem *onde*). |
| 2 | **🔥 Ecos TÊM que modificar o combo** | Kael + Mara + Sofia convergiram | Virou **princípio de design** no [03b §1](03b_Reward_System.md): todo Eco passa no teste "muda como eu jogo?". |
| 3 | **🔥 Follow-launch × legibilidade topdown** | Sofia × Kael (Davi por baixo) | **Não resolvida — é o que o spike M⁻¹ responde.** Documentada como "pergunta de um milhão" em [16 §6](../combat/16_Aerial_Combos.md). |

---

## 5. Veredito → ações aplicadas

| # | Ação | Origem | Onde foi aplicado | Status |
|---|---|---|---|:---:|
| 1 | **M⁻¹: spike de validação** do combo aéreo antes do M0 | Davi+Sofia | [Roadmap §1.5](../17_Implementation_Roadmap.md) | ✅ |
| 2 | **Launcher = `RootMotionSource` ancorado** (não `LaunchCharacter`) | Davi | [16 §2](../combat/16_Aerial_Combos.md) | ✅ |
| 3 | **State machine do ALVO** separada do `bIsFalling` do player | Davi | [16 §3.1](../combat/16_Aerial_Combos.md) | ✅ |
| 4 | **Ecos = sistema de 1ª classe** (pool 15-20, famílias, sinergias, raridade, "modificam o combo") | Mara | **novo doc [03b](03b_Reward_System.md)** | ✅ |
| 5 | **Gramática de cancelamento** (tabela estado×ação×janela) elevada a **P0 do M1** | Kael | [15 §6](../combat/15_Combat_Overview.md) + [Roadmap M1](../17_Implementation_Roadmap.md) | ✅ |
| 6 | **Hit-stop = freeze global** (congela CMC, não só montage) | Kael | [15 §5](../combat/15_Combat_Overview.md) | ✅ |
| 7 | **Modelo numérico do juggle** + `ddr.JuggleDebug` / `ddr.CombatDebug` | Kael | [16 §3](../combat/16_Aerial_Combos.md), [15 §6](../combat/15_Combat_Overview.md) | ✅ |
| 8 | **Meta destrava escolhas, não percentuais** | Mara | [03b §7](03b_Reward_System.md) + nota em [03 §5](03_Core_Loop_Roguelike.md) | ✅ |
| 9 | **IA (M3) precisa de orçamento** (não é "uma linha de tabela") | Sofia | nota em [Roadmap M3](../17_Implementation_Roadmap.md) | ✅ |

---

## 6. Perguntas abertas (para o spike / próximos playtests)

- ❓ **Follow launch ajuda ou destrói a leitura topdown?** (tensão 3 — o spike M⁻¹ existe pra isto)
- ❓ **Runtime granting de `GE`/Abilities 20×/run é estável** no Modelo A? (Davi→Mara; validar no M1)
- ❓ **Qual o gancho da 2ª run antes do meta-loop (M5) existir?** (Mara→Produto; não confundir "fun de combate" com "vício de loop" no playtest do M4)

---

## 7. Meta-lição

O plano estava **forte na organização e na priorização macro**, mas tinha um padrão claro: **profundo onde é fácil documentar (combate mecânico, locomoção), raso onde é difícil e arriscado (gramática de combo, sistema de build, viabilidade do juggle, prova de fun)**. A revisão puxou as 4 áreas rasas pra cima e — o mais importante — inverteu a ordem pra **validar a suposição existencial primeiro**. Próximo passo concreto: o **spike M⁻¹**.
