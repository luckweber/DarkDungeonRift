# 01 — Visão do Jogo

> **Dark Dungeon Rift** — um ARPG **topdown** roguelike de **hack'n'slash** onde a assinatura é **combate de combo no chão e no ar**, dentro de masmorras geradas que se reconfiguram a cada *run*.

---

## 1. Pitch de uma frase

> "Hades encontra Devil May Cry visto de cima": você desce numa fenda (*rift*) que muda toda run, encadeia combos que **lançam inimigos pro alto** e os mantém no ar, e morre pra ficar mais forte.

---

## 2. Pilares de design

Tudo que entrar no jogo precisa servir a **pelo menos um** pilar. Se não serve, corta.

| # | Pilar | O que significa na prática |
|---|---|---|
| **P1** | **Combate expressivo** | Combos com timing, cancelamentos, e o aéreo como skill ceiling. O jogador *bom* se sente bom. |
| **P2** | **Run sempre diferente** | Layout, inimigos, recompensas e build mudam toda descida. Curiosidade > repetição. |
| **P3** | **Morte com propósito** | Morrer destrava/avança algo. A run perdida vira progresso (meta). |
| **P4** | **Leitura clara (topdown)** | Tudo legível de cima: telegrafe de ataque, área de perigo, posição. Clareza > realismo. |

> ⚠️ O **Pilar 4** é o que faz a doc de locomoção priorizar combate sobre detalhe de pés. Em topdown, **leitura** é a moeda.

---

## 3. A fantasia (o que o jogador sente)

- **No controle:** o personagem responde *imediato*. Sem input lag, sem "patinar" pra começar a andar.
- **Poderoso, mas mortal:** corta hordas com estilo, mas um erro de posicionamento mata.
- **Em fluxo:** dash → ataque → launcher → combo aéreo → slam → próximo grupo, sem parar.
- **Curioso:** "o que tem na próxima sala? que upgrade vou achar?"

---

## 4. A assinatura: combo aéreo em topdown

É o que diferencia o jogo de um Diablo-like comum. Detalhes no [doc 16](../combat/16_Aerial_Combos.md), mas a visão:

- **Launcher:** um ataque específico arremessa o inimigo (e opcionalmente o player) pro alto.
- **Juggle:** no ar, a gravidade do alvo cai; ataques aéreos o mantêm flutuando.
- **Slam/Finisher:** um golpe descendente fecha o combo com impacto (hit-stop + shake + dano em área no pouso).
- **Topdown twist:** como a câmera é de cima, a "altura" é comunicada por **sombra no chão**, **escala**, e **deslocamento vertical na tela** — não por perspectiva. Isso é um desafio de leitura que a doc de câmera/combate trata.

---

## 5. Referências (e o que roubar de cada)

| Referência | O que aprender |
|---|---|
| **[Hades](https://hades.fandom.com/wiki/Hades_Wiki)** ↗ wiki | Core loop roguelike, ritmo de sala, **sistema de boons/bênçãos** (escolha de upgrade entre salas), feedback de hit, meta-progressão que vicia. **O modelo a seguir pro [core loop](03_Core_Loop_Roguelike.md).** |
| **[Death Must Die](https://store.steampowered.com/app/2334730/Death_Must_Die/)** ↗ Steam | **A referência de gênero mais próxima:** ARPG **topdown** roguelite, dodge como verbo, **builds por "Deuses"/bênçãos**, progressão run-a-run de itens, leitura de horda densa de cima. ⚠️ É mais *bullet heaven* que combo — roube a **estrutura de run e o sistema de build**, não o combate (o combate vem do DMC). |
| **Devil May Cry / Bayonetta** | Gramática de combo, **launcher/juggle**, ranking de estilo, cancelamentos. **A alma do combo aéreo (Pilar 1).** |
| **Diablo / Path of Exile** | Câmera topdown, densidade de inimigos, leitura de habilidade em área. |
| **Hyper Light Drifter** | Dash como verbo central, clareza visual, combate de posicionamento. |
| **Returnal** | "Bullet-hell" topdown legível, agressão recompensada, morte como reset narrativo. |

> 📚 **Referências externas linkadas:** [Hades Wiki](https://hades.fandom.com/wiki/Hades_Wiki) (estude o sistema de **Boons** e a estrutura de câmaras) · [Death Must Die — Steam](https://store.steampowered.com/app/2334730/Death_Must_Die/) (estude **builds de Deuses**, dodge e leitura topdown). Ambas alimentam o [doc 03 — Core Loop](03_Core_Loop_Roguelike.md).

---

## 6. Identidade audiovisual (direcional, não MVP)

- **Tom:** sombrio mas **legível** — não escureça a ponto de perder leitura de combate.
- **Paleta:** fundo dessaturado/escuro, **inimigos e telegrafes em cor saturada** (perigo "salta" da tela).
- **Câmera:** topdown 3/4, leve *screen shake* e *hit-stop* no impacto (juice barato, alto retorno).

> 🎨 Arte final é **pós-MVP**. O MVP usa o Mannequin/placeholder. O que **não** é pós-MVP: legibilidade (silhueta clara, telegrafe visível).

---

## 7. O que este jogo **NÃO** é

Definir o "não" evita scope creep:

- ❌ Não é um ARPG de loot infinito estilo Diablo (foco é combate ativo, não build de planilha).
- ❌ Não é mundo aberto. É **run estruturada** sala-a-sala.
- ❌ Não é cobertura/stealth. Crouch é, no máximo, P2.
- ❌ Não é multiplayer no MVP (mas o GAS já deixa a porta aberta — ver doc 05).

---

## 8. Próximo passo

→ [02 — Escopo do MVP](02_MVP_Scope.md): traduzir esta visão em **o que de fato vamos construir primeiro**.
