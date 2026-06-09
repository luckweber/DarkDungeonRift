# 49 — Codex & Vozes do Limiar (drip narrativo) · 🟡 P1 (mínimo) / 🔵 P2 (cheio)

> Expande a [Lore (45)](45_World_Lore.md) e materializa a mecânica **H** do [46 — Profundidade](46_Depth_Replayability.md): a **história que avança a cada run/morte** (estilo Hades). Inclui **conteúdo de exemplo** pra fixar o tom. Texto via [`FText` (38)](../systems/38_Localization.md).

> 🪝 **Por que importa pro vício:** o jogador não volta só pra bater — volta **pra ver o que acontece**. É a cola de retenção mais barata que existe (texto). No MVP, o mínimo (algumas falas + codex stub) já dá alma.

---

## 1. Os 3 canais de drip

| Canal | O que é | Gatilho | Custo |
|---|---|---|---|
| **A Voz da Fenda** | sussurros do fundo (1-2 linhas) | boot, morte, profundidade, Pacto | 🟢 baixíssimo |
| **Reações de NPC** | Guardiã/Forjador comentam seu progresso | volta ao Limiar | 🟢 baixo |
| **Codex** | memórias dos caídos / da Fenda | Essência / eventos / mortes | 🟡 médio |

> Tudo é **FText em DataTable** ([38](../systems/38_Localization.md), [44](../systems/44_Data_Driven.md)) com flags de gatilho. Zero código de "engine narrativa".

---

## 2. 🗣️ A Voz da Fenda (exemplos de tom)

Linhas curtas, melancólicas, com a *atração do fundo*. Disparadas por evento:

| Gatilho | Exemplo de fala |
|---|---|
| 1º boot | *"Outro que desce. Outro que vai ficar."* |
| Morte (cedo) | *"Tão perto da superfície ainda. A Fenda mal te conhece."* |
| Morte (fundo) | *"Você cai. Você volta. Já entendeu que isso é o caminho, não o fim?"* |
| Nova profundidade | *"Ninguém veio tão fundo desde... ele. Você se parece com ele."* |
| Pegou Lendário | *"Esse poder não é seu. Você só o lembra por um instante."* |
| Pacto alto ([48](48_Pact_Heat.md)) | *"Sim. Peça mais. Eu empurro de volta com prazer."* |
| Venceu o boss | *"Um Guardião a menos. O Coração te ouviu cair a porta."* |

> 🎯 **Curtas e ambíguas.** Insinuam (o "ele", o Coração, "ficar") sem explicar — a curiosidade é o anzol. Uma fala dessas no boot/morte custa nada e dá identidade imediata.

---

## 3. 📖 O Codex — memórias dos caídos

Entradas de lore destravadas por **Essência** (no Limiar) ou **eventos**. Dão razão às 4 famílias de Eco ([40](40_Eco_Pool_Catalog.md), [45 §4](45_World_Lore.md)):

| Entrada | Destrava com | Exemplo (resumo) |
|---|---|---|
| **O Trovejante Acorrentado** (⚡ Tempestade) | usar 10 Ecos Tempestade | um deus do céu que a Fenda prendeu; seu trovão ainda ecoa no ar partido — por isso você comba no alto |
| **A Mãe Carniça** (🩸) | curar X com Ecos Carniça | uma santa que se recusou a morrer; sua carniça alimenta quem sangra |
| **O Campeão Quebrado** (🔥 Fúria) | morrer com <10% vida | herói condenado a arder sem fim; quanto mais perto da morte, mais forte |
| **O Que Sussurra** (🌑 Vazio) | chegar ao boss | não é um caído — é a Fenda fingindo ser um. Cuidado com o que você lembra |
| **O primeiro Andarilho** | vencer o boss | quem desceu antes de você... e o que encontrou no Coração |

> 🪞 **Poder e história destravam juntos:** lembrar um caído (meta-unlock, [40 §6](40_Eco_Pool_Catalog.md)) revela sua entrada de codex. O jogador grinda Essência tanto por **poder** quanto por **verdade**.

---

## 4. 💬 Reações de NPC no Limiar

A Guardiã e o Forjador ([45 §6](45_World_Lore.md)) **comentam** seu progresso — é o que faz voltar ao Limiar *querendo* conversar:

| NPC | Gatilho | Exemplo |
|---|---|---|
| **Guardiã** | 3 mortes seguidas na mesma sala | *"A terceira câmara de novo. Ela aprende você mais rápido do que você a ela."* |
| **Guardiã** | nova profundidade | *"Mais fundo. Bom. Ou terrível. Aqui as duas coisas são iguais."* |
| **Forjador** | desbloqueou família nova | *"Você lembrou o Trovejante. Ele não vai agradecer — mas o poder é real."* |
| **Forjador** | 1º Lendário | *"Ecos assim queimam rápido. Use antes que a Fenda o reclame."* |

> Mesmo **3-4 reações por NPC** já criam a sensação de que o Limiar **responde a você**. É o oposto de um menu morto.

---

## 5. ❓ Eventos (sala de Evento, [47 §5](47_Room_Types_Routing.md))

Encontros com a Voz que oferecem **dilemas** — escolha com consequência:

```
A VOZ DA FENDA sussurra:
  "Há um eco aqui. Forte. Mas ele vai querer algo em troca."

  [Aceitar]  → Eco Lendário grátis, mas -25% vida máx até o fim da run
  [Recusar]  → +30 Essência (a Voz respeita a prudência)
  [Provocar] → luta de elite extra → recompensa dobrada (risco)
```

> Dilema = decisão memorável = run memorável. Texto barato; reusa o sistema de Eco/maldição via GAS.

---

## 6. A máquina de estados narrativa (death-positive)

A história avança com **flags** que mudam com runs/mortes/profundidade/boss:

```
NarrativeFlags (no UDDRSaveGame / NarrativeSubsystem):
  RunsPlayed, DeepestDepth, BossKills, FamiliesUnlocked, DeathsInRoom[]

A cada evento → checa DataTable de falas/codex elegíveis (por flag) → dispara a 1ª não-vista.
```

- **Morte AVANÇA a história** (Pilar 3, [45 §2](45_World_Lore.md)) — a Voz/NPCs reagem a quedas, não só a vitórias.
- Falas são **consumidas** (não repetem) → progressão sentida.
- Marcos grandes (1º boss kill, profundidade recorde) destravam **beats** maiores.

```cpp
UCLASS()
class UDDRNarrativeSubsystem : public UGameInstanceSubsystem {
public:
    void OnEvent(FGameplayTag EventTag);          // Event.Death, Event.NewDepth...
    FText GetNextLine(EDDRSpeaker Speaker, FGameplayTag Context);
    void UnlockCodex(FGameplayTag CodexId);
    bool IsCodexUnlocked(FGameplayTag CodexId) const;
    // flags persistidas no save (25)
};
```

---

## 7. Prioridade (mínimo no MVP, cheio depois)

| Item | Prioridade |
|---|---|
| Voz da Fenda: ~6 falas (boot/morte/profundidade) | 🟡 P1 (charme barato) |
| 2-3 reações por NPC no Limiar | 🟡 P1 |
| Codex: 5 entradas (4 famílias + boss) | 🟡 P1 (stub) → 🔵 cheio |
| `UDDRNarrativeSubsystem` + flags | 🟡 P1 |
| Eventos (dilemas) | 🔵 P2 |
| Arco completo / reviravolta do Coração | 🔵 P2+ |
| VO (vozes gravadas) | 🔵 P2+ |

> 🎯 **MVP = ~6 falas da Voz + 5 entradas de codex + flags.** Custa uma tarde de escrita e faz o jogo *parecer* ter alma e mistério. O arco completo é a expansão de retenção.

---

## 8. Checklist

- [ ] `UDDRNarrativeSubsystem` + flags no save ([25](../ui/25_Save_Load_Slots.md))
- [ ] `DT_VozDaFenda` (falas + gatilho + flag de visto)
- [ ] ~6 falas da Voz (boot/morte/profundidade/lendário/pacto/boss)
- [ ] 2-3 reações por NPC (Guardiã/Forjador)
- [ ] `DT_Codex` (5 entradas: 4 famílias + boss) em `FText` ([38](../systems/38_Localization.md))
- [ ] Codex destrava por Essência/evento ([40 §6](40_Eco_Pool_Catalog.md))
- [ ] Morte dispara avanço narrativo (death-positive)
- [ ] Falas consumidas não repetem

---

## 9. Próximo passo

→ [45 — Lore](45_World_Lore.md) (a moldura) · [46 — Profundidade](46_Depth_Replayability.md) · [47 — Eventos/Salas](47_Room_Types_Routing.md) · [40 — Ecos/desbloqueios](40_Eco_Pool_Catalog.md).
