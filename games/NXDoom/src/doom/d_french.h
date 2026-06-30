/****************************************************************************
 * apps/games/NXDoom/src/doom/d_french.h
 *
 * SPDX-License-Identifier: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *  Printed strings for translation.
 *  French language support.
 *  TODO: finish implementation
 *  WARN: Non-ascii text does not render properly. Find a solution or cope.
 *
 ****************************************************************************/

#ifndef __D_FRENCH__
#define __D_FRENCH__

/* Printed strings for translation */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* D_Main.C */

#define D_DEVSTR "Mode développement activé.\n"
#define D_CDROM "Version CD-ROM: default.cfg de c:\\doomdata\n"

/* M_Menu.C */

#define PRESSKEY "appuie une touche."
#define PRESSYN "appuie y ou n."
#define QUITMSG "es-tu certain que tu veux quitter ce super jeu ?"
#define LOADNET "tu ne peux pas charger pendant un jeu en ligne!\n\n" PRESSKEY
#define QLOADNET                                                             \
  "tu ne peux pas charger rapidement pendant un jeu en ligne!\n\n" PRESSKEY
#define QSAVESPOT "tu n'as pas choisi un slot sauvegarde rapide\n\n" PRESSKEY
#define SAVEDEAD "tu ne peux pas sauvegarder si tu ne joue pas\n\n" PRESSKEY
#define QSPROMPT "sauvegarde ton jeu nommé\n\n'%s'?\n\n" PRESSYN
#define QLPROMPT                                                             \
  "tu veux charger rapidement le jeu nommé\n\n'%s'?\n\n" PRESSYN

#define NEWGAME                                                              \
  "tu ne peux pas commencer un nouveau jeu\n"                                \
  "durant un jeu en ligne.\n\n" PRESSKEY

#define NIGHTMARE                                                            \
  "es-tu sure? ce niveau\n"                                                  \
  "n'est absolument pas juste.\n\n" PRESSYN

#define SWSTRING                                                             \
  "ceci est le version shareware de doom.\n\n"                               \
  "tu besoin de commander la trilogie entiere.\n\n" PRESSKEY

#define MSGOFF "Messages DÉSACTIVÉS"
#define MSGON "Messages ACTIVÉS"
#define NETEND "tu ne peux pas quitter un jeu en ligne!\n\n" PRESSKEY
#define ENDGAME "es-tu sure que tu veux quitter le jeu?\n\n" PRESSYN

#define DOSY "(appuie y pour quitter vers dos.)"

#define DETAILHI "Haute définition"
#define DETAILLO "Faible définition"
#define GAMMALVL0 "Correction gamma DÉSACTIVÉ"
#define GAMMALVL1 "Correction gamma niveau 1"
#define GAMMALVL2 "Correction gamma niveau 2"
#define GAMMALVL3 "Correction gamma niveau 3"
#define GAMMALVL4 "Correction gamma niveau 4"
#define EMPTYSTRING "slot vide"

/* P_inter.C */

#define GOTARMOR "Ramassé l'armure."
#define GOTMEGA "Ramassé le Mega Armure!"
#define GOTHTHBONUS "Ramassé un bonus de santé."
#define GOTARMBONUS "Ramassé un bonus d'armure."
#define GOTSTIM "Ramassé un stimpack."
#define GOTMEDINEED "Ramassé un medikit que tu as VRAIMENT besoin!"
#define GOTMEDIKIT "Ramassé un medikit."
#define GOTSUPER "Superchargé!"

#define GOTBLUECARD "Ramassé une carte d'accès bleue."
#define GOTYELWCARD "Ramassé une carte d'accès jaune."
#define GOTREDCARD "Ramassé une carte d'accès rouge."
#define GOTBLUESKUL "Ramassé une clé crâne bleue."
#define GOTYELWSKUL "Ramassé une clé crâne jaune."
#define GOTREDSKULL "Ramassé une clé crâne rouge."

#define GOTINVUL "Invulnérabilité!"
#define GOTBERSERK "Berserk!"
#define GOTINVIS "Invisibilité Partielle"
#define GOTSUIT "Radiation Shielding Suit" // TODO?
#define GOTMAP "Carte de la Zone Informatique"
#define GOTVISOR "Light Amplification Visor" // TODO?
#define GOTMSPHERE "MegaSphere!"

#define GOTCLIP "Ramassé un clip."
#define GOTCLIPBOX "Ramassé une boîte de balles."
#define GOTROCKET "Ramassé une fusée."
#define GOTROCKBOX "Ramassé une boîte de fusées."
#define GOTCELL "Ramassé une cellule énergétique."
#define GOTCELLBOX "Ramassé un paquet de cellules énergétiques."
#define GOTSHELLS "Ramassé 4 cartouches de fusil de chasse."
#define GOTSHELLBOX "Ramassé une boîte de cartouches de fusil de chasse."
#define GOTBACKPACK "Ramassé une sac à dos plein d'ammo!"

#define GOTBFG9000 "Tu as eu le BFG9000! Hohoho, oui!"
#define GOTCHAINGUN "Tu as eu le chaingun!"
#define GOTCHAINSAW "Un scie à chaîne! Trouve de la viande!"
#define GOTLAUNCHER "Tu as eu la lance-roquettes!"
#define GOTPLASMA "Tu as eu le pistolet à plasma!"
#define GOTSHOTGUN "Tu as eu le fusil de chasse!"
#define GOTSHOTGUN2 "Tu as eu le super fusil de chasse!"

/* P_Doors.C */

#define PD_BLUEO "You need a blue key to activate this object"
#define PD_REDO "You need a red key to activate this object"
#define PD_YELLOWO "You need a yellow key to activate this object"
#define PD_BLUEK "You need a blue key to open this door"
#define PD_REDK "You need a red key to open this door"
#define PD_YELLOWK "You need a yellow key to open this door"

/* G_game.C */

#define GGSAVED "game saved."

/* HU_stuff.C */

#define HUSTR_MSGU "[Message unsent]"

#define HUSTR_E1M1 "E1M1: Hangar"
#define HUSTR_E1M2 "E1M2: Nuclear Plant"
#define HUSTR_E1M3 "E1M3: Toxin Refinery"
#define HUSTR_E1M4 "E1M4: Command Control"
#define HUSTR_E1M5 "E1M5: Phobos Lab"
#define HUSTR_E1M6 "E1M6: Central Processing"
#define HUSTR_E1M7 "E1M7: Computer Station"
#define HUSTR_E1M8 "E1M8: Phobos Anomaly"
#define HUSTR_E1M9 "E1M9: Military Base"

#define HUSTR_E2M1 "E2M1: Deimos Anomaly"
#define HUSTR_E2M2 "E2M2: Containment Area"
#define HUSTR_E2M3 "E2M3: Refinery"
#define HUSTR_E2M4 "E2M4: Deimos Lab"
#define HUSTR_E2M5 "E2M5: Command Center"
#define HUSTR_E2M6 "E2M6: Halls of the Damned"
#define HUSTR_E2M7 "E2M7: Spawning Vats"
#define HUSTR_E2M8 "E2M8: Tower of Babel"
#define HUSTR_E2M9 "E2M9: Fortress of Mystery"

#define HUSTR_E3M1 "E3M1: Hell Keep"
#define HUSTR_E3M2 "E3M2: Slough of Despair"
#define HUSTR_E3M3 "E3M3: Pandemonium"
#define HUSTR_E3M4 "E3M4: House of Pain"
#define HUSTR_E3M5 "E3M5: Unholy Cathedral"
#define HUSTR_E3M6 "E3M6: Mt. Erebus"
#define HUSTR_E3M7 "E3M7: Limbo"
#define HUSTR_E3M8 "E3M8: Dis"
#define HUSTR_E3M9 "E3M9: Warrens"

#define HUSTR_E4M1 "E4M1: Hell Beneath"
#define HUSTR_E4M2 "E4M2: Perfect Hatred"
#define HUSTR_E4M3 "E4M3: Sever The Wicked"
#define HUSTR_E4M4 "E4M4: Unruly Evil"
#define HUSTR_E4M5 "E4M5: They Will Repent"
#define HUSTR_E4M6 "E4M6: Against Thee Wickedly"
#define HUSTR_E4M7 "E4M7: And Hell Followed"
#define HUSTR_E4M8 "E4M8: Unto The Cruel"
#define HUSTR_E4M9 "E4M9: Fear"

#define HUSTR_1 "niveau 1: entryway"
#define HUSTR_2 "niveau 2: underhalls"
#define HUSTR_3 "niveau 3: the gantlet"
#define HUSTR_4 "niveau 4: the focus"
#define HUSTR_5 "niveau 5: the waste tunnels"
#define HUSTR_6 "niveau 6: the crusher"
#define HUSTR_7 "niveau 7: dead simple"
#define HUSTR_8 "niveau 8: tricks and traps"
#define HUSTR_9 "niveau 9: the pit"
#define HUSTR_10 "niveau 10: refueling base"
#define HUSTR_11 "niveau 11: 'o' of destruction!"

#define HUSTR_12 "niveau 12: the factory"
#define HUSTR_13 "niveau 13: downtown"
#define HUSTR_14 "niveau 14: the inmost dens"
#define HUSTR_15 "niveau 15: industrial zone"
#define HUSTR_16 "niveau 16: suburbs"
#define HUSTR_17 "niveau 17: tenements"
#define HUSTR_18 "niveau 18: the courtyard"
#define HUSTR_19 "niveau 19: the citadel"
#define HUSTR_20 "niveau 20: gotcha!"

#define HUSTR_21 "niveau 21: nirvana"
#define HUSTR_22 "niveau 22: the catacombs"
#define HUSTR_23 "niveau 23: barrels o' fun"
#define HUSTR_24 "niveau 24: the chasm"
#define HUSTR_25 "niveau 25: bloodfalls"
#define HUSTR_26 "niveau 26: the abandoned mines"
#define HUSTR_27 "niveau 27: monster condo"
#define HUSTR_28 "niveau 28: the spirit world"
#define HUSTR_29 "niveau 29: the living end"
#define HUSTR_30 "niveau 30: icon of sin"

#define HUSTR_31 "niveau 31: wolfenstein"
#define HUSTR_32 "niveau 32: grosse"

#define PHUSTR_1 "niveau 1: congo"
#define PHUSTR_2 "niveau 2: well of souls"
#define PHUSTR_3 "niveau 3: aztec"
#define PHUSTR_4 "niveau 4: caged"
#define PHUSTR_5 "niveau 5: ghost town"
#define PHUSTR_6 "niveau 6: baron's lair"
#define PHUSTR_7 "niveau 7: caughtyard"
#define PHUSTR_8 "niveau 8: realm"
#define PHUSTR_9 "niveau 9: abattoire"
#define PHUSTR_10 "niveau 10: onslaught"
#define PHUSTR_11 "niveau 11: hunted"

#define PHUSTR_12 "niveau 12: speed"
#define PHUSTR_13 "niveau 13: the crypt"
#define PHUSTR_14 "niveau 14: genesis"
#define PHUSTR_15 "niveau 15: the twilight"
#define PHUSTR_16 "niveau 16: the omen"
#define PHUSTR_17 "niveau 17: compound"
#define PHUSTR_18 "niveau 18: neurosphere"
#define PHUSTR_19 "niveau 19: nme"
#define PHUSTR_20 "niveau 20: the death domain"

#define PHUSTR_21 "niveau 21: slayer"
#define PHUSTR_22 "niveau 22: impossible mission"
#define PHUSTR_23 "niveau 23: tombstone"
#define PHUSTR_24 "niveau 24: the final frontier"
#define PHUSTR_25 "niveau 25: the temple of darkness"
#define PHUSTR_26 "niveau 26: bunker"
#define PHUSTR_27 "niveau 27: anti-christ"
#define PHUSTR_28 "niveau 28: the sewers"
#define PHUSTR_29 "niveau 29: odyssey of noises"
#define PHUSTR_30 "niveau 30: the gateway of hell"

#define PHUSTR_31 "niveau 31: cyberden"
#define PHUSTR_32 "niveau 32: go 2 it"

#define THUSTR_1 "niveau 1: system control"
#define THUSTR_2 "niveau 2: human bbq"
#define THUSTR_3 "niveau 3: power control"
#define THUSTR_4 "niveau 4: wormhole"
#define THUSTR_5 "niveau 5: hanger"
#define THUSTR_6 "niveau 6: open season"
#define THUSTR_7 "niveau 7: prison"
#define THUSTR_8 "niveau 8: metal"
#define THUSTR_9 "niveau 9: stronghold"
#define THUSTR_10 "niveau 10: redemption"
#define THUSTR_11 "niveau 11: storage facility"

#define THUSTR_12 "niveau 12: crater"
#define THUSTR_13 "niveau 13: nukage processing"
#define THUSTR_14 "niveau 14: steel works"
#define THUSTR_15 "niveau 15: dead zone"
#define THUSTR_16 "niveau 16: deepest reaches"
#define THUSTR_17 "niveau 17: processing area"
#define THUSTR_18 "niveau 18: mill"
#define THUSTR_19 "niveau 19: shipping/respawning"
#define THUSTR_20 "niveau 20: central processing"

#define THUSTR_21 "niveau 21: administration center"
#define THUSTR_22 "niveau 22: habitat"
#define THUSTR_23 "niveau 23: lunar mining project"
#define THUSTR_24 "niveau 24: quarry"
#define THUSTR_25 "niveau 25: baron's den"
#define THUSTR_26 "niveau 26: ballistyx"
#define THUSTR_27 "niveau 27: mount pain"
#define THUSTR_28 "niveau 28: heck"
#define THUSTR_29 "niveau 29: river styx"
#define THUSTR_30 "niveau 30: last call"

#define THUSTR_31 "niveau 31: pharaoh"
#define THUSTR_32 "niveau 32: caribbean"

#define HUSTR_CHATMACRO1 "I'm ready to kick butt!"
#define HUSTR_CHATMACRO2 "I'm OK."
#define HUSTR_CHATMACRO3 "I'm not looking too good!"
#define HUSTR_CHATMACRO4 "Help!"
#define HUSTR_CHATMACRO5 "You suck!"
#define HUSTR_CHATMACRO6 "Next time, scumbag..."
#define HUSTR_CHATMACRO7 "Come here!"
#define HUSTR_CHATMACRO8 "I'll take care of it."
#define HUSTR_CHATMACRO9 "Oui"
#define HUSTR_CHATMACRO0 "Non"

#define HUSTR_TALKTOSELF1 "You mumble to yourself"
#define HUSTR_TALKTOSELF2 "Who's there?"
#define HUSTR_TALKTOSELF3 "You scare yourself"
#define HUSTR_TALKTOSELF4 "You start to rave"
#define HUSTR_TALKTOSELF5 "You've lost it..."

#define HUSTR_MESSAGESENT "[Message Sent]"

/* The following should NOT be changed unless it seems just AWFULLY
 * necessary
 */

#define HUSTR_PLRGREEN "Vert: "
#define HUSTR_PLRINDIGO "Indigo: "
#define HUSTR_PLRBROWN "Brun: "
#define HUSTR_PLRRED "Rouge: "

#define HUSTR_KEYGREEN 'g'
#define HUSTR_KEYINDIGO 'i'
#define HUSTR_KEYBROWN 'b'
#define HUSTR_KEYRED 'r'

/* AM_map.C */

#define AMSTR_FOLLOWON "Follow Mode ON"
#define AMSTR_FOLLOWOFF "Follow Mode OFF"

#define AMSTR_GRIDON "Grid ON"
#define AMSTR_GRIDOFF "Grid OFF"

#define AMSTR_MARKEDSPOT "Marked Spot"
#define AMSTR_MARKSCLEARED "All Marks Cleared"

/* ST_stuff.C */

#define STSTR_MUS "Music Change"
#define STSTR_NOMUS "IMPOSSIBLE SELECTION"
#define STSTR_DQDON "Degreelessness Mode On"
#define STSTR_DQDOFF "Degreelessness Mode Off"

#define STSTR_KFAADDED "Very Happy Ammo Added"
#define STSTR_FAADDED "Ammo (no keys) Added"

#define STSTR_NCON "No Clipping Mode ON"
#define STSTR_NCOFF "No Clipping Mode OFF"

#define STSTR_BEHOLD "inVuln, Str, Inviso, Rad, Allmap, or Lite-amp"
#define STSTR_BEHOLDX "Power-up Toggled"

#define STSTR_CHOPPERS "... doesn't suck - GM"
#define STSTR_CLEV "Changing Level..."

/* F_Finale.C */

#define E1TEXT                                                               \
  "Once you beat the big badasses and\n"                                     \
  "clean out the moon base you're supposed\n"                                \
  "to win, aren't you? Aren't you? Where's\n"                                \
  "your fat reward and ticket home? What\n"                                  \
  "the hell is this? It's not supposed to\n"                                 \
  "end this way!\n"                                                          \
  "\n"                                                                       \
  "It stinks like rotten meat, but looks\n"                                  \
  "like the lost Deimos base.  Looks like\n"                                 \
  "you're stuck on The Shores of Hell.\n"                                    \
  "The only way out is through.\n"                                           \
  "\n"                                                                       \
  "To continue the DOOM experience, play\n"                                  \
  "The Shores of Hell and its amazing\n"                                     \
  "sequel, Inferno!\n"

#define E2TEXT                                                               \
  "You've done it! The hideous cyber-\n"                                     \
  "demon lord that ruled the lost Deimos\n"                                  \
  "moon base has been slain and you\n"                                       \
  "are triumphant! But ... where are\n"                                      \
  "you? You clamber to the edge of the\n"                                    \
  "moon and look down to see the awful\n"                                    \
  "truth.\n"                                                                 \
  "\n"                                                                       \
  "Deimos floats above Hell itself!\n"                                       \
  "You've never heard of anyone escaping\n"                                  \
  "from Hell, but you'll make the bastards\n"                                \
  "sorry they ever heard of you! Quickly,\n"                                 \
  "you rappel down to  the surface of\n"                                     \
  "Hell.\n"                                                                  \
  "\n"                                                                       \
  "Now, it's on to the final chapter of\n"                                   \
  "DOOM! -- Inferno."

#define E3TEXT                                                               \
  "The loathsome spiderdemon that\n"                                         \
  "masterminded the invasion of the moon\n"                                  \
  "bases and caused so much death has had\n"                                 \
  "its ass kicked for all time.\n"                                           \
  "\n"                                                                       \
  "A hidden doorway opens and you enter.\n"                                  \
  "You've proven too tough for Hell to\n"                                    \
  "contain, and now Hell at last plays\n"                                    \
  "fair -- for you emerge from the door\n"                                   \
  "to see the green fields of Earth!\n"                                      \
  "Home at last.\n"                                                          \
  "\n"                                                                       \
  "You wonder what's been happening on\n"                                    \
  "Earth while you were battling evil\n"                                     \
  "unleashed. It's good that no Hell-\n"                                     \
  "spawn could have come through that\n"                                     \
  "door with you ..."

#define E4TEXT                                                               \
  "the spider mastermind must have sent forth\n"                             \
  "its legions of hellspawn before your\n"                                   \
  "final confrontation with that terrible\n"                                 \
  "beast from hell.  but you stepped forward\n"                              \
  "and brought forth eternal damnation and\n"                                \
  "suffering upon the horde as a true hero\n"                                \
  "would in the face of something so evil.\n"                                \
  "\n"                                                                       \
  "besides, someone was gonna pay for what\n"                                \
  "happened to daisy, your pet rabbit.\n"                                    \
  "\n"                                                                       \
  "but now, you see spread before you more\n"                                \
  "potential pain and gibbitude as a nation\n"                               \
  "of demons run amok among our cities.\n"                                   \
  "\n"                                                                       \
  "next stop, hell on earth!"

/* after niveau 6, put this: */

#define C1TEXT                                                               \
  "YOU HAVE ENTERED DEEPLY INTO THE INFESTED\n"                              \
  "STARPORT. BUT SOMETHING IS WRONG. THE\n"                                  \
  "MONSTERS HAVE BROUGHT THEIR OWN REALITY\n"                                \
  "WITH THEM, AND THE STARPORT'S TECHNOLOGY\n"                               \
  "IS BEING SUBVERTED BY THEIR PRESENCE.\n"                                  \
  "\n"                                                                       \
  "AHEAD, YOU SEE AN OUTPOST OF HELL, A\n"                                   \
  "FORTIFIED ZONE. IF YOU CAN GET PAST IT,\n"                                \
  "YOU CAN PENETRATE INTO THE HAUNTED HEART\n"                               \
  "OF THE STARBASE AND FIND THE CONTROLLING\n"                               \
  "SWITCH WHICH HOLDS EARTH'S POPULATION\n"                                  \
  "HOSTAGE."

/* After niveau 11, put this: */

#define C2TEXT                                                               \
  "YOU HAVE WON! YOUR VICTORY HAS ENABLED\n"                                 \
  "HUMANKIND TO EVACUATE EARTH AND ESCAPE\n"                                 \
  "THE NIGHTMARE.  NOW YOU ARE THE ONLY\n"                                   \
  "HUMAN LEFT ON THE FACE OF THE PLANET.\n"                                  \
  "CANNIBAL MUTATIONS, CARNIVOROUS ALIENS,\n"                                \
  "AND EVIL SPIRITS ARE YOUR ONLY NEIGHBORS.\n"                              \
  "YOU SIT BACK AND WAIT FOR DEATH, CONTENT\n"                               \
  "THAT YOU HAVE SAVED YOUR SPECIES.\n"                                      \
  "\n"                                                                       \
  "BUT THEN, EARTH CONTROL BEAMS DOWN A\n"                                   \
  "MESSAGE FROM SPACE: \"SENSORS HAVE LOCATED\n"                             \
  "THE SOURCE OF THE ALIEN INVASION. IF YOU\n"                               \
  "GO THERE, YOU MAY BE ABLE TO BLOCK THEIR\n"                               \
  "ENTRY.  THE ALIEN BASE IS IN THE HEART OF\n"                              \
  "YOUR OWN HOME CITY, NOT FAR FROM THE\n"                                   \
  "STARPORT.\" SLOWLY AND PAINFULLY YOU GET\n"                               \
  "UP AND RETURN TO THE FRAY."

/* After niveau 20, put this: */

#define C3TEXT                                                               \
  "YOU ARE AT THE CORRUPT HEART OF THE CITY,\n"                              \
  "SURROUNDED BY THE CORPSES OF YOUR ENEMIES.\n"                             \
  "YOU SEE NO WAY TO DESTROY THE CREATURES'\n"                               \
  "ENTRYWAY ON THIS SIDE, SO YOU CLENCH YOUR\n"                              \
  "TEETH AND PLUNGE THROUGH IT.\n"                                           \
  "\n"                                                                       \
  "THERE MUST BE A WAY TO CLOSE IT ON THE\n"                                 \
  "OTHER SIDE. WHAT DO YOU CARE IF YOU'VE\n"                                 \
  "GOT TO GO THROUGH HELL TO GET TO IT?"

/* After niveau 29, put this: */

#define C4TEXT                                                               \
  "THE HORRENDOUS VISAGE OF THE BIGGEST\n"                                   \
  "DEMON YOU'VE EVER SEEN CRUMBLES BEFORE\n"                                 \
  "YOU, AFTER YOU PUMP YOUR ROCKETS INTO\n"                                  \
  "HIS EXPOSED BRAIN. THE MONSTER SHRIVELS\n"                                \
  "UP AND DIES, ITS THRASHING LIMBS\n"                                       \
  "DEVASTATING UNTOLD MILES OF HELL'S\n"                                     \
  "SURFACE.\n"                                                               \
  "\n"                                                                       \
  "YOU'VE DONE IT. THE INVASION IS OVER.\n"                                  \
  "EARTH IS SAVED. HELL IS A WRECK. YOU\n"                                   \
  "WONDER WHERE BAD FOLKS WILL GO WHEN THEY\n"                               \
  "DIE, NOW. WIPING THE SWEAT FROM YOUR\n"                                   \
  "FOREHEAD YOU BEGIN THE LONG TREK BACK\n"                                  \
  "HOME. REBUILDING EARTH OUGHT TO BE A\n"                                   \
  "LOT MORE FUN THAN RUINING IT WAS.\n"

/* Before niveau 31, put this: */

#define C5TEXT                                                               \
  "CONGRATULATIONS, YOU'VE FOUND THE SECRET\n"                               \
  "LEVEL! LOOKS LIKE IT'S BEEN BUILT BY\n"                                   \
  "HUMANS, RATHER THAN DEMONS. YOU WONDER\n"                                 \
  "WHO THE INMATES OF THIS CORNER OF HELL\n"                                 \
  "WILL BE."

/* Before niveau 32, put this: */

#define C6TEXT                                                               \
  "CONGRATULATIONS, YOU'VE FOUND THE\n"                                      \
  "SUPER SECRET LEVEL!  YOU'D BETTER\n"                                      \
  "BLAZE THROUGH THIS ONE!\n"

/* after map 06 */

#define P1TEXT                                                               \
  "You gloat over the steaming carcass of the\n"                             \
  "Guardian.  With its death, you've wrested\n"                              \
  "the Accelerator from the stinking claws\n"                                \
  "of Hell.  You relax and glance around the\n"                              \
  "room.  Damn!  There was supposed to be at\n"                              \
  "least one working prototype, but you can't\n"                             \
  "see it. The demons must have taken it.\n"                                 \
  "\n"                                                                       \
  "You must find the prototype, or all your\n"                               \
  "struggles will have been wasted. Keep\n"                                  \
  "moving, keep fighting, keep killing.\n"                                   \
  "Oh yes, keep living, too."

/* after map 11 */

#define P2TEXT                                                               \
  "Even the deadly Arch-Vile labyrinth could\n"                              \
  "not stop you, and you've gotten to the\n"                                 \
  "prototype Accelerator which is soon\n"                                    \
  "efficiently and permanently deactivated.\n"                               \
  "\n"                                                                       \
  "You're good at that kind of thing."

/* after map 20 */

#define P3TEXT                                                               \
  "You've bashed and battered your way into\n"                               \
  "the heart of the devil-hive.  Time for a\n"                               \
  "Search-and-Destroy mission, aimed at the\n"                               \
  "Gatekeeper, whose foul offspring is\n"                                    \
  "cascading to Earth.  Yeah, he's bad. But\n"                               \
  "you know who's worse!\n"                                                  \
  "\n"                                                                       \
  "Grinning evilly, you check your gear, and\n"                              \
  "get ready to give the bastard a little Hell\n"                            \
  "of your own making!"

/* after map 30 */

#define P4TEXT                                                               \
  "The Gatekeeper's evil face is splattered\n"                               \
  "all over the place.  As its tattered corpse\n"                            \
  "collapses, an inverted Gate forms and\n"                                  \
  "sucks down the shards of the last\n"                                      \
  "prototype Accelerator, not to mention the\n"                              \
  "few remaining demons.  You're done. Hell\n"                               \
  "has gone back to pounding bad dead folks \n"                              \
  "instead of good live ones.  Remember to\n"                                \
  "tell your grandkids to put a rocket\n"                                    \
  "launcher in your coffin. If you go to Hell\n"                             \
  "when you die, you'll need it for some\n"                                  \
  "final cleaning-up ..."

/* before map 31 */

#define P5TEXT                                                               \
  "You've found the second-hardest niveau we\n"                              \
  "got. Hope you have a saved game a niveau or\n"                            \
  "two previous.  If not, be prepared to die\n"                              \
  "aplenty. For master marines only."

/* before map 32 */

#define P6TEXT                                                               \
  "Betcha wondered just what WAS the hardest\n"                              \
  "niveau we had ready for ya?  Now you know.\n"                             \
  "No one gets out alive."

#define T1TEXT                                                               \
  "You've fought your way out of the infested\n"                             \
  "experimental labs.   It seems that UAC has\n"                             \
  "once again gulped it down.  With their\n"                                 \
  "high turnover, it must be hard for poor\n"                                \
  "old UAC to buy corporate health insurance\n"                              \
  "nowadays..\n"                                                             \
  "\n"                                                                       \
  "Ahead lies the military complex, now\n"                                   \
  "swarming with diseased horrors hot to get\n"                              \
  "their teeth into you. With luck, the\n"                                   \
  "complex still has some warlike ordnance\n"                                \
  "laying around."

#define T2TEXT                                                               \
  "You hear the grinding of heavy machinery\n"                               \
  "ahead.  You sure hope they're not stamping\n"                             \
  "out new hellspawn, but you're ready to\n"                                 \
  "ream out a whole herd if you have to.\n"                                  \
  "They might be planning a blood feast, but\n"                              \
  "you feel about as mean as two thousand\n"                                 \
  "maniacs packed into one mad killer.\n"                                    \
  "\n"                                                                       \
  "You don't plan to go down easy."

#define T3TEXT                                                               \
  "The vista opening ahead looks real damn\n"                                \
  "familiar. Smells familiar, too -- like\n"                                 \
  "fried excrement. You didn't like this\n"                                  \
  "place before, and you sure as hell ain't\n"                               \
  "planning to like it now. The more you\n"                                  \
  "brood on it, the madder you get.\n"                                       \
  "Hefting your gun, an evil grin trickles\n"                                \
  "onto your face. Time to take some names."

#define T4TEXT                                                               \
  "Suddenly, all is silent, from one horizon\n"                              \
  "to the other. The agonizing echo of Hell\n"                               \
  "fades away, the nightmare sky turns to\n"                                 \
  "blue, the heaps of monster corpses start \n"                              \
  "to evaporate along with the evil stench \n"                               \
  "that filled the air. Jeeze, maybe you've\n"                               \
  "done it. Have you really won?\n"                                          \
  "\n"                                                                       \
  "Something rumbles in the distance.\n"                                     \
  "A blue light begins to glow inside the\n"                                 \
  "ruined skull of the demon-spitter."

#define T5TEXT                                                               \
  "What now? Looks totally different. Kind\n"                                \
  "of like King Tut's condo. Well,\n"                                        \
  "whatever's here can't be any worse\n"                                     \
  "than usual. Can it?  Or maybe it's best\n"                                \
  "to let sleeping gods lie.."

#define T6TEXT                                                               \
  "Time for a vacation. You've burst the\n"                                  \
  "bowels of hell and by golly you're ready\n"                               \
  "for a break. You mutter to yourself,\n"                                   \
  "Maybe someone else can kick Hell's ass\n"                                 \
  "next time around. Ahead lies a quiet town,\n"                             \
  "with peaceful flowing water, quaint\n"                                    \
  "buildings, and presumably no Hellspawn.\n"                                \
  "\n"                                                                       \
  "As you step off the transport, you hear\n"                                \
  "the stomp of a cyberdemon's iron shoe."

/* Character cast strings F_FINALE.C */

#define CC_ZOMBIE "ZOMBIEMAN"
#define CC_SHOTGUN "SHOTGUN GUY"
#define CC_HEAVY "HEAVY WEAPON DUDE"
#define CC_IMP "IMP"
#define CC_DEMON "DEMON"
#define CC_LOST "LOST SOUL"
#define CC_CACO "CACODEMON"
#define CC_HELL "HELL KNIGHT"
#define CC_BARON "BARON OF HELL"
#define CC_ARACH "ARACHNOTRON"
#define CC_PAIN "PAIN ELEMENTAL"
#define CC_REVEN "REVENANT"
#define CC_MANCU "MANCUBUS"
#define CC_ARCH "ARCH-VILE"
#define CC_SPIDER "THE SPIDER MASTERMIND"
#define CC_CYBER "THE CYBERDEMON"
#define CC_HERO "OUR HERO"

/* DOOM end messages */

#define DOOM1_ENDMSG                                                         \
  "are you sure you want to\nquit this great game?",                         \
      "please don't leave, there's more\ndemons to toast!",                  \
      "let's beat it -- this is turning\ninto a bloodbath!",                 \
      "i wouldn't leave if i were you.\ndos is much worse.",                 \
      "you're trying to say you like dos\nbetter than me, right?",           \
      "don't leave yet -- there's a\ndemon around that corner!",             \
      "ya know, next time you come in here\ni'm gonna toast ya.",            \
      "go ahead and leave. see if i care.",

/* QuitDOOM II messages */

#define DOOM2_ENDMSG                                                         \
  "are you sure you want to\nquit this great game?",                         \
      "you want to quit?\nthen, thou hast lost an eighth!",                  \
      "don't go now, there's a \ndimensional shambler waiting\nat the dos "  \
      "prompt!",                                                             \
      "get outta here and go back\nto your boring programs.",                \
      "if i were your boss, i'd \n deathmatch ya in a minute!",              \
      "look, bud. you leave now\nand you forfeit your body count!",          \
      "just leave. when you come\nback, i'll be waiting with a bat.",        \
      "you're lucky i don't smack\nyou for thinking about leaving.",

#endif /* __D_FRENCH__ */
