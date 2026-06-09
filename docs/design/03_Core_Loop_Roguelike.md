# 03 — Core Loop Roguelike

> Como uma *run* se estrutura, o que o jogador faz em loop, e como a morte vira progresso. Mantido **mínimo e jogável** para o MVP.

---

## 1. Os dois loops

Todo roguelike tem dois loops aninhados:

```
┌─ LOOP DA RUN (minutos) ─────────────────────────────┐
│  Sala → Combate → Limpar → Recompensa → Próxima sala │
│                          ↓ (morte ou vitória)        │
└──────────────────────────┬───────────────────────────┘
                           ↓
┌─ META-LOOP (entre runs) ─────────────────────────────┐
│  Gastar recurso persistente → Upgrade permanente →   │
│  Próxima run começa "um pouco mais forte"            │
└──────────────────────────────────────────────────────┘
```

- **Loop da run** = o jogo minuto-a-minuto. **É onde o combate vive.** Prioridade máxima.
- **Meta-loop** = o "vício" de longo prazo. No MVP, **mínimo viável** (1 recurso, poucos upgrades).

---

## 2. Estrutura de uma run (MVP)

```
[Hub/Entrada] → [Sala 1] → [Sala 2] → [Sala 3] → [Sala 4] → [Mini-Boss] → [Fim da run]
                   │          │          │          │
                (recompensa entre salas: escolher 1 de N)
```

| Elemento | MVP | Pós-MVP |
|---|---|---|
| Nº de salas | 3-5 fixo | Variável por "andar" |
| Layout | Arenas montadas à mão, **ordem embaralhada** | Geração procedural |
| Fim | 1 mini-boss | Múltiplos bosses, andares |
| Tipos de sala | Combate | Combate, elite, tesouro, evento, loja |

> 🧩 **Procedural sem procedural:** monte 6-8 arenas à mão e sorteie 4 por run. O jogador sente variação **sem** você escrever um gerador. Procedural de verdade é pós-MVP.

---

## 3. Anatomia de uma sala de combate

```
1. Player entra → portas trancam.
2. Spawn de onda(s) de inimigos.
3. Player limpa (aqui mora o combate — combos, aéreo, dash).
4. Última onda morre → portas abrem + recompensa aparece.
5. Player escolhe recompensa → avança.
```

**Variáveis de tuning da sala** (exponha como propriedades, não hardcode):
- Nº de ondas, tamanho da onda, mix de inimigos.
- Tempo entre ondas (respiro pra reposicionar).
- Tipo de recompensa garantido vs aleatório.

---

## 4. Recompensas (o que o jogador escolhe)

Entre salas, oferecer **1 de 2-3** opções (escolha = engajamento). Categorias MVP:

| Categoria | Exemplo | Efeito (via GAS — doc 05) |
|---|---|---|
| **Stat** | "+15% dano", "+1 vida máx" | `GameplayEffect` permanente na run |
| **Habilidade** | "Dash agora causa dano", "3º golpe lança" | Concede/modifica `GameplayAbility` |
| **Modificador** | "Combos aéreos curam 5 HP" | `GameplayEffect` condicional + tag |

> 🎰 As escolhas constroem a **build da run**. ⚠️ **Mas "combinações" só viram rejogabilidade se houver SINERGIA** — e o pool precisa de tamanho. A [Revisão de Design 2026-06](Design_Review_2026-06.md) elevou a recompensa a sistema de 1ª classe: veja **[03b — Sistema de Recompensa / Ecos](03b_Reward_System.md)** para pool-alvo (15-20), famílias, sinergias, raridade e a regra de ouro *"todo Eco muda como você joga, não só um número"*.

---

## 5. Morte & meta-progressão (MVP mínimo)

A morte **não pode ser só "game over"** — tem que dar progresso (Pilar 3).

**MVP mínimo viável:**
1. Ao morrer/vencer, converter desempenho em **1 recurso persistente** (ex.: "Essência" = inimigos mortos / salas limpas).
2. No hub, gastar Essência em **3-5 upgrades permanentes leves** (ex.: +5% vida inicial, +1 slot de upgrade, desbloquear 1 habilidade inicial).
3. Próxima run começa marginalmente mais forte.

Isso já prova o meta-loop. Árvores grandes de upgrade são pós-MVP.

> 🪞 **Destrave ESCOLHAS, não percentuais (revisão de design — Mara).** "+5% vida inicial" é progressão *invisível* — não dá dopamina. Prefira meta-upgrades que **mudam a próxima run**: destravar um Eco/família novos no pool, uma habilidade inicial, ou +1 slot de escolha. O Espelho do Hades destrava *mecânicas*, não números. Detalhe em [03b §7](03b_Reward_System.md).

```
Run termina → ganha Essência → hub → compra upgrade permanente → nova run mais forte
```

---

## 6. Economia de números (ponto de partida, ajuste no playtest)

| Variável | Valor inicial | Nota |
|---|---|---|
| Duração de uma run | 5-10 min | Curta o bastante p/ "só mais uma". |
| Salas por run | 4 + boss | |
| Inimigos por sala | 4-8 (1-2 ondas) | Densidade dá power fantasy sem caos. |
| Upgrades por run | 4-5 escolhas | Suficiente p/ sentir build. |
| Essência por run | ~normalizar p/ 3-5 runs por upgrade meta | Progresso sentido, não instantâneo. |

> 📐 Estes números são **chutes iniciais**. A verdade vem do playtest do M4/M5. Exponha tudo como propriedade editável.

---

## 7. Como isto conecta com os outros sistemas

| Sistema | Papel no loop | Doc |
|---|---|---|
| **GAS** | Aplica upgrades (effects/abilities), gerencia HP/stamina/morte | [05](../systems/05_GAS_Architecture.md) |
| **Combate** | O verbo central de cada sala | [15](../combat/15_Combat_Overview.md), [16](../combat/16_Aerial_Combos.md) |
| **Locomoção** | Reposicionar entre inimigos (dash/sprint) | [08](../locomotion/08_Locomotion_Overview.md) |
| **Câmera** | Enquadrar a arena, foco em alvo | [06](../systems/06_Camera_TopDown.md) |

---

## 8. Referências de estudo (roubar o que funciona)

Dois jogos resolvem **exatamente** este loop. Estude-os antes de inventar:

| Referência | O que copiar pro nosso loop |
|---|---|
| **[Hades — Wiki](https://hades.fandom.com/wiki/Hades_Wiki)** | **Boons (Hades)** → nossos **Ecos** (§4): escolher 1 de N upgrades que constroem a build. Estude também o ritmo câmara→câmara, os tipos de sala (combate/tesouro/elite) e a meta-progressão (Espelho/Néctar) → nosso meta-loop (§5). |
| **[Death Must Die — Steam](https://store.steampowered.com/app/2334730/Death_Must_Die/)** | **Builds por "Deuses"** (escolhe um deus → bênçãos temáticas) = referência direta pros nossos **Ecos** e famílias. Estude a leitura de **horda densa em topdown**, o dodge, e a progressão **run-a-run de itens** → nosso recurso meta (§5). É o gênero quase exato. |
| **[Diablo 4](https://diablo4.blizzard.com/)** | Ritmo sala→sala em masmorra, **telegrafe de perigo no chão** (Pilar 4), dodge como resposta ao overwhelm, elites que forçam reposicionamento. Use como benchmark de **justiça visual** em horda — não copie o loop de loot/endgame. |
| **[Path of Exile 2](https://pathofexile2.com/)** | Combate topdown mais **manual e legível**; skills com identidade visual forte de cima; profundidade de build por **camadas/modificadores** → inspira famílias e sinergias de Ecos ([03b](03b_Reward_System.md)). Não copie a complexidade de economia/crafting. |

> 🎯 **Atalho de design:** os **Ecos** seguem a mesma lógica dos Boons do Hades / Deuses do Death Must Die — expressos em [GameplayEffects + Abilities (doc 05)](../systems/05_GAS_Architecture.md), com **leitura de combate** calibrada contra Diablo 4 / POE2. Não reinvente — adapte o que já provou viciar.

---

## 9. Próximo passo

→ Comece a **construir**: [04 — Setup do Projeto](../systems/04_Project_Setup.md). Ou veja a ordem completa em [17 — Roadmap](../17_Implementation_Roadmap.md).
