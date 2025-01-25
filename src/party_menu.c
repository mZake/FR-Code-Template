#include "../include/evolution_scene.h"
#include "../include/field_control_avatar.h"
#include "../include/field_player_avatar.h"
#include "../include/field_effect.h"
#include "../include/field_screen_effect.h"
#include "../include/field_weather.h"
#include "../include/fieldmap.h"
#include "../include/item_use.h"
#include "../include/item_menu.h"
#include "../include/menu.h"
#include "../include/metatile_behavior.h"
#include "../include/overworld.h"
#include "../include/party_menu.h"
#include "../include/pokemon_icon.h"
#include "../include/pokemon_storage_system.h"
#include "../include/random.h"
#include "../include/script.h"
#include "../include/sound.h"
#include "../include/start_menu.h"
#include "../include/string_util.h"
#include "../include/text.h"
#include "../include/wild_encounter.h"
#include "../include/window.h"
#include "../include/item.h"
#include "../include/mail_data.h"
#include "../include/event_data.h"
#include "../include/constants/abilities.h"
#include "../include/constants/hold_effects.h"
#include "../include/constants/items.h"
#include "../include/constants/item_effects.h"
#include "../include/constants/moves.h"
#include "../include/constants/region_map_sections.h"
#include "../include/constants/songs.h"

struct PartyMenuInternal
{
	TaskFunc task;
	MainCallback exitCallback;
	u32 chooseHalf:1;
	u32 lastSelectedSlot:3;  //Used to return to same slot when going left/right bewtween columns
	u32 spriteIdConfirmPokeball:7;
	u32 spriteIdCancelPokeball:7;
	u32 messageId:14;
	u8 windowId[3]; //windowId[1] is highlighted mon
	u8 actions[8];
	u8 numActions;
	u16 palBuffer[BG_PLTT_SIZE / sizeof(u16)];
	s16 data[16];
};

extern struct PartyMenuInternal* sPartyMenuInternal;

const u16 gFieldMoves[] =
{
	[FIELD_MOVE_FLASH] = MOVE_FLASH,
	[FIELD_MOVE_CUT] = MOVE_CUT,
	[FIELD_MOVE_FLY] = MOVE_FLY,
	[FIELD_MOVE_STRENGTH] = MOVE_STRENGTH,
	[FIELD_MOVE_SURF] = MOVE_SURF,
	[FIELD_MOVE_WATERFALL] = MOVE_WATERFALL,
	[FIELD_MOVE_TELEPORT] = MOVE_TELEPORT,
	[FIELD_MOVE_DIG] = MOVE_DIG,
};

//Todo: hook
void SetPartyMonFieldSelectionActions(struct Pokemon *mons, u8 slotId)
{
	u8 i, j, k;

	sPartyMenuInternal->numActions = 0;
	AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_SUMMARY);

	//Add field moves to action list
	for (i = 0, k = 0; i < MAX_MON_MOVES; ++i)
	{
		for (j = 0; j < NELEMS(gFieldMoves); ++j)
		{
			if (GetMonData(&mons[slotId], i + MON_DATA_MOVE1, NULL) == gFieldMoves[j])
			{
				AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, j + MENU_FIELD_MOVES);
				++k;

				if (gFieldMoves[j] == MOVE_FLY)
					k = MAX_MON_MOVES; //No point in appending Fly if it is already there
				break;
			}
		}
	}

	//Try to give the mon fly
	if (k < MAX_MON_MOVES) //Doesn't know 4 field moves
	{
		bool8 hasHM = CheckBagHasItem(ITEM_HM02_FLY, 1) > 0;
		u16 species = GetMonData(&mons[slotId], MON_DATA_SPECIES_OR_EGG, NULL);

		if (species != SPECIES_NONE
		&& species != SPECIES_EGG
		&& hasHM
		&& HasBadgeToUseFieldMove(FIELD_MOVE_FLY)
		&& CanMonLearnTMTutor(&mons[slotId], ITEM_HM02_FLY, 0) == 0)
		{
			AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_FIELD_MOVES + FIELD_MOVE_FLY);
			++k;
		}
	}


    if (GetMonData(&mons[1], MON_DATA_SPECIES, NULL) != SPECIES_NONE)
        AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_SWITCH);
    if (ItemIsMail(GetMonData(&mons[slotId], MON_DATA_HELD_ITEM, NULL)))
        AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_MAIL);
    else
        AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_ITEM);

	AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_CANCEL1);
}

#define FieldCallback_Surf (void*) (0x812497C | 1)

//Todo: hook
bool8 SetUpFieldMove_Surf(void)
{
	u16 item = ITEM_NONE;

	item = ITEM_HM03_SURF;

	if (PartyHasMonWithFieldMovePotential(MOVE_SURF, item, 1) < PARTY_SIZE
	&& IsPlayerFacingSurfableFishableWater() == TRUE)
	{
		gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
		gPostMenuFieldCallback = FieldCallback_Surf;
		return TRUE;
	}

	return FALSE;
}

//Todo: hook
void sp10A_CanUseCutOnTree(void)
{
	u16 item = ITEM_NONE;

	item = ITEM_HM01_CUT;

	gSpecialVar_0x8004 = PARTY_SIZE;
	if (HasBadgeToUseFieldMove(FIELD_MOVE_CUT))
		gSpecialVar_0x8004 = PartyHasMonWithFieldMovePotential(MOVE_CUT, item, 1);
}

//Todo: hook
void sp10B_CanUseRockSmashOnRock(void)
{
	u16 item = ITEM_NONE;

	item = ITEM_HM06_ROCK_SMASH;

	gSpecialVar_0x8004 = PARTY_SIZE;
	if (HasBadgeToUseFieldMove(FIELD_MOVE_ROCK_SMASH))
		gSpecialVar_0x8004 = PartyHasMonWithFieldMovePotential(0, item, 1);
}

//Todo: hook
void sp10C_CanUseStrengthOnBoulder(void)
{
	u16 item = ITEM_NONE;

	item = ITEM_HM04_STRENGTH;

	gSpecialVar_0x8004 = PARTY_SIZE;
	if (HasBadgeToUseFieldMove(FIELD_MOVE_STRENGTH))
		gSpecialVar_0x8004 = PartyHasMonWithFieldMovePotential(MOVE_STRENGTH, item, 1);
}
