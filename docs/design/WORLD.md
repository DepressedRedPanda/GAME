# Heavy Mettle — the blended world

This is the single source of truth for the setting. It merges the two earlier games:

- **Heavy Mettle** (attempt-two, Heavy-Mettle repos): an amnesiac android doing mercenary jobs on a dusty alien frontier. Spaghetti western in space.
- **Veil of Ash: Hollowreach** (The-Game repo): a dark-fantasy action RPG with ember shrines, a memory reliquary, two guardian bosses, factions, stealth, survival and five endings.

Rule for the blend: **everything that looks like magic is real, but nobody alive understands it.** The frontier folk call it "the Veil." The android is the only one who can read it, and doesn't know why.

## The premise in one paragraph

A human-looking android wakes in the red salt flats with no memory. He drifts into **Cinder's Rest**, a market town of human/alien hybrids, and takes small-time mercenary jobs: missing power cells, a debt collector gone quiet, a medic short on supplies. Under the frontier lie the ruins of a vanished civilization — the **Hollow** — whose machines still run on **embers**: palm-sized cores of living light. Bandits strip embers for power; preachers worship them; the android's own chest holds one. Every ember he touches returns a fragment of memory (an **Echo**). The deeper he digs, the clearer it gets that he was built by the Hollow, and that the two ancient guardians in the mountains — the **Pale Keeper** and the **Hollow Warden** — have been waiting for him.

## Tone

Western first, mystery second. Long quiet walks, petty debts, bad trades, sudden violence, moral choices with local consequences. The ancient layer is felt at the edges (strange ruins, ember-lit shrines, one villager who won't stop humming the same tune) and only becomes the main event in the back half. No wizards, no mana bars, no gods — the "magic" is old tech that looks like magic.

## Name mapping (Hollowreach → Heavy Mettle)

The C++ code keeps its internal IDs (`Quest.Embers`, `Item.IronBlade` …) because saves reference them. Only **display names, text and art** change. When you rename something, change the display text in `Source/Hollowreach/Systems/HRBuiltinContent.cpp` or a Blueprint/DataTable — never the ID.

| In the code (Hollowreach) | In the game (Heavy Mettle) |
| --- | --- |
| Emberfall (starting village) | **Cinder's Rest** — the market town |
| Elder Maren | **Tessa Rhun**, tavern keeper who "knows everyone and sells silence" |
| Ember Hearth / shrines (checkpoints) | **Ember beacons** — rest, save, respawn, level up |
| Echoes (XP + currency, dropped on death) | **Echoes** — recovered memory fragments; plus **chits** as cash |
| Whispering Wood | **The Stalk Fields** — tall alien grass, stealth zone |
| Ashborn Encampment | **Silt Jack's camp** — outlaws sheltering displaced miners |
| Sunken Ruins | **The Old Relay** — buried Hollow machinery, environmental puzzles |
| The Drowned Bell | **The Drowned Bell** (kept) — a flooded Hollow mine shaft |
| Skyreach Spire | **The Spire** — a broken antenna to climb; parkour |
| Memory Reliquary | **The Memory Reliquary** (kept) — where the android's past is stored |
| Ashen Peak | **Ashen Peak** (kept) — cold, ashfall, the final guardian |
| Trial of Echoes (procedural dungeon) | **The Trial** — a shifting Hollow vault, roguelite runs |
| Pale Keeper (boss 1) | **The Pale Keeper** — the Reliquary's guardian machine |
| Hollow Warden (boss 2) | **The Hollow Warden** — the last Hollow war-frame |
| Kestrel (companion) | **Kestrel** — a hybrid tracker and scout who decides you're worth following |
| Hover skiff | **Dust skiff** — hover bike for the salt flats |
| Firebolt / Kinetic Push / Blink / Ward / Shadow Veil / Spirit Sight | Ember-tech: **Arc Shot / Shove Pulse / Phase Step / Barrier / Cloak / Echo Sight** — powered by **Battery** |
| Void Pulse / Spectral Form (Veil abilities) | **Overload Pulse / Ghost Mode** |
| Beast Form (transformation) | **Overclock** — the android drops its limiters |
| Focus | **Battery** (from Heavy Mettle's design) |
| Iron Blade + scaling weapons | **The sword on his back**; plus the **pistol on his thigh** |
| Infection / ashfall exposure | **Ember sickness** from raw ember dust |
| Factions (4 + Freefolk/Choir/Unbound) | Townsfolk, Silt Jack's outlaws, the **Choir** (ember preachers), the **Freefolk** (miners' union), the **Unbound** (machines that woke up) |

## The player

From Heavy Mettle's creative direction, unchanged: humanlike male-presenting android, looks about nineteen, long weathered duster with Chaos Cloth, sword across the back, sidearm at the right thigh. Health = structural damage, Stamina = exertion, **Battery** = ember-tech abilities.

Combat merges both games:
- **Shooting** (Heavy Mettle): shoulder-camera free aim, pistol, ammo you craft.
- **Melee** (Hollowreach C++, already written): light/heavy combos, posture breaks, deflect/parry, dodge with i-frames, riposte, stealth takedowns.
- **Lock-on** within 3.5 m for melee; free-aim beyond that.

## Story spine (one campaign)

1. **Arrive at Cinder's Rest.** Light the town's ember beacon. Meet Tessa. *(Quest.Embers stage 1–2)*
2. **Small jobs** — Heavy Mettle's quest list is the opening act:
   - *The Missing Cells* (Oren Bale) — first full quest, teaches shooting, sneaking, parley.
   - *A Thimble of Mercy* (Nima Voss) — crafting and scavenging.
   - *Nine Bells and a Body* (Tessa Rhun) — investigation; uses the C++ clue/deduction system.
3. **Side threads open** — Silt Jack's pact (*Quest.AshbornPact*), Kestrel's ruins (*Quest.KestrelRuins*), the Drowned Bell, the Trial.
4. **The Memory Reliquary.** Open the gate with 3 cipher seals + 2 lost records, or solve the rune order. Fight the **Pale Keeper**. The android remembers what he is.
5. **Ashen Peak.** Climb, survive the cold, defeat the **Hollow Warden**.
6. **The Altar.** Choose an ending — Hollowreach's five, plus the Freefolk, Choir and Unbound outcomes. Faction reputation decides what's on offer.

## Scope discipline (still applies)

Heavy Mettle's rule wins: **the first playable is 15 minutes** — Cinder's Rest, the Old Relay, three NPCs, two enemy types, one pistol, one sword, one crafting bench, *The Missing Cells*, one save slot. Everything else already exists in C++ and gets switched on one region at a time after that loop is fun.
