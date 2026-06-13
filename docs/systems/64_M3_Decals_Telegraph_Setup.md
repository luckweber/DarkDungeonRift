# 64 — M3 Decals: Sombra Blob & Telegrafe (passo a passo) · 🟡 LUTAR

> **O que fazer no Unreal Editor** para os dois decals do M3: **sombra blob** (leitura de altura do juggle) e **telegrafe no chão** (leitura de perigo — o item nº1 da justiça topdown, [32 §1](../enemies/32_Enemy_Combat_Behavior.md)). Materiais, components e GameplayCues, clique a clique. Detalha o [61 §7](61_M3_Editor_Setup.md) e **corrige a receita resumida** do [60 §8](60_M2_Editor_Setup.md). Refs: [36 §4 VFX](../feel/36_VFX_Niagara.md) · [37 §5 linha](../combat/37_Projectiles.md).

> ✅ **STATUS C++:** nada a compilar — os ganchos já existem e foram conferidos no código:
> - `GA_Enemy_Melee_Slash` executa **`GameplayCue.Enemy.Telegraph`** no início do windup (Instigator/MyTarget = inimigo, Location = posição dele); o windup vem de `UDDREnemyData → Telegraph.WindupSeconds`.
> - `GA_Enemy_Ranged_Bolt` executa **`GameplayCue.Proj.Telegraph`** (MyTarget = archer, **Location = posição do ALVO**); a duração vem de `UDDRProjectileData.TelegraphDuration` (0.55 default).
> - `FDDRTelegraphConfig` (categoria **Combat → Telegraph** do DataAsset) expõe pro BP: `WindupSeconds` · `ConeAngleDegrees` · `ConeRadius` · `Color`.

---

## 0. Checklist rápido

- [ ] Entendi a caixa de projeção (§1) — evita 90% dos erros
- [ ] `M_BlobShadow` corrigido (§2)
- [ ] Decal de sombra no `BP_DDRPlayer`, `BP_TrainingDummy` e `BP_Enemy_Grunt` (§3) + **Receives Decals OFF** nas meshes
- [ ] `M_Telegraph_Circle` (§4) → `M_Telegraph_Cone` (§5) → `M_Telegraph_Line` (§6)
- [ ] `BP_TelegraphDecal` com timeline de Fill (§7)
- [ ] `GCN_Enemy_Telegraph` + `GCN_Proj_Telegraph` (§8)
- [ ] `DA_Enemy_Grunt_Melee` → Telegraph com números honestos (§9)
- [ ] PIE: "sei onde NÃO estar olhando só pro chão" (§10)

---

## 1. Como decal funciona (2 min que evitam 90% dos erros)

Um material de domínio **Deferred Decal** não é aplicado num mesh — ele é projetado por uma **caixa invisível** (`UDecalComponent`) em qualquer superfície dentro dela:

```
DecalSize = MEIA-extensão (X, Y, Z)     Rotação no painel Details do component:
  X = profundidade (eixo da projeção)     campo X = Roll · campo Y = Pitch · campo Z = Yaw
  Y e Z = pegada no chão                → para projetar NO CHÃO: campo Y (Pitch) = -90
```

As cinco regras:

1. **`DecalSize` é metade.** (128, 45, 45) = caixa com 256uu de fundo e pegada de **90×90uu** no chão. (80, 160, 160) vira um blob de **320uu** de diâmetro — gigante.
2. **A projeção sai pelo eixo X local.** Com rotação (0,0,0) o decal projeta pra FRENTE (vira borrão esticado no chão, ou nada). **Pitch −90** (campo **Y** do painel) aponta pra baixo.
3. **A forma vive no pino Opacity**, não no Base Color. Opacity 0 = invisível, 1 = pinta a superfície. Gradiente no Base Color com Opacity 1 = retângulo branco do tamanho da caixa.
4. Com Pitch −90, o **U da textura corre na direção da frente do ator** e o V na direção da direita (verificado no shader da 5.4: `DeferredDecal.usf`, swizzle `zyx`). Por isso o cone do §5, desenhado apontando pra +U, sai **na frente do inimigo** sem calibração.
5. **A esfera de preview do editor de material engana** — decal não projeta nela (fica branca/estranha mesmo com o material certo). Julgue **no level**.

> ⚙️ Pré-condição do projeto (default da 5.4, só confira se nada aparecer): **Project Settings → Rendering → Decals → DBuffer Decals = ON**.

---

## 2. `M_BlobShadow` — o material da sombra (corrigir)

Em `Content/DarkDungeonRift/Materials/M_BlobShadow`:

1. **Details:** Material Domain = **Deferred Decal** · Blend Mode = **Translucent** (Shading Model fica travado em Default Lit — ok).
2. `Constant3Vector` **(0, 0, 0)** → **Base Color** (a sombra é preta).
3. Node `RadialGradientExponential`: deixe UVs/CenterPosition default · **Radius = 0.45** · **Density = 6** (centro cheio, borda macia).
4. `ScalarParameter` **`ShadowOpacity`** = **0.55**.
5. `Multiply` (3 × 4) → **Opacity**.
6. **Apply + Save.**

```
RadialGradientExponential ──┐
                            Multiply ──► Opacity
ShadowOpacity (0.55) ───────┘
Constant3 (0,0,0) ──────────────────────► Base Color
```

> ⚠️ **O erro clássico:** ligar o `RadialGradientExponential` no **Base Color** e deixar Opacity = 1.0. Resultado: blob BRANCO ocupando a caixa inteira (o gradiente é 1 no centro = branco, e opacity 1 pinta tudo). No decal, **a forma é o Opacity**; o Base Color é só a cor do que pintou.

---

## 3. Sombra nos personagens (player, dummy, grunt)

Nos **três** BPs — `BP_DDRPlayer`, `BP_TrainingDummy`, `BP_Enemy_Grunt` (e todo inimigo novo):

1. **Add Component → Decal**, nome `ShadowDecal`, filho do **CapsuleComponent** (não da mesh — a mesh anima/ragdolla; a cápsula é o corpo que o juggle move).
2. No Details do decal:

| Propriedade | Valor | Por quê |
|---|---|---|
| Decal Material | `M_BlobShadow` | §2 |
| **Rotation** | X **0** · **Y −90** · Z **0** | Y = Pitch → projeta pra baixo (§1.2) |
| Location | (0, 0, 0) | centro da cápsula |
| **Decal Size** | **(512, 45, 45)** | X 512 = alcança o chão até no topo do juggle · 45 = blob de ø90uu (no player pode 50) |
| Sort Order | **0** | o telegrafe (§7) desenha por cima com 10 |
| Fade Screen Size | **0** | nunca some com zoom out |

3. 🔥 **Em TODAS as meshes de personagem** — `Mesh` e `WeaponMesh` do player e do grunt, mesh do dummy: Details → Rendering → **Receives Decals = OFF**. Sem isso a sombra (e o telegrafe!) **pinta os pés/corpo** dos personagens que pisarem na zona.
4. **Teste (critério do M2):** lance o dummy — a sombra **fica no chão** e o gap sombra↔corpo = altura. Se ela sumir no topo do juggle, aumente o Decal Size **X**.

> 📝 Substitui a receita resumida do [60 §8](60_M2_Editor_Setup.md) (lá dizia "tamanho ~(80, 80, 200)" — eixos trocados: a profundidade é o **X**, e os valores são **meia**-extensão).

---

## 4. `M_Telegraph_Circle` — o material base do telegrafe

Crie a pasta **`Content/DarkDungeonRift/VFX/Telegraph/`** ([36 §1](../feel/36_VFX_Niagara.md)) e dentro dela o material `M_Telegraph_Circle`.
**Details:** Material Domain = **Deferred Decal** · Blend Mode = **Translucent**.

**Parâmetros** (criam os knobs do MID — nomes exatos, case-sensitive):

| Parâmetro | Tipo | Default | Função |
|---|---|---|---|
| `Color` | Vector | (1, 0.08, 0.08, 1) | cor do perigo (daltonismo troca depois, [36 §4](../feel/36_VFX_Niagara.md)) |
| `Fill` | Scalar | 0 | 0→1 durante o windup — o disco que cresce |
| `OpacityMax` | Scalar | 0.65 | teto de opacidade |
| `PulseRate` | Scalar | 2.5 | pulsos por segundo |
| `EmissiveBoost` | Scalar | 3 | brilho (legível em chão claro E escuro) |

**O grafo em 4 blocos** (use **Named Reroute** nos resultados intermediários pra não virar espaguete):

**(a) `Dist` — distância radial: 0 no centro → 1 na borda da caixa**

1. `TextureCoordinate[0]` → `Multiply` (Const B = **2.0**) → `Subtract` (Const B = **1.0**) → Named Reroute **`UVc`** (UV centrado, −1..+1).
2. `Distance` (A = `UVc`, B = `Constant2Vector (0,0)`) → Named Reroute **`Dist`**.

**(b) `Ring` — anel fixo na borda (o contorno da zona, sempre visível)**

3. `SmoothStep` (Value = `Dist`, Min = **0.88**, Max = **0.92**).
4. `SmoothStep` (Value = `Dist`, Min = **0.97**, Max = **1.0**) → `OneMinus`.
5. `Multiply` (3 × 4) → Named Reroute **`Ring`**.

**(c) `FillDisc` — o disco que cresce (o relógio do windup)**

6. `Subtract` (A = param `Fill`, Const B = **0.05**).
7. `SmoothStep` (Value = `Dist`, Min = nó 6, Max = `Fill`) → `OneMinus` → Named Reroute **`FillDisc`**.

**(d) Pulso e saídas**

8. `Time` → `Multiply` (× param `PulseRate`) → `Sine` (Period = **1.0**) → `Multiply` (Const B = 0.5) → `Add` (Const B = 0.5) → `Lerp` (A = **0.55**, B = **1.0**, Alpha = resultado) → Named Reroute **`Pulse`**.
9. `Multiply` (`FillDisc` × Const **0.75**) → `Multiply` (× `Pulse`) → `Add` (+ `Ring`) → `Saturate` → Named Reroute **`Mask`**.
10. `Multiply` (param `Color` × param `EmissiveBoost`) → `Multiply` (× `Mask`) → **Emissive Color**.
11. param `Color` → **Base Color** (o vermelho lê mesmo onde o emissive não salta).
12. `Multiply` (`Mask` × param `OpacityMax`) → **Opacity**.

Resultado: **anel** vermelho marcando o limite da zona + **disco interno enchendo** conforme o windup + **pulso** de urgência.

> 👁️ **Preview:** a esfera default do editor **mente** (decal não projeta nela — vira "pétalos" vermelhos repetidos). Troque a malha de preview pra **Plane** (ícone de plano na barra inferior do viewport) → aí aparece o círculo real. Com `Fill = 0` você deve ver **só o anel fino**; suba `Fill` no painel Parameters e o disco interno cresce. Validação definitiva: jogue o material num `DecalActor`/`BP_TelegraphDecal` no chão (Pitch −90), nunca na esfera.

---

## 5. `M_Telegraph_Cone` — a variação do golpe do Grunt

**Duplique** `M_Telegraph_Circle` → `M_Telegraph_Cone` e acrescente a máscara angular:

1. Novo `ScalarParameter` **`ConeHalfAngleDeg`** = **60** (o C++ manda `ConeAngleDegrees` = 120; o BP divide por 2 no §8.1).
2. De `UVc`, crie **DOIS nós `ComponentMask` separados**: um com **só R** marcado e outro com **só G** marcado. ⚠️ **NÃO** use um único mask `RG` (float2) — o `Arctangent2Fast` exige **dois escalares**; pinos Y/X vazios = **ERROR!** e o material não compila.
3. `Arctangent2Fast` (**Y = Mask G**, **X = Mask R**) → `Abs` → ângulo 0..π, onde **0 = frente do ator** (§1.4). Ordem importa: `atan2(Y,X)` = 0 no eixo +X (= +U = frente).
4. `ConeHalfAngleDeg` → `Multiply` (Const B = **0.017453**) → meio-ângulo em radianos.
5. `Subtract` (A = nó 4, Const B = **0.06**) → `SmoothStep` (Value = nó 3, Min = nó 5, Max = nó 4) → `OneMinus` → Named Reroute **`ConeMask`**.
6. No bloco (d), nó 9: depois do `Add` (+ `Ring`) insira `Multiply` (× `ConeMask`) **antes** do `Saturate` — anel e fill ficam recortados no setor do cone.

> 🎯 O cone aponta pra **frente do inimigo** de graça: o U do decal corre no eixo da frente do ator (§1.4) e o `BP_TelegraphDecal` (§7) spawna com o yaw do inimigo (§8.1).

---

## 6. `M_Telegraph_Line` — a variação do Atirador

Duplique `M_Telegraph_Circle` → `M_Telegraph_Line`. Delete os blocos (b) anel e (c) fill (o windup do tiro é curto demais pra disco crescer) e troque a máscara:

1. De `UVc`: `ComponentMask (R)` → `Abs` → `SmoothStep` (Min = **0.90**, Max = **1.0**) → `OneMinus` → **pontas** (suaviza o início/fim da linha — o comprimento corre no U, §1.4).
2. De `UVc`: `ComponentMask (G)` → `Abs` → `SmoothStep` (Min = **0.55**, Max = **0.95**) → `OneMinus` → **largura** (borda lateral macia).
3. `Multiply` (1 × 2) → `Multiply` (× `Pulse`) → `Saturate` → **`Mask`** → segue pros mesmos nós 10–12 de saída.

---

## 7. `BP_TelegraphDecal` — o actor que anima o decal

Em `VFX/Telegraph/` → **Blueprint Class → Actor** → `BP_TelegraphDecal`. Um actor único serve cone, círculo e linha — o GCN (§8) escolhe material e medidas no spawn.

**Component:** **Add → Decal** (filho do DefaultSceneRoot), no Details:

| Propriedade | Valor | Nota |
|---|---|---|
| Decal Material | `M_Telegraph_Cone` | default; o spawn pode trocar |
| Relative Rotation | X 0 · **Y −90** · Z 0 | projeta pra baixo |
| **Decal Size** | **(256, 1, 1)** | tamanho **unitário**: o raio entra por **escala** no BeginPlay (na 5.4 `DecalSize` não é settável em BP — `BlueprintReadOnly`) |
| Sort Order | **10** | desenha por cima da sombra blob (0) |
| Fade Screen Size | 0 | |

**Variáveis** (todas **Instance Editable + Expose on Spawn**):

| Var | Tipo | Default | Uso |
|---|---|---|---|
| `Duration` | Float | 0.45 | tempo do Fill 0→1 (= windup) |
| `Radius` | Float | 150 | raio (cone/círculo) ou **meio-comprimento** (linha) |
| `WidthOverride` | Float | 0 | 0 = cone/círculo · >0 = linha (meia-largura, ex. 20) |
| `ConeHalfAngleDeg` | Float | 60 | só o cone usa |
| `Color` | LinearColor | (1, 0.08, 0.08, 1) | |
| `DecalMaterial` | Material Interface | `M_Telegraph_Cone` | trocar por Circle/Line no spawn |
| `ExtraLifetime` | Float | 0.2 | ver nota abaixo |

**Event Graph → BeginPlay:**

1. Decal → **Set Decal Material** (`DecalMaterial`).
2. Decal → **Set Relative Scale 3D** = (X **1.0**, Y = `Select` (`WidthOverride` > 0 ? `WidthOverride` : `Radius`), Z = `Radius`). O comprimento da linha corre no **Z local** (= U da textura, §1.4); a largura no Y.
3. Decal → **Create Dynamic Material Instance** → salve em var `MID` → **Set Vector Parameter Value** (`Color`) → **Set Scalar Parameter Value** (`ConeHalfAngleDeg`).
4. **Timeline `FillTL`**: Length = **1.0**, um track float `Alpha` linear com 2 keys (t=0 → 0, t=1 → 1). Antes do Play: **Set Play Rate** = `1 / max(Duration, 0.05)`.
5. **Update** → `MID` → Set Scalar **`Fill`** = Alpha.
6. **Finished** → **Delay** (`ExtraLifetime`) → **DestroyActor**.

> 🎯 **Por que `ExtraLifetime`:** o cue dispara no **início do windup**, mas o golpe REAL conecta um tiquinho depois (os frames de swing da montage até o `ANS_DDRHitbox`). 0.15–0.25s mantém a zona visível até o impacto — ela some **junto com o golpe**, não antes dele.

---

## 8. GameplayCues — ligar o C++ nos decals

O fluxo que já existe no C++ (não mexa):

```
GA_Enemy_Melee_Slash.ActivateAbility
  └─ ExecuteGameplayCue(GameplayCue.Enemy.Telegraph)    MyTarget/Instigator = inimigo
       windup = DA inimigo → Telegraph.WindupSeconds     Location = posição do inimigo

GA_Enemy_Ranged_Bolt.ActivateAbility
  └─ ExecuteGameplayCue(GameplayCue.Proj.Telegraph)     MyTarget/Instigator = archer
       windup = DA projétil → TelegraphDuration          Location = posição do ALVO
```

### 8.1 `GCN_Enemy_Telegraph` (cone do melee)

1. `VFX/Telegraph/` → **Blueprint Class** → busque parent **`GameplayCueNotify_Static`** → `GCN_Enemy_Telegraph`.
2. **Class Defaults → Gameplay Cue Tag = `GameplayCue.Enemy.Telegraph`** (a tag é nativa do `DDRTags`, aparece no dropdown).
3. My Blueprint → Functions → **Override → On Execute**, no grafo:
   - `My Target` → **Cast To DDREnemyCharacter**. Falhou → **Return false**.
   - → **Get Enemy Data** → **Telegraph** → **Break FDDRTelegraphConfig** (`WindupSeconds`, `ConeAngleDegrees`, `ConeRadius`, `Color`).
   - **SpawnActor `BP_TelegraphDecal`**:
     - Location = `My Target → GetActorLocation` (a caixa de 256 de fundo alcança o chão a partir do centro da cápsula).
     - Rotation = Make Rotator (Roll 0, Pitch 0, **Yaw = `My Target → GetActorRotation → Break → Yaw`**) — é isso que vira o cone pra frente do inimigo.
     - Expose on Spawn: `Duration = WindupSeconds` · `Radius = ConeRadius` · `ConeHalfAngleDeg = ConeAngleDegrees × 0.5` · `Color = Color` · `DecalMaterial = M_Telegraph_Cone`.
   - **Return true.**

> ⚙️ `GameplayCueNotify_Static` roda no CDO mas **pode spawnar actors**: na 5.4 o `GetWorld()` dele devolve o mundo cacheado do GameplayCueManager. Não precisa de `GameplayCueNotify_Actor` aqui — o `BP_TelegraphDecal` cuida da própria vida (timeline → DestroyActor).

### 8.2 `GCN_Proj_Telegraph` (linha do Atirador)

1. Mesmos passos → `GCN_Proj_Telegraph`, tag **`GameplayCue.Proj.Telegraph`**.
2. **On Execute**:
   - `Origem` = `My Target → GetActorLocation` · `Alvo` = `Parameters → Break → Location` (o C++ manda a posição do player).
   - **Find Look at Rotation** (Start = Origem, Target = Alvo) → Break → **Yaw**.
   - `Distance (Vector)` (Origem, Alvo) → **Comprimento**.
   - **SpawnActor `BP_TelegraphDecal` #1 — a linha:** Location = `(Origem + Alvo) / 2` · Rotation = (0, 0, Yaw) · `DecalMaterial = M_Telegraph_Line` · `Radius = Comprimento / 2` · `WidthOverride = 20` · `Duration = 0.55`.
   - **SpawnActor `BP_TelegraphDecal` #2 — zona de impacto:** Location = `Alvo` · Rotation = (0,0,0) · `DecalMaterial = M_Telegraph_Circle` · `Radius = 48` · `Duration = 0.55`.
   - **Return true.**

> ⚠️ O `Duration = 0.55` é o default do `TelegraphDuration` no `UDDRProjectileData`. Se mudar no DA, mude aqui também (melhoria M3+: o C++ passar a duração via `CueParameters.RawMagnitude`).

### 8.3 Não apareceu? (checagem de 30s)

- A tag do GCN é **exatamente** `GameplayCue.Enemy.Telegraph` / `GameplayCue.Proj.Telegraph`.
- O GCN BP está dentro de `Content/` (o GameplayCueManager escaneia `/Game` por default — o projeto não tem `GameplayCueNotifyPaths` custom).
- O DA do inimigo tem a ability e o BT a ativa ([61 §4–5](61_M3_Editor_Setup.md)).

---

## 9. Os números honestos no DataAsset

`DA_Enemy_Grunt_Melee` → **Combat → Telegraph**:

| Campo | Valor M3 | Regra de ouro |
|---|---|---|
| Windup Seconds | **0.45** | = a janela de leitura ([32 §2](../enemies/32_Enemy_Combat_Behavior.md)) |
| Cone Angle Degrees | **120** | = arco real do sweep frontal |
| Cone Radius | **180** | ⚠️ = `MeleeAttackRange` (180). **Decal menor que o golpe = morte "injusta"; maior = o player aprende a ignorar o decal.** |
| Color | (1, 0.08, 0.08, 1) | vermelho perigo |

No `DA_Projectile_ArcherBolt`: **Telegraph Duration = 0.55** · Telegraph Color vermelho ([37 §2](../combat/37_Projectiles.md)).

---

## 10. Teste PIE (o critério de leitura do M3)

| Teste | Esperado |
|---|---|
| Grunt aproxima e telegrafa | cone vermelho **na frente dele**, enchendo por 0.45s, pulsando; some junto do golpe |
| Ficar fora do cone | o golpe **erra** — o chão disse a verdade |
| Ficar dentro de propósito | dano — e você **sabia onde não estar** ([61 §9](61_M3_Editor_Setup.md)) |
| Archer em windup | linha archer→você + círculo de impacto por 0.55s |
| Juggle no grunt | sombra **fica no chão**, gap = altura, não some no topo |
| Sombra/telegrafe nos pés de alguém | sobrou mesh com Receives Decals ON (§3.3) |
| 2 grunts atacando | 2 cones independentes (o token limita a 2, [61 §8.5](61_M3_Editor_Setup.md)) |

---

## 11. Troubleshooting

| Sintoma | Causa | Fix |
|---|---|---|
| Blob **branco** / retângulo claro | gradiente no Base Color + Opacity 1.0 | §2 — a forma vai no **Opacity**; Base Color preto |
| Decal invisível no chão | rotação (0,0,0) — projetando pra frente | §3 — campo **Y (Pitch) = −90** |
| Blob/zona **gigante** | `DecalSize` é **meia**-extensão (160 = ø320uu) | §3 — (512, **45**, 45) |
| Sombra some no alto do juggle | Decal Size **X** (profundidade) curto | §3 — X = 512 |
| Decal pinta corpo/pés dos personagens | mesh com Receives Decals ON | §3.3 — OFF em todas as meshes de personagem |
| Nenhum decal aparece em nada | DBuffer Decals desligado no projeto | §1 — Project Settings → Rendering |
| Cone aponta pro lado errado | spawn sem o yaw do inimigo | §8.1 — Rotation Z = Yaw do MyTarget |
| Linha do archer atravessada/curta | sem `Find Look at Rotation` / Radius ≠ Comprimento/2 | §8.2 |
| Telegrafe não aparece | tag errada no GCN / ability não ativa | §8.3 |
| Telegrafe não cresce (sem fill) | timeline sem Set Play Rate / MID não salvo | §7.4 — rate = 1/Duration |
| Decal some com zoom out | Fade Screen Size | §3/§7 — 0 |
| Preview do material branco/estranho | preview não projeta decal | normal — julgue no level (§1.5) |
| Telegrafe fraco em chão claro | só opacity, sem emissive | §4 — `EmissiveBoost` 3+ |
| Zona não bate com o golpe real | `ConeRadius` ≠ alcance do sweep | §9 — radius = `MeleeAttackRange` |

---

## 12. Próximo passo

→ De volta ao **[61 — M3](61_M3_Editor_Setup.md)**: arena com ondas + token (§8) e o critério PIE (§9). O upgrade visual do telegrafe (Niagara `NS_Telegraph_Zone`, daltonismo via `User.Tint`) é polish P1: [36 §4](../feel/36_VFX_Niagara.md).
