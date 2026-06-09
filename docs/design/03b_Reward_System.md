# 03b — Sistema de Recompensa / Boons · 🟢 P0 (design) / 🟡 P1 (código)

> Sistema de 1ª classe — **mesmo peso que o combate**. Estende o [§4 do Core Loop](03_Core_Loop_Roguelike.md). Criado na [Revisão de Design 2026-06](Design_Review_2026-06.md) a pedido da cadeira de Sistemas Roguelike (Mara): *"no meu gênero, o loop não é o embrulho — é o produto. Combate profundo com sistema raso = repetição rápida."*

> 📌 **Design no papel agora (P0); código só pós-spike M⁻¹ (P1).** Dimensionar o pool e as sinergias é planilha de design, não código — e precisa existir **antes** do M4, senão o MVP chega no fim com 6 upgrades e parece vazio sem ninguém saber por quê.

---

## 1. A regra de ouro (convergência de 3 cadeiras)

> 🔑 **Um Boon bom transforma COMO você joga — não apenas um número.**

Kael (Combate), Mara (Roguelike) e Sofia (Produto) atacaram o mesmo ponto de ângulos diferentes na revisão: se os upgrades são só "+15% dano", o pilar (combo aéreo) vira a mesma animação 50 runs seguidas e a rejogabilidade morre. **Teste de aceite de todo Boon do MVP:**

```
"Isto muda como eu jogo o combo / a run?"
   SIM → Boon de verdade (maioria do pool)
   NÃO (só mexe num número) → Stat puro (minoria, "tempero", nunca o prato)
```

E o corolário, da cadeira de Combate: **os Boons DEVEM poder modular o pilar** — adicionar hit ao juggle, transformar o slam em AoE de fogo, dar reset de decay. Um roguelike de combo cujos upgrades não tocam o combo desperdiça o pilar.

---

## 2. Pool-alvo (números no papel)

Pra *sentir* roguelike (não só ter o encanamento), mire:

| Métrica | MVP-alvo | Por quê |
|---|---|---|
| **Boons de run (total)** | **15-20** | Com 6, o jogador vê tudo em 2 runs e o "só mais uma" morre. |
| **Famílias temáticas** | **3-4** | Dão identidade de build (estilo "Deuses" do Death Must Die / Olimpo do Hades). |
| **Sinergias explícitas** | **≥ 3-4** | Combinação só importa se houver sinergia. Sem isso, "dezenas de combinações" é falso conforto. |
| **Tiers de raridade** | **3** (Comum/Raro/Lendário) | Cria cobiça e tensão no RNG. |
| **Escolhas por run** | 4-5 (1 de 2-3 por vez) | Suficiente pra sentir build sem paralisia. |

> Não é trabalho de código — é uma **planilha**. Faça-a antes do M4.

---

## 3. Famílias temáticas (exemplo — adapte)

Cada família empurra um **estilo de jogo** diferente. 4 famílias × ~4-5 boons = ~18.

| Família | Fantasia | Exemplos de Boon (note como tocam o pilar) |
|---|---|---|
| **Tempestade** (aéreo) | "o combo aéreo é seu jogo" | *+1 hit no cap do juggle* · *jump-cancel reseta o decay* · *slam vira AoE elétrica* |
| **Carniça** (sustain) | "agressão te cura" | *combo aéreo cura X* · *finisher restaura vida* · *lifesteal no 3º golpe* |
| **Fúria** (risco/dano) | "vidro que explode" | *+dano quanto menos vida* · *dash causa dano* · *crit estende hitstop* |
| **Vazio** (controle) | "manipula o campo" | *launcher puxa inimigos próximos* · *slam atordoa em área* · *kills no ar dropam cura* |

> 🎯 Repare: quase todo exemplo **modifica uma mecânica** (juggle, slam, launcher, dash), não só um stat. Essa é a regra do §1 na prática.

---

## 4. Sinergias (o que gera profundidade real)

Sinergia = Boon A + Boon B produz um efeito **maior que a soma**. Defina ≥ 3-4 explícitas no MVP:

| Sinergia | Combinação | Resultado |
|---|---|---|
| **Furacão** | "+1 hit no juggle" + "jump-cancel reseta decay" | combo aéreo praticamente infinito controlado por skill |
| **Sanguessuga aérea** | "combo aéreo cura" + "+1 hit no juggle" | mais hits no ar = mais cura = sustain de build |
| **Trovão final** | "slam vira AoE elétrica" + "slam atordoa em área" | finisher vira limpa-tela com controle |
| **Vampiro frágil** | "+dano com pouca vida" + "lifesteal no 3º golpe" | dança na corda bamba: forte e perigoso |

> 🧩 Sinergias são o que faz o jogador dizer *"essa run virou uma build"*. É o motor de rejogabilidade — e é barato (são só regras condicionais em GAS, §6).

---

## 5. Raridade & cobiça (transformar menu em decisão)

"1 de 3 opções iguais" é um menu, não uma decisão. Adicione **hierarquia** e **1 eixo de tensão**:

| Tier | Magnitude | Frequência |
|---|---|---|
| **Comum** | efeito base | comum |
| **Raro** | +efeito / 2º gatilho | menos comum |
| **Lendário** | transforma uma mecânica | raro (o que você *torce* pra aparecer) |

**Eixo de tensão (1, barato):** às vezes a escolha oferece um Boon **mais forte com um custo** — *"+30% dano, mas -15% vida máxima"*. Custo ~zero de implementação (é só mais um `GE` com modifier negativo) e transforma cada escolha numa decisão de verdade.

> 🎰 Hades faz você *querer* o Legendary e torcer no RNG. Cobiça é o que prende. Sem tiers, a recompensa é um checkbox.

---

## 6. Como tudo isto vira GAS (encanamento)

Tudo se expressa nas peças do [doc 05](../systems/05_GAS_Architecture.md) — a fundação já está certa:

| Conceito de design | Implementação GAS |
|---|---|
| Boon de stat ("+15% dano") | `GE_RunUpgrade_*` **Infinite** aplicado no ASC |
| Boon de mecânica ("dash causa dano") | concede/**troca** uma `GameplayAbility` |
| Boon condicional ("combo aéreo cura") | `GE` condicional + **GameplayTag** de gatilho (ex.: `State.Combat.InAir`) |
| Sinergia (A+B) | tag de A habilita o efeito extra de B (checagem de tag) |
| Raridade | magnitude via `SetByCaller` / nível do `GE` |
| Custo/tensão | um `GE` único com modifier **negativo** junto |
| Cap de juggle (quem é dono?) | **a ability** (`GA_AirAttack`) é dona do cap; Boons ajustam um atributo `MaxJuggleHits` lido por ela — evita stacking quebrado de `GE` |

> ⚠️ **Pergunta técnica aberta (Davi → Mara):** conceder/trocar/empilhar `GE` Infinite + Abilities **20× por run** (toda sala) é estável no Modelo A (ASC no Character)? **Valide o runtime granting** num teste isolado no M1 — antes que o sistema de recompensa dependa dele no M4. Cuidado com `stacking policy` e leak de spec.

---

## 7. Meta-progressão: destrave ESCOLHAS, não percentuais

Da revisão (Mara): "+5% vida inicial" é progressão **invisível** — não dá dopamina. O Espelho do Hades destrava *escolhas e mecânicas*, não números.

**Reescrita do meta-loop ([Core Loop §5](03_Core_Loop_Roguelike.md)):** gastar o recurso persistente deve, de preferência, **destravar Boons/famílias novas no pool**, **desbloquear uma habilidade inicial**, ou **dar +1 slot de escolha** — coisas que mudam a *próxima run*, não só engordam um stat. Mantenha pequeno no MVP (3-5), mas que cada um **mude algo perceptível**.

---

## 8. Checklist

- [ ] Planilha com **15-20 Boons** em **3-4 famílias** (no papel, antes do M4)
- [ ] Maioria dos Boons **modifica mecânica** (passa no teste do §1), stat puro é minoria
- [ ] Pelo menos **alguns Boons modulam o pilar aéreo** (juggle/slam/launcher)
- [ ] **≥ 3-4 sinergias** explícitas documentadas
- [ ] **3 tiers** de raridade + **1 eixo de custo/tensão**
- [ ] Mapeamento pra GAS definido (§6); **runtime granting validado no M1**
- [ ] Cap de juggle é dono da **ability**, não de `GE` empilhável
- [ ] Meta-upgrades destravam **escolhas/mecânicas**, não só percentuais

---

## 9. Próximo passo

→ Volte ao [Core Loop](03_Core_Loop_Roguelike.md) e ao [Roadmap M4](../17_Implementation_Roadmap.md). O encanamento é [GAS (doc 05)](../systems/05_GAS_Architecture.md). A origem desta priorização: [Design Review 2026-06](Design_Review_2026-06.md).
