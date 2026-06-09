# Revisão de Corpus — Auditoria Completa 2026-06

> Auditoria de **todo o corpus** (53 docs) por 4 agentes-revisores (Design/Roguelike, Combate/Feel/Juice, Locomoção/Inimigos, Sistemas/UI). Registro dos achados e correções aplicadas. Companheiro de [Design_Review_2026-06](Design_Review_2026-06.md) e [Design_Review_Combat_2026-06](Design_Review_Combat_2026-06.md).

> **Nota: 8.5/10 nos 4 clusters.** Corpus forte e maduro — os problemas eram **contratos entre docs** e **divergências de número/tag** acumuladas, não falhas de design. ✅ **Todos os achados abaixo foram aplicados.**

---

## 🔴 Críticos (★ = achado por 2 revisores)

| # | Achado | Doc | Correção aplicada |
|---|---|---|---|
| 1 ★★ | Física do juggle se contradiz (RootMotionSource dirige vs "reduzir GravityScale 0.3-0.5" vs `LaunchCharacter down`) | 16 | RootMotionSource dirige a altura; `GravityScale~0` no Airborne; removidos resíduos de LaunchCharacter/0.3-0.5 |
| 2 ★ | Ecos leem atributos inexistentes no AttributeSet (`MaxJuggleHits`, `JugglePopHeight`, `PoiseDamageMult`, `BonusHitStopFrames`) | 05, 40 | 05 §3 declara os atributos do roguelike; 40 referencia |
| 3 ★ | Cap de juggle diverge ("6-8" vs 7) | 16, 19 | Canônico **7** (default); "6-8" = faixa de tuning |
| 4 | Golpe "Heavy" em 18/21 sem ability em 19 | 19 | Adicionada ficha `GA_Attack_Heavy` |
| 5 | Premissa single-player vaza no GAS (`OnRep_PlayerState`, replicação) | 05 | §8 reescrito p/ Modelo A local; replicação rebaixada a "só co-op" |
| 6 | `IA_Pause` + actions de UI faltam no 07 | 07 | Adicionadas (subconjunto declarado do 39) |
| 7 | HUD põe ultimate na tecla errada (R=ult vs R=Slam) | 28 | `[R] Slam`, `(Q) ult` |

## 🟡 Médios

| Achado | Doc | Correção |
|---|---|---|
| Tag órfã `GameplayCue.Hit.Impact` | 15 | → família `Hit.Light/.Heavy/.Air` |
| Redundância 46 §3 ↔ 47/48/49 | 46 | §3 vira índice de 1 linha → docs-filhos |
| Duas fontes pra "é boss?" | 42 | `ERoomType` absorve `bIsBossRoom` |
| Rota (47) pode dar <4 Ecos vs "4-5/run" | 03b/47 | Combate sempre oferta Eco |
| Linked Anim Layer órfão no 08 | 08 | Referencia 50 + nó no AnimGraph |
| Wall-splat prometido (18) sem entrada em 21 | 21 | Adicionada entrada |
| BT vs StateTree do boss ambíguo | 34 | Crava BT no MVP |
| Dono do token hedge | 32 | Alinhado ao 33 (Encounter Manager) |
| Dup "Dash Air Attack" | 22 | Resolvida |
| Dois USaveGame não cruzados | 25 | `UDDRSettingsSave` nomeado como irmão |

## 🟢 Nice-to-have
- 45 §6: papel mecânico da Voz da Fenda (comenta Pactos) atualizado.
- 41: catálogo de console vars `ddr.*` adicionado.
- 09/08: nota de que `EDDRGait` é a mesma enum compartilhada.

---

## O que está ÓTIMO (preservado)

Disciplina de escopo P0/P1/P2 · **lore diegética** (Eco=eco, combo aéreo=gravidade quebrada) · **divisão feel/juice sem vazamento** · **gancho juggle-pause** (30/32/16/18) · gate `CanLaunch`↔poise · fórmula `100/(100+Armor)` · `LocalOnly` no GAS · separação **07/39** (input vs layout) · números de feel batendo entre 16/19/20/43 · terminologia Eco/Essência/Fenda/Limiar rigorosa.

---

## Meta-lição

Um corpus construído iterativamente (53 docs, parte por agentes, parte pelo dev) acumula **drift de contrato**: um doc decide algo (RootMotionSource, cap=7, atributos novos) e os docs-irmãos não acompanham. A correção é estabelecer **donos canônicos** (05=atributos, 21=intensidade de juice, 19=roster de abilities, 39=layout de input, 42=definição de sala) e fazer os outros **referenciarem**, não redefinirem. A revisão multi-agente pegou isso porque a **convergência** (2 revisores no mesmo ponto) sinaliza os contratos mais frágeis.
