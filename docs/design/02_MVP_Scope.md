# 02 — Escopo do MVP

> Objetivo do MVP: **uma run jogável e divertida** — entrar na rift, lutar em algumas salas com combo de chão + aéreo, morrer/vencer, repetir. Tudo o mais é depois.

---

## 1. Definição de "MVP pronto"

O MVP está pronto quando um estranho consegue, **sem você explicar**:

1. Mover e atacar com resposta imediata.
2. Encadear um combo de chão e **lançar + combar no ar** pelo menos um inimigo.
3. Limpar uma sala, escolher uma recompensa, e ir pra próxima.
4. Morrer, ver "você ficou X mais forte", e querer tentar de novo.

Se esses 4 acontecem e é **gostoso**, o MVP cumpriu seu papel. 🎯

---

## 2. Escopo — dentro vs fora

### ✅ Dentro do MVP (faça)

| Área | Escopo mínimo |
|---|---|
| **Personagem** | 1 player jogável, movimento responsivo (run + sprint + dash). |
| **Combate chão** | 1 arma, combo de 3-4 golpes, hit detection, hit-stop. |
| **Combate aéreo** | 1 launcher + 2-3 ataques aéreos + 1 slam. **(pilar do jogo)** |
| **Inimigos** | 2 tipos: 1 *melee* simples + 1 *ranged*/telegrafado. IA básica (perseguir, atacar, recuar). |
| **Sala** | Arena fechada, ondas de inimigos, porta abre ao limpar. |
| **Run** | 3-5 salas + 1 mini-boss no fim. |
| **Recompensa** | Escolher 1 de 2-3 upgrades entre salas (dano, vida, nova habilidade). |
| **Meta** | 1 recurso persistente que dá upgrade permanente leve (prova do loop). |
| **GAS** | Attributes (HP, dano, stamina), abilities (ataque, dash, launcher, slam). |
| **Câmera/Input** | Topdown jogável + Enhanced Input + buffer de combo. |

### ❌ Fora do MVP (resista)

- ❌ Múltiplas armas/classes (1 já prova o combate).
- ❌ Geração procedural sofisticada (use salas montadas à mão + ordem aleatória).
- ❌ Foot IK, turn-in-place polido, stride warping perfeito (topdown esconde — ver locomoção).
- ❌ Narrativa, vozes, cutscenes.
- ❌ Menu/UI bonito (use UI funcional feia).
- ❌ Multiplayer (arquitetura GAS deixa aberto, mas não implemente).
- ❌ Save/load completo (só o recurso meta persistente).
- ❌ Áudio mixado (1 som por ação já dá feedback).

> 🧪 **Regra de ouro:** se um recurso não muda a resposta à pergunta "isso é divertido de jogar?", ele é pós-MVP.

---

## 3. Milestones (cada um termina JOGÁVEL)

Nenhum milestone é "só backend". Cada um produz algo que dá pra **pegar o controle e sentir**.

| M | Nome | Entrega jogável | Docs-chave |
|---|---|---|---|
| **M⁻¹** ⚡ | **Spike** | *Descartável:* cubo lança cubo + sombra + 1 hit. Valida fun/legibilidade do pilar **antes** de construir infra. | [17 §1.5](../17_Implementation_Roadmap.md), 16 |
| **M0** | **Esqueleto** | Personagem topdown anda/sprinta/dasha numa sala vazia, câmera boa. | 04, 06, 07, 08, 09 |
| **M1** | **Bater** | Combo de chão de 3 golpes com hit-stop num boneco. GAS ligado. | 05, 15 |
| **M2** | **Voar** | Launcher + combo aéreo + slam num inimigo. **(o pilar)** | 13, 16 |
| **M3** | **Lutar** | 2 inimigos com IA básica numa arena com ondas. | (IA — fora do escopo desta doc, mas plugável no GAS) |
| **M4** | **Run** | 3-5 salas encadeadas + recompensa entre salas + mini-boss. | 03 |
| **M5** | **Loop** | Morte → meta-upgrade → nova run. Tunning de feel. | 03, 17 |

> ✅ **Depois do M2 você já sabe se o jogo é divertido.** Priorize chegar no M2 rápido — é onde mora a alma do projeto.

---

## 4. Anti-objetivos (o que vai te atrasar)

| Armadilha | Por que mata o MVP | Faça em vez disso |
|---|---|---|
| "Vou deixar a animação perfeita" | Topdown esconde; buraco sem fundo. | Animação *legível*, não perfeita. P0 = responder rápido. |
| "Preciso de geração procedural primeiro" | Semanas sem combate testável. | Salas à mão + ordem aleatória. Procedural é pós-MVP. |
| "Vou fazer 5 armas" | 5× o trabalho, 0× mais aprendizado. | 1 arma, combate profundo. |
| "Motion Matching desde o início" | Curva de aprendizado trava o M0. | Bespoke state machine (doc 08). MM é spike opcional. |
| "Sistema de save robusto" | Engenharia sem payoff de fun. | Só persistir o recurso meta. |

---

## 5. Orçamento de esforço sugerido (proporção, não prazo)

```
Combate (chão + aéreo) ████████████████  40%   ← o pilar, é onde o fun mora
Sistemas base (GAS/cam/input) ██████████  25%
Run/roguelike loop      ███████           18%
Locomoção (movimento)   █████             12%   ← só o P0; resto é polish
Inimigos/IA             ██                 5%   ← básico no MVP
```

> A locomoção parece grande na sua lista (12 recursos), mas em **topdown** ela é **suporte**, não protagonista. O protagonista é o combate. Aloque seu tempo onde o jogador olha.

---

## 6. Próximo passo

→ [03 — Core Loop Roguelike](03_Core_Loop_Roguelike.md): a estrutura da run que amarra tudo.
