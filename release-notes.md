# pico-gamate 3.1.0 --- Release Notes

Version 3.1.0 adds randomized custom palettes and improves the interaction
between quick save/load and Demo mode.

## Highlights

-   Added a randomized custom-palette mode using the expanded Gamate palette
    definitions.
-   Random palette generation chooses the lightest and darkest colors
    independently and pairs the two middle colors across cold/warm groups.
-   Expanded and corrected the preset color tables used by random palettes.
-   Quick save/load now exits Demo mode before accessing a state slot,
    preventing an automatic cartridge change from invalidating the saved or
    restored state.

## Random custom palettes

The custom palette can now be randomized from the current Gamate palette
set.

The four palette entries are selected as follows:

-   RGB0 is selected randomly from the complete RGB0 set.
-   RGB1 is selected randomly from the complete RGB1 set.
-   RGB2 is selected from the opposite temperature group: a cold RGB1 is
    paired with a warm RGB2, while a warm RGB1 is paired with a cold RGB2.
-   RGB3 is selected randomly from the complete RGB3 set.

This keeps the generated palettes varied while avoiding cold/cold and
warm/warm combinations for the two middle colors.

The preset tables were also synchronized with the updated Gamate palette
definitions, including the additional RGB1/RGB2 choices and corrected RGB0
values.

## Quick states and Demo mode

Using a quick-state hotkey now terminates the active Demo session before the
save or load operation is performed.

This is important for quick saves made during Demo mode. Previously a state
could be saved for one cartridge, Demo mode could automatically advance to a
different cartridge, and a later quick load could then restore the old state
on top of the wrong game.

Quick save/load now clears the Demo state and stops the pending automatic
advance before accessing the state slot. The currently running cartridge
therefore remains active after a quick save, and Demo mode cannot continue
switching cartridges after a quick load.
