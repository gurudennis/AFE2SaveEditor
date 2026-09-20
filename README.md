# AFE2SaveEditor

AFE2SaveEditor is a save file manipulation utility for
[Aliens Fireteam Elite 2](https://store.steampowered.com/app/3448650/Aliens_Fireteam_Elite_2/) (the game).

AFE2SaveEditor is entirely free for any use. Our goal is to make your experience with Aliens Fireteam Elite 2
better, and perhaps compensate for some of the bugs with progression unlocks in the game. That said, you should
understand that the software is provided as-is, with no warranty of any kind, and you should exercise reasonable
caution when editing your save files. If anything should go wrong, AFE2SaveEditor makes an automatic backup
of the save state that you can restore as needed.

AFE2SaveEditor can unlock items that are grindy and often don't fully unlock due to game bugs:
- Weapon attachments;
- Weapon traits;
- Cosmetics.

Conversely, AFE2SaveEditor **does NOT** unlock items that become available through trivial progression:
- Game modes, campaign difficulties and store categories (beat the campaign on any difficulty);
- Weapons (to unlock all weapons, simply earn 1 star for each available weapon);

## Peace of mind: backing up your save manually

Although AFE2SaveEditor makes a backup of your save file, feel free to back it up manually as well. You'll find
your save under `%localappdata%\AFE2\Saved\SaveGames` (paste this into Explorer to open the folder). The save
file itself is `char.sav` and that is what you may want to copy somewhere safe before running AFE2SaveEditor.

## Running AFE2SaveEditor

Go to [AFE2SaveEditor Releases](https://github.com/gurudennis/AFE2SaveEditor/releases) and pick the most recent
one at the top.

Then click on the AFE2SaveEditor.exe download link, download and run it. If Windows pops up a warning that says
"Microsoft Defender SmartScreen prevented an unrecognized app from starting", click on "More info" and then "Run
anyway".

## How to use AFE2SaveEditor

The tool will tell you that it opened your save file, and present you with a menu of possible actions. To unlock
all known items (weapon attachments and cosmetics), press `2`. Once done, press `6` to exit.

If you're a power user, you can press `5` to go into the advanced menu that offers a few more options.

## Limitations

AFE2SaveEditor can only unlock items that are known to it. If you have a save file that contains items that are not
yet known to AFE2SaveEditor, please [create an issue](https://github.com/gurudennis/JumpSaves/issues/new) and
attach the save file. The content that you unlocked will become available in the next release of AFE2SaveEditor.
