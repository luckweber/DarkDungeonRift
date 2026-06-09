# 38 — Localização & Múltiplos Idiomas · 🟡 P1

> Suporte a **vários idiomas** no Dark Dungeon Rift: arquitetura UE5 (`FText`, String Tables, Localization Dashboard), quais textos localizar, troca de idioma em runtime (Settings + Main Menu), e convenções para não refazer UI depois. Pré-req: [23 — UI Overview](../ui/23_UI_Overview.md), [27 — Settings](../ui/27_Settings.md), [24 — Main Menu](../ui/24_MainMenu.md).

> 🧭 **Princípio:** localize **desde o primeiro widget** — trocar `FString` por `FText` depois em 200 telas é caro. No MVP a doc é PT-BR, mas o **código** já nasce pronto pra EN/ES/etc.

> 🎮 **Single-player:** sem servidor de strings; tudo em pacotes locais (`.locres`).

---

## 1. `FString` vs `FText` — a regra de ouro

| Tipo | Uso | Localizável? |
|---|---|:---:|
| **`FText`** | Tudo que o **jogador lê** (UI, HUD, prompts, nomes de Eco) | ✅ |
| **`FString`** | IDs internos, paths, debug, logs, chaves de save | ❌ |
| **`FName`** | Tags GAS, identificadores de gameplay | ❌ |

```cpp
// ❌ ERRADO — não localiza
UTextBlock->SetText(FText::FromString("Nova Run"));

// ✅ CERTO — chave na String Table
UTextBlock->SetText(FText::FromStringTable(ST_DDR_UI, "MainMenu_NewRun"));
// ou no UMG: marque "Is Variable" e use FText no Bind
```

> ⚠️ **`FText::FromString("literal")` em runtime NÃO localiza** — só embrulha string fixa. Use String Table, `LOCTEXT`, ou `NSLOCTEXT`.

---

## 2. Arquitetura de pastas & namespaces

```
Content/
  Localization/
    Game/                    ← target principal
      en/Game.en.po
      pt-BR/Game.pt-BR.po
      es/Game.es.po
  UI/
    StringTables/
      ST_DDR_UI.uasset       ← menus, HUD, settings
      ST_DDR_Combat.uasset   ← nomes de abilities, Ecos
      ST_DDR_Items.uasset    ← itens, recompensas (futuro)
```

| Namespace / String Table | Conteúdo | Exemplos de chave |
|---|---|---|
| **`ST_DDR_UI`** | menus, HUD, settings, loading | `MainMenu_NewRun`, `Settings_Audio_Master`, `HUD_HP` |
| **`ST_DDR_Combat`** | abilities, Ecos, status | `Ability_Dash`, `Eco_Fireball_Name`, `Eco_Fireball_Desc` |
| **`ST_DDR_World`** | salas, inimigos (display name) | `Enemy_Grunt`, `Room_Combat_01` |
| **`ST_DDR_System`** | erros, confirmações | `Confirm_AbandonRun`, `Error_SaveFailed` |

> 🔑 **Chaves em inglês, valores traduzidos.** `MainMenu_NewRun` → EN: "New Run", PT: "Nova Run", ES: "Nueva Partida".

---

## 3. Como criar textos localizáveis

### 3.1 — C++ (`LOCTEXT`)

```cpp
// DDRLocalization.h — define o namespace uma vez
#define LOCTEXT_NAMESPACE "DDR"

// Em qualquer .cpp
FText Label = LOCTEXT("Settings_Language", "Language");
#undef LOCTEXT_NAMESPACE
```

O **Localization Gathering** exporta `LOCTEXT` automaticamente pro `.po`.

### 3.2 — UMG / Blueprint

- Textos estáticos no designer: marque **"Text is Culture Invariant" = false** (padrão).
- Use **String Table Reference** no campo Text quando possível (melhor pra tabelas grandes).
- Bindings dinâmicos: propriedade `FText`, nunca `FString`.

### 3.3 — DataAssets (Ecos, inimigos)

```cpp
UCLASS()
class UDDREcoData : public UPrimaryDataAsset
{
    UPROPERTY(EditDefaultsOnly, meta=(DisplayName="Name"))
    FText DisplayName;          // localizável no editor

    UPROPERTY(EditDefaultsOnly, meta=(DisplayName="Description"))
    FText Description;
};
```

No editor, cada campo `FText` ganha aba de tradução quando o pacote de idioma está ativo.

---

## 4. Idiomas alvo (MVP → lançamento)

| Cultura | Código UE | Prioridade | Notas |
|---|---|---|---|
| **Português (BR)** | `pt-BR` | 🟢 P0 (padrão) | Idioma da doc e do time |
| **English** | `en` | 🟡 P1 | Mercado principal Steam |
| **Español** | `es` | 🟡 P1 | LATAM + Espanha |
| Francês | `fr` | 🔵 P2 | |
| Alemão | `de` | 🔵 P2 | |
| Japonês | `ja` | 🔵 P2 | Fonte dedicada (§8) |

> 🎯 **MVP:** suporte técnico completo + **PT-BR + EN**. Traduza só o que aparece na tela (UI + Ecos do MVP). Texto placeholder em EN até ter tradutor.

---

## 5. Troca de idioma em runtime

### 5.1 — `UDDRLocalizationSubsystem` (GameInstanceSubsystem)

```cpp
UCLASS()
class UDDRLocalizationSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void SetLanguage(const FString& CultureCode);  // "pt-BR", "en", "es"
    FString GetCurrentLanguage() const;
    TArray<FString> GetAvailableLanguages() const;

    DECLARE_MULTICAST_DELEGATE(FOnLanguageChanged);
    FOnLanguageChanged OnLanguageChanged;
};
```

```cpp
void UDDRLocalizationSubsystem::SetLanguage(const FString& CultureCode)
{
    FInternationalization::Get().SetCurrentCulture(CultureCode);
    OnLanguageChanged.Broadcast();
    // Persiste em UDDRSettingsSave (doc 27)
}
```

### 5.2 — Onde o jogador troca

| Local | Doc | Comportamento |
|---|---|---|
| **Settings → Jogabilidade → Idioma** | [27 §1](../ui/27_Settings.md) | dropdown; aplica ao vivo |
| **Main Menu (rodapé)** | [24](../ui/24_MainMenu.md) | atalho rápido `[⚙ idioma]` |
| **Boot** | [27 §3](../ui/27_Settings.md) | lê `UDDRSettingsSave` e aplica antes da primeira tela |

> 🔗 Ao mudar idioma, **todos os widgets visíveis** precisam atualizar. Inscreva `UDDRUIManager` ([23 §5](../ui/23_UI_Overview.md)) no delegate `OnLanguageChanged` → `RefreshAllScreens()`.

---

## 6. Pipeline de tradução (workflow)

```
1. DEV escreve chaves (EN ou PT como source)
        ↓
2. Localization Dashboard → Gather Text (export .po)
        ↓
3. Tradutor edita .po (Poedit, Crowdin, Lokalise...)
        ↓
4. Import translations → Compile (gera .locres)
        ↓
5. Teste in-editor: Preview Language = es
        ↓
6. Package: Staged Localization Data incluído no build
```

### Configuração no projeto

1. **Edit → Project Settings → Localization** → crie **Localization Target** `Game`.
2. Adicione culturas: `en`, `pt-BR`, `es`.
3. Marque **String Tables** e **Gather from packages** (UMG, DataAssets).
4. **Compile** antes de cook.

> 📦 No build de shipping, cada idioma vira um arquivo `.locres` em `Content/Localization/Game/{culture}/`.

---

## 7. O que localizar (por milestone)

| Milestone | Textos | Prioridade |
|---|---|---|
| **M4-M5** (HUD + Ecos) | HP labels, nomes/descrições de 6-8 Ecos, death screen | 🟢 P0 técnico / 🟡 tradução |
| **Menus** | Main menu, pause, settings, confirmações | 🟡 P1 |
| **Combate** | Damage numbers não precisam (são números); ability names no HUD sim | 🟡 P1 |
| **Inimigos** | Display names (opcional no MVP — grunts sem nome na tela) | 🔵 P2 |
| **Tutoriais / lore** | Pós-MVP | 🔵 P2 |

> 🎯 **No MVP, foque em:** botões de menu, settings, Seleção de Eco (nome + descrição curta), game over. Isso já cobre 90% do que o jogador lê.

---

## 8. Fontes & layout (CJK e acentos)

| Idioma | Fonte | Notas |
|---|---|---|
| PT/EN/ES/FR/DE | Uma família com **acentos** (ex.: Noto Sans) | Tamanho base 14-16 no HUD |
| Japonês (P2) | Fonte CJK separada (Noto Sans JP) | Composite Font no UMG |
| Ícones | Culture-invariant | Não traduzir símbolos de input (use Enhanced Input icons) |

**Composite Font** no projeto UMG:

```
DefaultFont:
  • Noto Sans (Latin, Latin Extended)
  • Noto Sans JP (fallback CJK — P2)
```

> ⚠️ Alemão e espanhol têm palavras longas — teste botões com `Auto Wrap Text` e min-width generoso ([23 §4](../ui/23_UI_Overview.md) gamepad).

---

## 9. Texto dinâmico & pluralização

```cpp
// ❌ Concatenar strings — ordem das palavras muda por idioma
FString Msg = "You got " + FString::FromInt(N) + " Essence";

// ✅ FText::Format com argumentos nomeados
FText Msg = FText::Format(
    LOCTEXT("Reward_Essence", "You gained {Amount} {EssenceLabel}"),
    FFormatNamedArguments{
        { "Amount", FText::AsNumber(N) },
        { "EssenceLabel", LOCTEXT("Currency_Essence", "Essence") }
    });
```

| Caso | API |
|---|---|
| Números | `FText::AsNumber`, `FText::AsPercent` |
| Plural | `FText::Format` + regras por cultura (ou chaves separadas `Item_One` / `Item_Many` no MVP) |
| Nomes de variáveis em UI | `FText::Format` — nunca concatene |

---

## 10. Integração com Settings & Save

Persistência em **`UDDRSettingsSave`** ([27 §3](../ui/27_Settings.md)):

```cpp
UPROPERTY()
FString LanguageCulture = "pt-BR";  // default
```

No boot do `GameInstance`:

```cpp
auto* Loc = GetSubsystem<UDDRLocalizationSubsystem>();
auto* Save = LoadSettingsSave();
Loc->SetLanguage(Save->LanguageCulture);
```

Dropdown na Settings:

| Label (localizado) | Culture |
|---|---|
| Português (Brasil) | `pt-BR` |
| English | `en` |
| Español | `es` |

> 🔗 Labels do **próprio seletor de idioma** podem ser culture-invariant (cada opção no seu idioma nativo) — padrão Steam/OS.

---

## 11. Debug & QA

| Ferramenta | Uso |
|---|---|
| **Preview Language** (editor toolbar) | testa ES/EN sem trocar OS |
| `ddr.ShowLocalizationKeys 1` (cvar) | mostra chave em vez do valor — achar textos hardcoded |
| **Missing translation log** | Output Log ao compilar .locres incompleto |
| **Pseudolocalization** | ``«Ţëšţ»`` — detecta overflow de UI antes de traduzir |

Checklist de QA por idioma:

- [ ] Main menu navegável (gamepad)
- [ ] Settings aplica idioma sem restart (ou avisa se precisar)
- [ ] Nenhum `FText::FromString` literal em UI
- [ ] Seleção de Eco: nome + descrição traduzidos
- [ ] Textos não cortam em botões (DE/ES)

---

## 12. Prioridade MVP

| Item | Prioridade |
|---|---|
| `FText` em todo UMG + DataAssets de Eco | 🟢 P0 (técnico) |
| String Tables `ST_DDR_UI` + `ST_DDR_Combat` | 🟢 P0 |
| `UDDRLocalizationSubsystem` + save | 🟡 P1 |
| Dropdown idioma em Settings | 🟡 P1 |
| Tradução EN completa | 🟡 P1 |
| Tradução ES | 🟡 P1 |
| pt-BR como cultura padrão no `DefaultEngine.ini` | 🟢 P0 |
| Japonês / FR / DE | 🔵 P2 |
| Crowdin/Lokalise CI | 🔵 P2 |

---

## 13. Checklist

- [ ] Nenhum texto de jogador em `FString` / `FText::FromString` literal
- [ ] String Tables criadas (`ST_DDR_UI`, `ST_DDR_Combat`)
- [ ] Localization Target `Game` com `en`, `pt-BR`, `es`
- [ ] `UDDRLocalizationSubsystem` + persistência em `UDDRSettingsSave`
- [ ] Settings → Idioma ([27 §1](../ui/27_Settings.md))
- [ ] `OnLanguageChanged` → refresh de widgets abertos
- [ ] `FText` em `UDDREcoData` (nome + descrição)
- [ ] Composite Font com acentos (PT/ES)
- [ ] Gather + Compile testado; Preview Language no editor

---

## 14. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Texto não muda ao trocar idioma | Widget não refresha / FString | §5 — delegate + FText |
| Aparece chave em vez de texto | Tradução faltando no .locres | Compile + import .po |
| `LOCTEXT` não exporta | Namespace errado / não gather | §6 — Localization Dashboard |
| Botão corta texto em DE | Layout fixo narrow | §8 — wrap + min-width |
| Eco em inglês na build PT | DataAsset não gatherado | Incluir DataAssets no target |
| Precisa restart pra mudar | Cultura não aplicada em runtime | `SetCurrentCulture` (§5) |

---

## 15. Próximo passo

→ [27 — Settings](../ui/27_Settings.md) (dropdown idioma) · [24 — Main Menu](../ui/24_MainMenu.md) (atalho) · [03b — Ecos](../design/03b_Reward_System.md) (nomes localizáveis).
