# 59 — Dodge Direcional 8-Way · 🟢 P0 (M1)

> Responde **"faz sentido 8 direções de dodge pro meu jogo?"** (sim — pelo motivo certo, §0), e documenta a implementação real: o `GA_Dash` agora toca uma **montage direcional por seção** (C++ pronto, build limpo) + o **passo a passo no editor**. Assets: pastas `06_Dodge/` do set ([22](../22_Animation_Inventory.md)).

---

## 0. 🎯 A resposta de design: 8 faz sentido? SIM — pelo motivo certo

O instinto diz "topdown + orient-to-movement gira o corpo pro movimento, então só preciso de 1 clip (frente)". **Quase** — o detalhe que muda tudo:

| Situação | O corpo aponta pra... | Dodge precisa de direção? |
|---|---|---|
| **Correndo livre** (sem combate) | direção do movimento | ❌ Não — o corpo já gira; "F" cobre |
| **🔑 DASH-CANCEL em combate** ([15 §6](15_Combat_Overview.md)) | **o inimigo** (engajado no golpe) | ✅ **SIM** — esquivar pra trás/lado **sem girar de costas** pro alvo |

O dash-cancel é o **cancelamento-âncora nº 1** do combate. Quando você corta um golpe com dash, o corpo está virado pro inimigo — e a esquiva tem que sair **lateral/pra trás mantendo o facing** (é literalmente pra isso que o set tem `Dodge_Combat_B`, `_L`, `_R`…). Girar o personagem de costas no meio do combate ficaria horrível e ilegível.

**E os custos:**
- Os **8 clips JÁ EXISTEM** (custo de asset = zero).
- A implementação por **seções de montage** custa o mesmo pra 4 ou 8 (mesmo código, +4 seções).
- ⚖️ *Se você NÃO tivesse os clips:* 4 direções (F/B/L/R) bastariam — em topdown a -55°, a diferença diagonal lê pouco. Mas tendo, use os 8.

> 📐 **Decisão registrada:** dodge **direcional 8-way mantendo o facing** (default). O modo alternativo "Hades" (gira pra direção e usa 1 clip) existe no código como `bFaceDashDirection=true` — bom pra comparar feel no PIE.

### 0b. "Mas 8-way não seria pra STRAFE/LOCK-ON?" — sua intuição está certa

**Sim** — o lar canônico do dodge direcional é o modelo **target-facing** (strafe/lock-on, estilo Souls/DMC com lock): o corpo encara o alvo o tempo todo, então TODA esquiva é relativa ao facing. Como isso se encaixa no DDR:

| Modelo | DDR tem? | O 8-way vale? |
|---|---|---|
| **Hard lock-on** (trava alvo, strafe contínuo) | ❌ não no MVP ([18 §6](18_Combat_System_Deep.md): decisão = soft-lock, sem hard lock) | — (futuro) |
| **🔑 Soft-lock + facing de ataque** ([18 §6](18_Combat_System_Deep.md), [60 §7](../systems/60_M2_Editor_Setup.md)) | ✅ **SIM** — `FaceSoftLockTarget` no startup do golpe (C++ em `UDDRCombatComponent`) | ✅ é AQUI que o 8-way paga hoje: dash-cancel no meio do golpe = corpo encarando o alvo = esquiva direcional correta |
| Orient-to-movement (fora de combate) | ✅ | ➖ corpo já gira pro movimento → quase sempre `F` |

> 🎯 **E o melhor:** a implementação por **ângulo facing→direção** é *exatamente* a que um lock-on usa. Se um dia o hard lock-on entrar (P2), **nada muda no dodge** — com o facing travado no alvo, a mesma seleção de seção produz o strafe-dodge clássico. Construímos a fundação certa.

---

## 1. O que o C++ entrega (já compilado)

`UGA_Dash` ganhou:

| Membro | O que faz |
|---|---|
| **`DashMontage`** (`UAnimMontage`, EditDefaults) | montage com **8 seções** — escolhida por ângulo. **Opcional**: sem ela, o dash é só deslocamento |
| **🔑 AUTO-DETECT de root motion** | `DashMontage->HasRootMotion()`? **Modo A**: o **CLIP dirige a cápsula**. Senão, **Modo B**: ConstantForce dirige e a montage é só visual. Ver §2.4 |
| **`RootMotionTranslationScale`** (default 1.0) | Modo A: **escala o deslocamento autorado** dos clips — gancho pro Eco "dash +X%" sem reautorar |
| **`bFaceDashDirection`** (default `false`) | `true` = gira o ator pra direção e toca sempre `F` ("modo Hades", 1 clip) |
| **🆕 `RunSectionSuffix`** (default `"_Run"`) | **dodge→corrida pela intenção:** segurando movimento ao dashar → usa a seção `<dir>_Run` (família `06_Dodge_Combat_to_Run`, termina **emendando na corrida**); dash parado → seção base (termina em idle). **Fallback gracioso:** se a seção `_Run` não existe na montage, usa a base — funciona antes mesmo de você adicionar as seções novas (§2.3) |
| **Seleção por ângulo** | ângulo assinado entre o **facing atual** e a direção do dash → setor de 45° → seção `F/FR/R/BR/B/BL/L/FL` |
| **Trava de rotação** | `bOrientRotationToMovement` desligado durante o dash e **restaurado no EndAbility** (no Modo A a física de rotação já pausa durante anim-RM; a trava cobre os blends) |

```
Fluxo: IA_Dash → GA_Dash
  1. Commit (cooldown 0.6s) + GE_DDRDashIFrames (0.25s)
  2. trava rotação · calcula seção pelo ângulo facing→direção
  3a. MODO A (clips COM root motion): PlayMontageAndWait(seção) → o clip desloca a cápsula
       (distância/easing autorados × RootMotionTranslationScale) → fim/interrupção → EndAbility
  3b. MODO B (clips in-place): Montage visual + RootMotionConstantForce (550cm/0.22s)
       → fim do force → EndAbility (recovery visual segue, interrompível pelo slot)
```

---

## 2. 🛠️ Passo a passo no editor

### 2.1 Criar a montage `AM_Dodge`

1. `Content/DarkDungeonRift/Animation/` → clique direito no clip **`Dodge_Combat_F`** → *Create → Anim Montage* → renomeie **`AM_Dodge`**.
2. Arraste os **outros 7 clips** pra timeline, em sequência (a ordem não importa — as seções isolam).
3. Crie **8 seções**, uma no início de cada clip, com os **nomes EXATOS** que o C++ procura:

| Seção (nome exato) | Clip do set (`06_Dodge/02_Dodge_Combat/`) | Direção (relativa ao facing) |
|---|---|---|
| `F` | `Dodge_Combat_F` | frente |
| `FR` | `Dodge_Combat_F_R` | frente-direita |
| `R` | `Dodge_Combat_R` | direita |
| `BR` | `Dodge_Combat_B_R` | trás-direita |
| `B` | `Dodge_Combat_B` | trás |
| `BL` | `Dodge_Combat_B_L` | trás-esquerda |
| `L` | `Dodge_Combat_L` | esquerda |
| `FL` | `Dodge_Combat_F_L` | frente-esquerda |

4. ⚠️ **Montage Sections → [Clear]** — o MESMO passo crítico do combo ([57 §0](57_M1_Combo_Editor_Setup.md)): seções linkadas = toca os 8 dodges em sequência com 1 dash. **Cada seção numa linha, sem setas.**
5. Confirme o **Slot = `DefaultGroup.DefaultSlot`** (igual à `AM_Combo` — é o que deixa ataque cortar dodge e vice-versa).

### 2.2 Ligar no GA_Dash

1. Blueprint Class → parent **`GA_Dash`** → **`BP_GA_Dash`**.
2. Class Defaults → **Dash Montage = `AM_Dodge`** (deixe `bFaceDashDirection` desligado).
3. `BP_DDRPlayer` → **Startup Abilities** → troque `GA_Dash` por **`BP_GA_Dash`** (mantém Input ID `Dash`).

### 2.3 🆕 Dodge → Run (família `06_Dodge_Combat_to_Run`) — as DUAS famílias juntas

> **Vale a pena? SIM** — é polish barato de altíssima fluidez: o jogo é sobre reposicionar entre grupos, então *dodge→continuar correndo* acontece o tempo todo. Sem o `to_Run`, o fluxo é dodge → para em idle → re-acelera (hitch feio quando o jogador está segurando o stick). E os clips **já existem**.

**A regra (já no C++):** a intenção do jogador escolhe a família —

```
SEGURANDO movimento ao dashar  → seção "<dir>_Run"  (dodge que SAI CORRENDO)
Dash parado (sem input)        → seção "<dir>"      (dodge que termina em idle — o que você já montou)
```

**Como montar (na MESMA `AM_Dodge`):**

1. Arraste os clips de **`06_Dodge_Combat_to_Run`** pro fim da timeline da `AM_Dodge`.
2. Crie **+8 seções** com sufixo `_Run`, mapeando igual à tabela do §2.1:

| Seção | Clip (`06_Dodge_Combat_to_Run/`) |
|---|---|
| `F_Run` | `Dodge_Combat_to_Run_F` |
| `FR_Run` | `..._F_R` |
| `R_Run` | `..._R` |
| `BR_Run` | `..._B_R` |
| `B_Run` | `..._B` |
| `BL_Run` | `..._B_L` |
| `L_Run` | `..._L` |
| `FL_Run` | `..._F_L` |

3. ⚠️ **Montage Sections → [Clear]** de novo (agora são 16 seções — todas sem setas!).
4. **`EnableRootMotion = TRUE`** também nos 8 clips novos (Modo A, §2.4).
5. *A pasta tem **9 itens** — confira o extra (provavelmente uma variante duplicada de F); use os 8 que casam com a tabela.*

**Notas de comportamento (Modo A):**
- Os clips `to_Run` costumam ter **rotação autorada no root** (o personagem rola e *vira* pra sair correndo na direção) — isso é **desejável** e funciona: a trava de rotação só impede a *física* de girar; o root motion autorado do clip gira normalmente.
- No fim do clip, a velocidade volta pro CMC: com o stick segurado e `MaxAcceleration` 2048, o blendspace Run assume em ~2 frames — handoff limpo. Se sentir micro-hitch, encurte o **Blend Out** da montage (0.15-0.2s).

### 2.4 Root motion dos clips — o GA_Dash AUTO-DETECTA (configure o Modo A)

> ⚠️ **Correção (versão anterior deste doc):** o conselho era "desligue EnableRootMotion nos clips". **ERRADO para clips autorados COM deslocamento** (como os do seu set): com `EnableRootMotion=false`, o root bone anima visualmente → o **mesh derrapa pra fora da cápsula e "snapa" de volta** no blend out. O código agora resolve isso por auto-detect.

**Os dois modos (o C++ escolhe sozinho via `HasRootMotion()`):**

| | **Modo A — clip dirige** (✅ o certo pros seus clips) | **Modo B — código dirige** |
|---|---|---|
| Para clips… | **autorados COM root motion** (dodge packs típicos) | in-place (sem deslocamento no root) |
| Quem move a cápsula | **o clip** (`PlayMontageAndWait` roteia o RM pro CMC) | `RootMotionConstantForce` (550cm/0.22s) |
| Distância/easing | **autorados pelo animador** (arranque forte, desacelera — feel pronto); B-dodge mais curto que F é *intencional* | uniforme, velocidade constante (mais "slide") |
| Tuning / Eco | `RootMotionTranslationScale` (escala o deslocamento; Eco "dash +X%") | `DashDistance`/`DashDuration` (dados) |
| Rotação durante | física de rotação já pausa em anim-RM (+ nossa trava) | trava manual (feita) |
| Colisão | ✅ respeita paredes | ✅ respeita paredes |

**Como configurar o Modo A (1 passo por clip):** abra cada uma das **8 Animation Sequences** de `02_Dodge_Combat` → *Asset Details* → **`EnableRootMotion = ✅ TRUE`** (o "Root Motion Root Lock" pode ficar no default *Ref Pose*). Com isso o root fica visualmente travado, a montage extrai o deslocamento e o entrega à cápsula — sem drift, sem snap, sem soma dobrada.

> 🔎 **Como saber em qual modo está:** no PIE, se o B-dodge anda **menos** que o F-dodge → Modo A (distâncias autoradas ✅). Se todas as direções andam exatamente igual (~550) → Modo B. Se o mesh **derrapa e volta** → clip com RM mas `EnableRootMotion=false` → ligue (passo acima).

---

## 3. Qual variante das 6 pastas usar (mapa do set)

| Pasta | Uso | Quando |
|---|---|---|
| **`02_Dodge_Combat`** | **← O M1 usa esta** (postura de espada, casa com `Idle_Combat`/combo) | 🟢 **agora** |
| `01_Dodge` (normal) | dodge desarmado/relax — se um dia houver estado "fora de combate" real | 🔵 P2 |
| `03_Dodge_Air` / `04_Dodge_Air_Combat` | **esquiva aérea** durante o juggle — entra com o pilar | 🟡 **M2** ([16](16_Aerial_Combos.md)) |
| `05_Dodge_to_Run` / `06_Dodge_Combat_to_Run` | transições dodge→corrida (polish; o blend out do slot já cobre razoável) | 🔵 P2 |

---

## 4. Teste (PIE)

| Teste | Esperado |
|---|---|
| Parado + dash (sem input) | dodge **pra frente** (`F`, família base) → termina em **idle** |
| **Segurando movimento + dash** | seção **`_Run`** → o dodge **sai correndo emendado** (se §2.3 montado; senão base) |
| Correndo + dash + SOLTAR o stick no meio | toca `_Run` mas o CMC para no fim (sem input) — aceitável; blend out disfarça |
| Correndo + dash na mesma direção | `F` (facing ≈ direção) |
| **Atacando o dummy + dash pra trás** (S+dash) | **`B`** — esquiva de costas **sem girar** o corpo 🔑 |
| Atacando + dash lateral (A/D+dash) | `L`/`R` mantendo o facing no dummy |
| Dash → atacar imediato | ataque **corta** o recovery do dodge (mesmo slot) |
| Ataque → dash (dash-cancel) | dodge corta o golpe, direção certa |
| `ddr.LocomotionDebug 1` | `Dash=1` durante (tag dos i-frames) |

---

## 5. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| **Toca os 8 dodges em sequência** | seções LINKADAS | §2.1 passo 4 — **Clear** ([57 §0](57_M1_Combo_Editor_Setup.md)) |
| Sempre toca a mesma direção (`F`) | nomes de seção errados (o C++ procura `F/FR/R/BR/B/BL/L/FL` exatos) | §2.1 passo 3 |
| **Mesh derrapa e "snapa" de volta** | clip COM root motion mas `EnableRootMotion=false` | §2.4 — ligar `EnableRootMotion` em TODOS os clips da montage (Modo A) |
| Distâncias diferentes por direção | Modo A: autorado pelo animador (B < F é normal) | intencional; uniformizar = `RootMotionTranslationScale` ou Modo B |
| Dodge "comprometido" demais (preso até o fim do clip) | Modo A usa a duração do clip | atacar corta (slot); ou encurte o blend out da montage |
| Corpo **gira** no meio do dodge | build antiga (sem a trava de rotação) | recompile — `bOrientRotationToMovement` é travado/restaurado pelo GA |
| Montage não toca | sem `DashMontage` no BP_GA_Dash, ou ABP sem Slot | §2.2 / [58 §4](../locomotion/58_AnimGraph_Step_by_Step.md) |
| Esquerda/direita invertidas | mapeamento clip↔seção trocado | §2.1 tabela (L = `Dodge_Combat_L`) |
| Seção `_Run` nunca toca | nomes errados (`F_Run` exato) ou clips não adicionados | §2.3 — sufixo default `_Run` (editável no BP) |
| Hitch de velocidade no fim do `to_Run` | handoff montage→CMC | encurtar Blend Out (0.15-0.2s); accel 2048 retoma em ~2 frames |
| Dodge não corta o ataque | `CancelAbilitiesWithTag` sem `Ability.Attack` | [19 ficha GA_Dash](19_Abilities_Deep.md) |
| Quero comparar com "modo Hades" | — | `bFaceDashDirection = true` no BP_GA_Dash (1 clip, gira o corpo) |

---

## 6. Próximo passo

→ **Perfect-dodge** (sub-janela nos i-frames → witch-time): [56 — Combate Defensivo](56_Defensive_Combat.md) — agora com o dodge visual pronto, é o próximo upgrade do verbo. · Esquiva aérea (`04_Dodge_Air_Combat`): junto do pilar no **M2** ([16](16_Aerial_Combos.md)).
