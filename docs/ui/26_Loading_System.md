# 26 — Tela & Sistema de Loading · 🟡 P1

> Como carregar níveis/assets **sem travar o frame** e mostrar uma tela de loading legível. Pré-req: [23 — UI Overview](23_UI_Overview.md).

> 🎯 **Por que importa:** um `OpenLevel` síncrono **congela o jogo** (hitch feio) durante o carregamento. A tela de loading existe pra (a) esconder esse hitch e (b) dar feedback de que algo está acontecendo. Em roguelike, as transições são frequentes (entre runs/salas) — vale fazer direito.

---

## 1. Dois tipos de "loading" no jogo

| Tipo | Quando | Técnica |
|---|---|---|
| **Transição de nível** | Main Menu → Run; fim de run → Hub | Loading screen + `OpenLevel` (async) |
| **Entre salas (mesma run)** | Sala → sala dentro da run | **Level streaming** (sem tela cheia) ou load curto |

> Para um roguelike de **arenas**, o ideal é que a troca de sala seja **rápida ou seamless** (streaming), não uma tela cheia a cada porta. Tela de loading só nas transições grandes (entrar/sair da run).

---

## 2. Tela de loading (widget)

```
┌──────────────────────────────────────────────┐
│                                              │
│                                              │
│         (arte / cena escura)                 │
│                                              │
│   💡 "Combos aéreos curam se você pegar       │  ← dica/lore (Hades/DMD style)
│      o Eco de Carniça."                       │
│                                              │
│                              Carregando… ◐   │  ← spinner / progresso
└──────────────────────────────────────────────┘
```

- **Camada 5** (cobre tudo, [23 §2](23_UI_Overview.md)).
- **Dica/lore rotativa:** barata e dá personalidade (referência: Hades, Death Must Die). Ensina mecânicas/builds enquanto carrega.
- **Spinner** ou barra. Barra de progresso real é difícil com async — um spinner "indeterminado" é honesto e suficiente.

---

## 3. Como carregar sem travar

### Opção A — Loading widget + async open (recomendado MVP)
```
1. UDDRUIManager.ShowLoading()           // sobe a tela (camada 5)
2. (1 frame pra UI renderizar)
3. UGameplayStatics::OpenLevel(...)       // ou async load
4. No novo nível, BeginPlay → preload de assets críticos (§4)
5. UDDRUIManager.HideLoading() quando pronto
```

### Opção B — MoviePlayer (loading screen "de verdade" durante o stall)
O `OpenLevel` em si trava a game thread. Pra mostrar UI *durante* esse stall, use o **MoviePlayer** (`IGameMoviePlayer` / `LoadingScreenModule`) — ele renderiza numa thread separada e sobrevive ao carregamento do mapa. É o jeito AAA de "loading screen que não congela". P1.

### Opção C — Level streaming (entre salas)
```
LoadStreamLevel(nome, bMakeVisibleAfterLoad=true, ...)   // assíncrono, sem tela cheia
UnloadStreamLevel(sala_anterior)
```
Para salas dentro da run: carrega a próxima arena em streaming enquanto o player termina a atual → transição quase seamless.

---

## 4. Preload de assets (mata o hitch do primeiro hit)

A pior trava não é o nível — é o **primeiro uso** de um asset não carregado (a primeira montage de combo, o primeiro Niagara de hit). Use o **Asset Manager** pra pré-carregar:

```cpp
// no loading, antes de soltar o jogador:
StreamableManager.RequestAsyncLoad(AssetsDoCombate, OnLoaded);
// AssetsDoCombate = montages do combo/aéreo, cues de hit (21), VFX/SFX
```

> 🥊 Sem preload, o **primeiro combo** do jogador engasga enquanto carrega a montage/VFX — péssima 1ª impressão. Pré-carregue o kit de combate no loading.

---

## 5. MVP vs polish

| Item | Prioridade |
|---|---|
| Loading widget (camada 5) + spinner | 🟡 P1 |
| Async open level (não travar) | 🟡 P1 |
| Preload do kit de combate | 🟡 P1 (mata hitch do 1º hit) |
| Dicas/lore rotativas | 🟡 P1 (barato, alto charme) |
| Level streaming entre salas (seamless) | 🟡 P1 |
| MoviePlayer (loading durante o stall real) | 🔵 P2 |
| Barra de progresso real | 🔵 P2 |

> No MVP-cedo, um fade-to-black + `OpenLevel` já dá pra testar o loop. A tela bonita vem com o M4-M5.

---

## 6. Checklist

- [ ] `ShowLoading`/`HideLoading` no `UDDRUIManager` (camada 5)
- [ ] Open level assíncrono (não síncrono na game thread)
- [ ] **Preload do kit de combate** no loading (anti-hitch do 1º hit)
- [ ] Dicas/lore rotativas
- [ ] Streaming entre salas (P1) pra transição rápida
- [ ] Fade in/out pra esconder o "pop" do nível

---

## 7. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Jogo congela ao trocar de nível | `OpenLevel` síncrono sem tela | §3 — loading widget + async / MoviePlayer |
| 1º combo engasga | Asset não pré-carregado | §4 — preload |
| Tela de loading "pisca" | Some antes do nível pronto | §3 — só `HideLoading` no callback de pronto |
| Transição de sala lenta | Load cheio em vez de streaming | §3 Opção C |
| Loading screen não aparece (só congela) | UMG não renderiza durante stall da game thread | §3 Opção B (MoviePlayer) |

---

## 8. Próximo passo

→ [27 — Settings](27_Settings.md) · [28 — HUD](28_HUD.md) (o que aparece quando o loading some).
