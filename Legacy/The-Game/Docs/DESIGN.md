# Hollowreach design map

This document maps every element you asked for to the code that implements it, and names the games each idea
borrows from. Paths are relative to `Source/Hollowreach/`.

## The world

| Region | What it teaches | Systems exercised |
| --- | --- | --- |
| **Emberfall** (village, origin) | Shrines, talking, trading, the companion, the skiff | Checkpoints, dialogue, merchant economy, companion, vehicle, physics props |
| **Whispering Wood** (north) | Sneaking, gathering, the first Hollow | Stealth grass, awareness AI, resources, hidden collectibles, a bell clue |
| **Ashborn Encampment** (east) | Faction conflict, high ground, fire | Factions, ranged AI, explosive barrels, watchtower, key-carrying captain |
| **Sunken Ruins** (west) | Environmental puzzles | Dial + glyph puzzle, fire braziers, pressure plate, keyed vault, relics |
| **The Drowned Bell** (south-west) | Climbing and investigation | Spiral mantle climb, final clue, secret ending lore |
| **Skyreach Spire** (south-east) | Platforming and parkour | Mantling, moving & crumbling platforms, wall run, grapple, double jump |
| **Ashen Peak** (far south) | Cold, the boss, the ending | Survival temperature, shrine before boss, fog gate, 3-phase boss, altar |
| **Trial of Echoes** (far east) | Repeated challenges | Procedural rooms, run modifiers, mimics, lever puzzle, guardian, descent |

No quest markers point anywhere. The journal describes places; landmarks announce themselves when found.

## Gameplay elements

| Element | Where | Inspired by |
| --- | --- | --- |
| Open-world exploration, exploration without guidance, discovery | `Procedural/HRWorldBootstrap`, `World/HRDiscoverables` (landmarks), HUD text-only quest tracker | Elden Ring, Breath of the Wild-likes, TUNIC |
| Interconnected environments, nonlinear progression | Regions open in any order; traversal relics/skills open new routes (`Components/HRTraversalComponent`) | Dark Souls, Metroidvanias, Jedi: Fallen Order |
| Difficult combat, learning through failure, repeated challenges | `Components/HRCombatComponent`, boss hints on each death (`Characters/HRBossCharacter`), the Trial | Sekiro, Dark Souls, Binding of Isaac, Nightreign |
| Stealth | `Components/HRStealthComponent`, `AI/HRAIController` awareness meter, takedowns, tall grass | Assassin's Creed, Ghost of Tsushima, Metal Gear Solid |
| Survival | `Components/HRSurvivalComponent` (food, water, warmth, fatigue), weather & altitude cold | DayZ, Project Zomboid, 7 Days to Die |
| Investigation | `AHRClue` in `World/HRDiscoverables`, deductions in `Narrative/HRNarrativeSubsystem`, *The Drowned Bell* quest | The Sinking City, The Witcher 3, Alan Wake 2 |
| Puzzle-solving, environmental puzzles | `Puzzles/` (levers, dials, braziers, pressure plates, glyphs, ordered sequences) | TUNIC, God of War, Psychonauts 2 |
| Dungeon crawling, procedural generation | `Procedural/HRDungeonGenerator` | Binding of Isaac, Diablo, Demeo |
| Boss battles | `Characters/HRBossCharacter` (phases, arena gates, deathblows per phase) | Sekiro, Elden Ring, Black Myth: Wukong |
| Resource gathering, scavenging, looting | `World/HRResourceNode`, `World/HRLootContainer` (chests, loot bags, mimics), `Items/HRLootLibrary` | ARC Raiders, Terraria, Dark Souls |
| Crafting | `Components/HRCraftingComponent` + recipes in `Systems/HRBuiltinContent.cpp` | Minecraft, The Witcher 3, 7 Days to Die |
| Character development | `Components/HRProgressionComponent` (attributes, levels, skill tree) | Dark Souls, Baldur's Gate 3 |
| Quest completion | `Narrative/HRQuestSubsystem` (staged, branching quests) | The Witcher 3, Red Dead Redemption 2 |
| Environmental interaction, physics interactions | `World/HRPhysicsProp` (impact damage, chain explosions), Kinetic Push, fire reactions | Half-Life-likes, Human: Fall Flat, Jedi: Survivor |
| Platforming, parkour | `Components/HRTraversalComponent`, `World/HRMovingPlatform`, `World/HRGrapplePoint` | Assassin's Creed, Jedi: Survivor, Ninja Gaiden |
| Vehicle exploration | `Vehicles/HRHoverVehicle` (physics hover-skiff, ramming) | Star Wars Outlaws, No Man's Sky, Forza Horizon |
| Cooperative missions | Companion (`Characters/HRCompanionCharacter`, `AI/HRCompanionController`) and *What the Ruins Keep* | God of War, Baldur's Gate 3 |
| Branching narratives, player freedom | Dialogue conditions/consequences/skill checks, faction stances, 5 endings | Baldur's Gate 3, Fallout-likes, The Witcher |
| Experimentation | Systemic interactions: fire lights braziers and barrels; push props onto plates; brute-force or read the glyph | Breath of the Wild, Psychonauts 2 |

## Game mechanics

| Mechanic | Where |
| --- | --- |
| Skill trees | `Progression/HRSkillTypes.h`, skills in `Systems/HRBuiltinContent.cpp` (Blade, Shadow, Arcane, Survival) |
| Leveling, experience points | Echoes are XP and currency; spend at shrines (`HRProgressionComponent::LevelUpAttribute`) |
| Customizable builds | Six attributes (Vigor, Endurance, Strength, Dexterity, Arcane, Mind), weapon scaling, gear affixes |
| Weapon upgrades | `HRCraftingComponent::UpgradeItem` (+1..+10, Moonstone past +5) |
| Equipment customization, inventory management | `HRInventoryComponent` (slots, weight & encumbrance, quick item, flasks) |
| Stamina management | `HRAttributeComponent` (Souls overdraw rule, regen delay) |
| Posture breaking, staggering | Posture meter, stagger + poise thresholds, guard breaks |
| Parrying, blocking, counterattacking | Deflect window, attacker posture damage, riposte window (+50% damage, +60% posture) |
| Dodging | I-frames, perfect dodge triggers slow-motion |
| Stealth takedowns, enemy awareness | `HRCombatComponent::TryStealthTakedown`, `HRAIController` (Unaware > Suspicious > Searching > Alerted) |
| Magic, supernatural abilities | `Abilities/` (Firebolt, Kinetic Push, Blink, Ward, Shadow Veil, Spirit Sight) |
| Transformations | Beast Form (`UHRAbility_BeastForm`) |
| Companions | Kestrel: follow/hold/aggressive/passive, mirrors stealth, revives you, approval |
| Dialogue choices, reputation, factions | `HRNarrativeSubsystem` dialogue, `HRFactionSubsystem` (4 factions, ripple politics, price effects) |
| Multiple endings | `RegisterEndings` (Keeper, Ashen Throne, Hollow King, New Warden, Embers' End) |
| Hidden collectibles | Spirit-Sight-only caches, lore pages, relics, vigor seeds |
| Randomized loot | Weighted tables, luck-shifted rarity, rolled affixes (`HRLootLibrary`) |
| Checkpoints, resurrection | Shrines (`HRCheckpoint`); Sekiro-style resurrection charges refilled by kills or rest |
| Injuries | Deep wounds, broken arms, sprains, concussions, frostbite, burns; cured by bandages/splints/rest |
| Weather, day-night cycle | `HRTimeWeatherSubsystem` (Markov weather; light, sight, noise, temperature; blood moon) |
| Resource economy | Merchant buy/sell with reputation pricing; sold goods become stock |
| Rhythm-based timing | Flow stacks for on-beat combo inputs; deflect timing; mashing resets Flow |
| Tactical positioning | Backstab, high ground, flanking multipliers; attack tokens make enemies take turns |
| Combat combinations | Light combos per weapon, heavy finishers, cancel into dodge/deflect from recovery |
| Exploration abilities | Mantle, double jump, wall run, grapple, glide |
| Knowledge-based progression | Knowledge gates skills (Spirit Sight), recipes (alchemy), glyph puzzles, dialogue, the secret ending |
| Souls death loop | Drop echoes on death, reclaim them or lose them; enemies return when you rest |
| Blood moon hordes | Every 7th night (`HRGameMode::HandleNightfall`) |

## How the systems talk to each other

- **Combat** asks **Factions** who is hostile, asks **Attributes** to apply damage, and tells **Factions** when
  the player strikes someone who wasn't hostile (reputation loss).
- **AI** reads the player's **Stealth** visibility, **Time/Weather** sight range, and uses **Combat** attack tokens.
- **Narrative** is the shared memory: flags, knowledge and clues are written by world actors and read by quests,
  dialogue, skills, recipes, puzzles and endings. Saving the narrative state saves the world.
- **Content** is the data hub: items, recipes, loot, skills, quests, dialogue, lore, factions and endings.
  Everything is referenced by `FName` ids, so saves stay valid as content changes.

## Adding content

All built-in content lives in `Systems/HRBuiltinContent.cpp`. To add an item, quest, dialogue or ending, follow
the existing patterns there. Hand-placed actors in a real level work the same as the bootstrap's: give chests,
collectibles, clues and puzzles a unique `PersistentId` so their state saves.
