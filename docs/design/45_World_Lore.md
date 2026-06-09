# 45 — Mundo & Lore (a Fenda e os Ecos) · 🟡 P1 (moldura 🟢 / narrativa pesada 🔵)

> A ficção do Dark Dungeon Rift — e como ela **amarra cada mecânica à narrativa**. O melhor da lore aqui não é enredo: é que *"Eco"*, *"morte"*, *"hub"* e *"combo aéreo"* ganham **razão diegética**. Estende a [Visão (01)](01_Game_Vision.md).

> 🧭 **MVP:** a **moldura** (Fenda, Ecos = ecos, Limiar, morte que volta) é **barata de semear** e dá alma ao loop. A **narrativa pesada** (diálogos, cutscenes, codex) é **pós-MVP**. Mas decida a moldura **agora** — ela colore tudo.

---

## 0. Pitch narrativo (1 parágrafo)

> Uma **Fenda** rasgou o mundo — uma ferida viva que devora o reino e se **reconfigura** a cada descida. Você é um(a) **Andarilho da Fenda**: não pode morrer de verdade — cada queda te **desfaz** de volta ao **Limiar**. Lá embaixo, a física é quebrada e os mortos não somem: viram **Ecos**, fragmentos de poder dos heróis e deuses que caíram antes. Você empresta esses Ecos, desce de novo, e tenta alcançar o **Coração da Fenda** — onde algo espera, chamando.

---

## 1. O mundo & a Fenda

- O reino caiu. No seu centro abriu-se a **Fenda** — não um lugar, mas uma **ferida na realidade**. Espaço e tempo dentro dela estão **partidos**.
- Por isso ela **se reconfigura**: não é uma masmorra fixa, é um *ferimento* que cicatriza errado a cada vez. (← justifica o "procedural sem procedural", [03 §2](03_Core_Loop_Roguelike.md).)
- Quem entra é devorado. Quem **sobrevive** sai mudado — ou não sai.

> 🎨 **Tom (Pilar 4):** sombrio, melancólico, **mas legível**. Não é horror que esconde; é mistério que *puxa*. "O que tem lá no fundo?" é o motor.

---

## 2. O Andarilho da Fenda (você) — e por que não morre

- Você está **ligado(a) à Fenda** — talvez o(a) último(a) de uma ordem, talvez já um eco você mesmo(a). Detalhe revelado pela narrativa (pós-MVP).
- **Não morre de verdade:** cada "morte" te **desfaz** e te cospe de volta ao **Limiar** (§6). A Fenda esquece você; você **não** esquece.
- 🔑 Isso é o **Pilar 3 (morte com propósito)** virado ficção: a morte não é game over — é o **mecanismo** da jornada. Você *precisa* cair pra voltar mais forte.

---

## 3. Por que a Fenda reseta (o roguelike em ficção)

| Mecânica | Razão diegética |
|---|---|
| **Run reseta** | a Fenda existe fora do tempo; ao cair, você é *desfeito* e ela se **reembaralha** |
| **Ecos somem na morte** | poder emprestado dos caídos **retorna à Fenda** quando você cai |
| **Meta persiste (Essência)** | você **carrega** Essência através do Limiar — ela atravessa, a Fenda não a reclama |
| **Salas mudam** | a ferida nunca cicatriza igual |

> Tudo que o roguelike "exige" mecanicamente tem desculpa na ficção. Nada parece arbitrário.

---

## 4. 🔑 Os Ecos = ecos dos que caíram (o coração da lore)

> **"Eco" não é só um nome de upgrade — é o que ele É.**

Incontáveis heróis, deuses e monstros caíram na Fenda. Seus poderes **não somem**: ecoam. Pegar um **Eco** ([03b](03b_Reward_System.md), [40](40_Eco_Pool_Catalog.md)) = **ligar a si o eco de um caído** por uma run. Some quando você cai (volta à Fenda).

As **4 famílias** = ecos de **4 entidades caídas**:

| Família | Entidade caída (lore) | Fantasia mecânica |
|---|---|---|
| **⚡ Tempestade** | **o Trovejante Acorrentado** — um deus do céu aprisionado na Fenda | combo aéreo, juggle, raio ([40](40_Eco_Pool_Catalog.md)) |
| **🩸 Carniça** | **a Mãe Carniça** — santa da peste que se recusou a morrer | sustain, lifesteal, cura no ar |
| **🔥 Fúria** | **o Campeão Quebrado** — herói condenado a arder sem fim | dano, risco, dash ofensivo |
| **🌑 Vazio** | **o Que Sussurra** — algo da própria Fenda, que não deveria existir | controle, poise, void |

> 🪞 **Meta-progressão = lembrar os caídos.** Desbloquear uma família/Eco no hub ([40 §6](40_Eco_Pool_Catalog.md)) é, na ficção, **despertar/lembrar** aquela entidade — o que também revela lore dela. Poder e história destravam **juntos**.

---

## 5. Essência (a moeda meta, em ficção)

- A Fenda **sangra** Essência — substância bruta de memória/alma condensada.
- Você **colhe** (matando, limpando salas — [42 §8](../systems/42_Run_Room_Manager.md)) e **carrega** de volta pelo Limiar.
- Gasta no hub pra **fortalecer-se** e **despertar Ecos/memórias** (§4, [03b §7](03b_Reward_System.md)).

> Essência atravessa porque **você** a carrega através do Limiar — por isso ela persiste e os Ecos não.

---

## 6. O Limiar (o hub) & quem vive nele

O **Limiar** é o lugar liminar entre o mundo e a Fenda — pra onde você volta ao cair. NPCs (interação cresce com a narrativa — pós-MVP):

| NPC | Papel mecânico | Papel narrativo |
|---|---|---|
| **A Guardiã do Limiar** | meta-upgrades ("Espelho" do DDR, [29 §5](../ui/29_Run_Flow_Menus.md)) | guarda o Limiar; sabe mais do que diz |
| **O Forjador de Ecos** | desbloqueio/despertar de Ecos ([40 §6](40_Eco_Pool_Catalog.md)) | liga os ecos dos caídos a você |
| **A Voz da Fenda** | comenta Pactos ([48](48_Pact_Heat.md)) e dispara o drip narrativo / dilemas ([49](49_Codex_Limiar.md)) | sussurra do fundo; te chama mais fundo (antagonista?) |
| *(pós-MVP)* um eco senciente | sidequest/codex | um caído que virou consciente; espelha seu destino |

> 🗣️ **Relações estilo Hades:** os NPCs **reagem** à sua progressão e mortes. É o que faz você voltar pro Limiar **querendo** falar com eles — cola social do loop (ver [46 §narrativa](46_Depth_Replayability.md)).

---

## 7. O Coração da Fenda (objetivo & antagonista)

- No fundo está o **Coração da Fenda** — a origem da ferida. O que é: revelação do jogo (um deus caído? o primeiro Andarilho? **o seu próprio eco**?).
- **Mini-boss** ([34](../enemies/34_MiniBoss.md)) = um **Guardião** de camada — um caído poderoso barrando o caminho.
- O Coração é o "final" que dá sentido à descida repetida (e à reviravolta narrativa pós-MVP).

---

## 8. 🪂 Por que você comba no AR (o pilar é diegético!)

> O combo aéreo — o **pilar** ([01 §4](01_Game_Vision.md), [16](../combat/16_Aerial_Combos.md)) — tem **razão na ficção.**

Dentro da Fenda, **gravidade e física estão quebradas** (é uma ferida na realidade). O Andarilho **aprendeu a usar isso como arma**: arremessa inimigos no **ar partido** da Fenda — onde a gravidade mal segura — e os comba lá em cima, onde eles não deveriam estar.

> 🎯 O **launcher** ([16 §2](../combat/16_Aerial_Combos.md)) não é "só um uppercut" — é você **rasgando a gravidade local**. O combo aéreo é a Fenda quebrada feita movimento. Isso dá **identidade** ao pilar que nenhum hack'n'slash topdown tem.

---

## 9. Progressão narrativa (o drip que vicia — estilo Hades)

A genialidade do Hades: **a história avança a cada run/morte**. Aplicado ao DDR:

- Cada descida revela **fragmentos** (sussurros da Voz, mudanças no Limiar, codex).
- NPCs **comentam** suas mortes/profundidade ("caiu na 3ª de novo?", "você foi mais fundo dessa vez").
- Essência destrava **memória** (codex de lore dos caídos), não só poder.
- Vencer o boss N vezes **avança a verdade** sobre o Coração.

> 🪝 É a **cola de retenção**: você não volta só pelo combate — volta pra **ver o que acontece**. Detalhe de implementação no [46 §6](46_Depth_Replayability.md).

---

## 10. O que semear no MVP (barato) vs pós-MVP

| Elemento | MVP (barato) | Pós-MVP |
|---|---|---|
| Moldura (Fenda/Ecos=ecos/Limiar/morte volta) | 🟢 nomes + 2-3 frases de tom | — |
| Nomes/identidade das 4 famílias caídas | 🟢 já no [40](40_Eco_Pool_Catalog.md) | — |
| Hub funcional (Guardiã = meta-upgrades) | 🟡 1 NPC funcional | relações/diálogo |
| Voz da Fenda (1-2 linhas no boot/morte) | 🟡 texto barato, alto charme | VO, arco |
| Codex de lore (memórias dos caídos) | 🔵 | P2 |
| Cutscenes, VO, reviravolta do Coração | 🔵 | P2+ |

> 🎯 **No MVP, lore = texto barato + nomes com intenção.** A moldura custa quase nada e faz o jogo *parecer* ter alma. A narrativa cheia é a expansão que segura o jogador a longo prazo.

---

## 11. Próximo passo

→ [46 — Profundidade & Replay](46_Depth_Replayability.md) (como a narrativa vira retenção + auditoria da run) · [03b — Ecos](03b_Reward_System.md) · [01 — Visão](01_Game_Vision.md).
