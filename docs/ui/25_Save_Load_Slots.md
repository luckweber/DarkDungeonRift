# 25 — Save / Load & Slots de Save · 🟡 P1

> Sistema de persistência: **o que** se salva, **como** (UE `USaveGame`), e a **UI de slots**. Pré-req: [23 — UI Overview](23_UI_Overview.md), [03 — Core Loop](../design/03_Core_Loop_Roguelike.md), [05 — GAS](../systems/05_GAS_Architecture.md).

> 🎲 **Verdade roguelike (importante):** num roguelike, **a run NÃO é salva como um RPG** — ela é efêmera (você morre, recomeça). O que **persiste** é a **meta-progressão**. Por isso o "save" do DDR é diferente de um save de RPG.

---

## 1. O que se salva (e o que NÃO se salva)

| Dado | Persiste? | Onde |
|---|---|---|
| **Meta: Essência** (recurso) ([03 §5](../design/03_Core_Loop_Roguelike.md)) | ✅ Sim | SaveGame |
| **Meta: upgrades permanentes** comprados | ✅ Sim | SaveGame |
| **Meta: desbloqueios** (Ecos/habilidades/famílias) | ✅ Sim | SaveGame |
| **Settings** (volume, qualidade) | ✅ Sim | SaveGame separado ([27](27_Settings.md)) ou GameUserSettings |
| **Estatísticas** (runs, kills, melhor combo) | ✅ Sim | SaveGame |
| **Build da run atual** (Ecos pegos) | ⚠️ Só se houver **run-resume** (§4) | SaveGame de run (P1) |
| **Estado momento-a-momento** (HP, posição) | ❌ Não no MVP | — |

> 🎯 **MVP ([03 §5](../design/03_Core_Loop_Roguelike.md), [02](../design/02_MVP_Scope.md)):** persistir **só a meta** (Essência + upgrades + desbloqueios). Salvar a run no meio (resume) é **P1** (Hades faz; é ótimo, mas não é o core).

---

## 2. Estrutura do SaveGame

```cpp
UCLASS()
class UDDRSaveGame : public USaveGame {
  // versionamento (migração futura — NÃO esquecer):
  UPROPERTY() int32 SaveVersion = 1;

  // meta-progressão:
  UPROPERTY() int32 Essencia = 0;
  UPROPERTY() TArray<FGameplayTag> UpgradesComprados;   // ex.: Meta.Upgrade.Health
  UPROPERTY() TArray<FGameplayTag> Desbloqueios;        // ex.: Eco.Family.Storm

  // estatísticas:
  UPROPERTY() int32 RunsJogadas = 0;
  UPROPERTY() int32 MelhorCombo = 0;

  // (P1) run em andamento:
  UPROPERTY() FDDRRunState RunSalva;   // Ecos, sala atual, seed
  UPROPERTY() bool bTemRunAtiva = false;
};
```

> 🔢 **`SaveVersion` é obrigatório.** Quando você mudar a struct (e vai mudar), o load precisa migrar saves antigos sem crashar. Sem versão, todo update quebra os saves do jogador.

> 🗂️ **Dois saves IRMÃOS:** este `UDDRSaveGame` (perfil/meta) **não** guarda configurações. Volume/vídeo/idioma/acessibilidade vivem num **`UDDRSettingsSave`** separado ([27 §3](27_Settings.md), [38](../systems/38_Localization.md)) — vídeo via `GameUserSettings`. Mantenha os dois separados (settings não devem morrer com o perfil).

---

## 3. API de Save/Load

```cpp
// Salvar (assíncrono — não trave o frame):
UGameplayStatics::AsyncSaveGameToSlot(SaveObj, "DDR_Profile_0", 0, OnSaved);
// Carregar:
UGameplayStatics::AsyncLoadGameFromSlot("DDR_Profile_0", 0, OnLoaded);
// Existe?
UGameplayStatics::DoesSaveGameExist("DDR_Profile_0", 0);   // → "Continuar" (doc 24)
```

**Quando salvar (autosave — roguelike não tem "save manual" no meio do combate):**
- Ao **comprar upgrade meta** no Hub.
- Ao **terminar uma run** (morte ou vitória) — grava Essência + stats.
- (P1) Ao **entrar numa sala nova** — grava `RunState` pra resume.

> 🔒 **Single-player + autosave** = simples. Sem conflito de sessão, sem cloud-merge. Um autosave bem posicionado > save manual.

---

## 4. Slots: quantos?

A pergunta "slots saves". Duas filosofias:

| Modelo | Como | Quando |
|---|---|---|
| **A — Perfil único (autosave)** | 1 slot, salva sozinho | 🟢 **Recomendado MVP** — roguelike é 1 progressão contínua (Hades, DMD) |
| **B — Múltiplos slots/perfis** | 3 slots, jogador escolhe | 🟡 P1+ — se quiser várias "contas" na mesma máquina |

**Decisão MVP: Modelo A (perfil único + autosave).** Roguelikes raramente precisam de múltiplos slots — a graça é a *única* jornada de meta-progressão. Se quiser slots depois, a UI abaixo já cobre.

### Run-resume (P1)
Se salvar a run (`bTemRunAtiva`), o "Continuar" do main menu retoma a run **do início da sala atual** (não do frame exato — re-spawna a sala pela seed). Hades-style: você pode largar e voltar.

---

## 5. UI de slots (se Modelo B / ou tela de perfil)

```
┌──────────────────────────────────────────┐
│  PERFIS                                   │
│  ┌────────────────────────────────────┐  │
│  │ Perfil 1  · Essência 340 · 12 runs │▶ │  ← foco
│  │ melhor combo: S · 3h12             │  │
│  └────────────────────────────────────┘  │
│  ┌────────────────────────────────────┐  │
│  │ Perfil 2  · [vazio — Nova]         │  │
│  └────────────────────────────────────┘  │
│   [A] Selecionar   [X] Apagar   [B] Volta │
└──────────────────────────────────────────┘
```

- Cada slot mostra **resumo** (Essência, runs, playtime, melhor combo).
- **Apagar** sempre com modal de confirmação.
- No Modelo A (perfil único), esta tela **não existe** — o main menu carrega o perfil direto.

---

## 6. MVP vs depois

| Item | Prioridade |
|---|---|
| `UDDRSaveGame` (meta) + `SaveVersion` | 🟢 P0 (do meta-loop) |
| Autosave (fim de run + compra meta) | 🟢 P0 |
| `DoesSaveGameExist` → "Continuar" | 🟡 P1 |
| Run-resume (`RunState`) | 🟡 P1 |
| UI de múltiplos slots | 🔵 P2 |
| Migração por `SaveVersion` | 🟡 P1 (antes do 1º update) |

---

## 7. Checklist

- [ ] `UDDRSaveGame` com `SaveVersion` + campos meta
- [ ] Async save/load (não síncrono)
- [ ] Autosave no fim de run e na compra de upgrade meta
- [ ] `DoesSaveGameExist` alimenta o "Continuar" ([24](24_MainMenu.md))
- [ ] Modelo A (perfil único) no MVP
- [ ] (P1) run-resume re-spawna a sala pela seed, não o frame exato
- [ ] Apagar perfil com confirmação

---

## 8. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Save some após update | Sem `SaveVersion`/migração | §2 — versionar + migrar |
| Hitch ao salvar | Save síncrono | §3 — `AsyncSaveGameToSlot` |
| "Continuar" sempre ativo | Não checa `DoesSaveGameExist` | §3 |
| Run-resume teleporta/buga | Tentou salvar estado exato | §4 — re-spawna sala pela seed |
| Meta não persiste | Não salvou no momento certo | §3 — autosave na compra/fim de run |

---

## 9. Próximo passo

→ [26 — Loading System](26_Loading_System.md) (o load entre telas) · [29 §5 — Hub](29_Run_Flow_Menus.md) (onde se gasta Essência).
