# Dark Dungeon Rift

ARPG **top-down** roguelike de hack'n'slash com **combos aéreos** — construído em **Unreal Engine 5.4** com **Gameplay Ability System (GAS)**.

> *"Hades encontra Devil May Cry visto de cima":* desça numa fenda que muda a cada run, lance inimigos pro ar, encadeie combos e morra pra ficar mais forte.

## Status

🚧 **Pré-MVP** — projeto base Third Person + documentação de design/implementação. O código de gameplay (GAS, combo aéreo, IA) está em construção conforme o [roadmap](docs/17_Implementation_Roadmap.md).

## Pilares

| # | Pilar |
|---|---|
| **P1** | Combate expressivo (combo de chão + aéreo) |
| **P2** | Run sempre diferente (roguelike) |
| **P3** | Morte com propósito (meta-progressão) |
| **P4** | Leitura clara em top-down (telegrafe, área de perigo) |

## Requisitos

- **Unreal Engine 5.4**
- **Visual Studio 2022** (toolchain C++ para Windows)
- Windows 10/11 (plataforma de desenvolvimento atual)

## Como abrir o projeto

1. Clone o repositório.
2. Clique duas vezes em `DarkDungeonRift.uproject` ou abra via Epic Games Launcher.
3. Se pedir, permita a recompilação dos módulos C++.
4. Gere os project files se necessário: botão direito no `.uproject` → *Generate Visual Studio project files*.

## Estrutura

```
DarkDungeonRift/
├── Config/          # DefaultEngine, Input, Game
├── Content/         # Assets do jogo (packs da Marketplace no .gitignore)
├── Source/          # C++ (módulo DarkDungeonRift)
├── docs/            # Documentação completa do projeto
├── DarkDungeonRift.uproject
└── README.md
```

## Documentação

Toda a documentação está em [`docs/`](docs/00_Index.md). Comece pelo [índice](docs/00_Index.md).

| Área | Destaques |
|---|---|
| Design | [Visão](docs/design/01_Game_Vision.md) · [MVP](docs/design/02_MVP_Scope.md) · [Core loop](docs/design/03_Core_Loop_Roguelike.md) |
| Sistemas | [GAS](docs/systems/05_GAS_Architecture.md) · [Câmera](docs/systems/06_Camera_TopDown.md) · [Input](docs/systems/07_Input.md) |
| Combate | [Overview](docs/combat/15_Combat_Overview.md) · [Combos aéreos](docs/combat/16_Aerial_Combos.md) · [Projéteis](docs/combat/37_Projectiles.md) |
| Execução | [Roadmap](docs/17_Implementation_Roadmap.md) |

## Assets de terceiros

Packs da Unreal Marketplace e conteúdo de template (`StarterContent`, `ThirdPerson`, etc.) **não estão no repositório** — listados no `.gitignore`. Cada desenvolvedor precisa possuir/licenciar os assets localmente.

## Licença

Código e documentação originais deste repositório: uso privado do autor. Assets de marketplace e engine permanecem sob suas respectivas licenças (Epic / publishers).
