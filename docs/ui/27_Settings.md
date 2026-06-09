# 27 — Tela de Settings (Ajustes) · 🟡 P1

> Tela de configurações: vídeo, áudio, controles, jogabilidade e **acessibilidade**. Pré-req: [23 — UI Overview](23_UI_Overview.md).

> 🎯 **MVP mínimo:** volume + fullscreen + 1-2 toggles já destravam o playtest. O resto é P1. Mas **planeje as categorias** desde já pra não refazer a navegação.

---

## 1. Categorias

| Categoria | Itens | Prioridade |
|---|---|---|
| **🎬 Vídeo** | Resolução · Modo janela (Fullscreen/Borderless/Windowed) · VSync · Cap de FPS · Qualidade (escala) | 🟡 P1 |
| **🔊 Áudio** | Master · Música · SFX · UI (sliders 0-100) | 🟢 P0 (volume é básico) |
| **🎮 Controles** | Sprint hold/toggle · Sensibilidade (Modelo B) · Vibração on/off · Rebind ([39](../systems/39_Controls.md)) | 🟡 P1 (rebind P2) |
| **🕹️ Jogabilidade** | Idioma ([38](../systems/38_Localization.md)) · Mostrar damage numbers · Auto-pickup · Dificuldade (P2) | 🟡 P1 |
| **♿ Acessibilidade** | **Escala de screen-shake** · **Escala de hit-stop** · Reduzir flashes · Daltonismo · Tamanho de texto | 🟡 P1 (alto valor) |

---

## 2. ♿ Acessibilidade ↔ Juice (conexão importante)

O [doc 21 — Juice](../feel/21_Juice_FX.md) propôs as cvars **`ddr.ShakeScale`** e **`ddr.HitStopScale`** justamente pra *tuning E acessibilidade*. A tela de Settings **expõe esses knobs ao jogador**:

| Setting | Liga em | Por quê |
|---|---|---|
| **Escala de screen-shake** (0-100%) | `ddr.ShakeScale` | Shake forte enjoa/causa desconforto a muita gente |
| **Escala de hit-stop** (0-100%) | `ddr.HitStopScale` | Alguns querem combate mais fluido, menos freeze |
| **Reduzir flashes** | desliga flashes fortes / slow-mo | Fotosensibilidade (o jogo tem hit-flash + witch-time) |
| **Daltonismo** | troca paleta de telegrafe | O telegrafe vermelho ([21](../feel/21_Juice_FX.md)) precisa ser legível |

> 🔗 Isso **conecta UI ↔ juice ↔ feel**: o que o doc 21 chamou de "knob de tuning" vira **opção de acessibilidade** aqui. Barato e inclusivo. Em **single-player**, mexer no time-dilation/shake por config é trivial.

---

## 3. Vídeo via `UGameUserSettings`

UE já tem persistência de vídeo pronta — **não reinvente**:

```cpp
auto* S = UGameUserSettings::GetGameUserSettings();
S->SetScreenResolution(...); S->SetFullscreenMode(...); S->SetVSyncEnabled(...);
S->SetOverallScalabilityLevel(0..3);   // Low..Epic
S->SetFrameRateLimit(60.f);
S->ApplySettings(false);  S->SaveSettings();
```

Áudio/jogabilidade/acessibilidade (que não são do GameUserSettings) → salve num **`UDDRSettingsSave`** (USaveGame separado, [25](25_Save_Load_Slots.md)) e aplique no boot.

---

## 4. Apply / Revert / Defaults (padrão de UX)

```
[ Aplicar ]  → comita as mudanças (e salva)
[ Reverter ] → volta aos valores de quando abriu a tela
[ Padrões ]  → reseta pra default
```

- **Resolução/fullscreen:** aplicar com **confirmação por timeout** ("Manter? 10s…") — se o jogador escolher uma resolução quebrada, reverte sozinho. Padrão AAA.
- Sliders de áudio: aplicam **ao vivo** (ouvir enquanto arrasta).

---

## 5. Onde a tela abre

- Do **Main Menu** ([24](24_MainMenu.md)) → camada menu, input UIOnly.
- Do **Pause** ([29 §2](29_Run_Flow_Menus.md)) durante a run → mesma tela, jogo pausado.

> A **mesma** `WBP_Settings` serve nos dois lugares — não duplique.

---

## 6. Layout (diagrama)

```
┌───────────────────────────────────────────────┐
│  AJUSTES                                       │
│ ┌─────────┐  Modo janela:  [ Borderless ▾ ]    │
│ │▶ Vídeo  │  Resolução:    [ 1920x1080 ▾ ]     │
│ │  Áudio  │  VSync:        [ On ]              │
│ │ Control.│  Cap FPS:      [ 60 ]              │
│ │ Jogab.  │  Qualidade:    [ Alto ▾ ]          │
│ │ Acess.  │                                    │
│ └─────────┘                                    │
│   [Padrões]            [Reverter]  [Aplicar]   │
└───────────────────────────────────────────────┘
```

Abas à esquerda (navegáveis por LB/RB no pad), conteúdo à direita.

---

## 7. MVP vs polish

| Item | Prioridade |
|---|---|
| Sliders de volume (Master/Música/SFX) | 🟢 P0 |
| Modo janela + resolução + VSync | 🟡 P1 |
| Qualidade (scalability) + cap FPS | 🟡 P1 |
| Acessibilidade (shake/hitstop/flash) | 🟡 P1 (alto valor, barato) |
| Idioma | 🟡 P1 |
| Rebind de teclas | 🔵 P2 |
| Confirmação por timeout de resolução | 🔵 P2 |

---

## 8. Checklist

- [ ] Categorias com navegação por abas (gamepad LB/RB)
- [ ] Vídeo via `UGameUserSettings` (Apply+Save)
- [ ] Áudio/gameplay/acessibilidade em `UDDRSettingsSave`
- [ ] Settings aplicados no **boot** (não só em runtime)
- [ ] **Acessibilidade liga `ddr.ShakeScale`/`ddr.HitStopScale`** ([21](../feel/21_Juice_FX.md))
- [ ] Mesma `WBP_Settings` no Main Menu e no Pause
- [ ] Apply / Reverter / Padrões

---

## 9. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Vídeo não persiste | Faltou `SaveSettings()` | §3 |
| Volume não muda | Mixer/SoundClass não ligado ao slider | bind no SoundMix/SoundClass |
| Resolução quebrada trava o jogador | Sem confirmação por timeout | §4 |
| Acessibilidade não afeta o jogo | cvar não lida pelo sistema de juice | §2 — ligar `ddr.*Scale` |
| Settings somem no boot | Não aplicados no início | §8 — aplicar no startup |

---

## 10. Próximo passo

→ [28 — HUD](28_HUD.md) · [29 — Run-Flow Menus](29_Run_Flow_Menus.md) · [38 — Localização](../systems/38_Localization.md) (dropdown idioma).
