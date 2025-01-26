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

bool8 SetUpFieldMove_Fly(void);
bool8 SetUpFieldMove_Surf(void);
bool8 SetUpFieldMove_Waterfall(void);
bool8 SetUpFieldMove_Teleport(void);

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
extern const u16 sFieldMoves[];

struct FieldMoveCB
{
	bool8 (*fieldMoveFunc)(void);
	u8 msgId;
};

const struct FieldMoveCB gFieldMoveCursorCallbacks[] =
{
	[FIELD_MOVE_FLASH] = {(void*) 0x80C9B2D, 0x0d},
	[FIELD_MOVE_CUT] = {(void*) 0x8097899, 0x07},
	[FIELD_MOVE_FLY] = {SetUpFieldMove_Fly, 0x0d},
	[FIELD_MOVE_STRENGTH] = {(void*) 0x80D07ED, 0x0d},
	[FIELD_MOVE_SURF] = {SetUpFieldMove_Surf, 0x08},
	[FIELD_MOVE_ROCK_SMASH] = {(void*) 0x80C99D9, 0x0d},
	[FIELD_MOVE_WATERFALL] = {SetUpFieldMove_Waterfall, 0x0d},
	[FIELD_MOVE_TELEPORT] = {SetUpFieldMove_Teleport, 0x0d},
	[FIELD_MOVE_DIG] = {(void*) 0x80C9A79, 0x0d},
	[FIELD_MOVE_MILK_DRINK] = {(void*) 0x80E5685, 0x10},
	[FIELD_MOVE_SOFT_BOILED] = {(void*) 0x80E5685, 0x10},
	[FIELD_MOVE_SWEET_SCENT] = {(void*) 0x80DE0C9, 0x0d},
};

#define FieldCallback_Surf (void*) (0x812497C | 1)
#define FieldCallback_Teleport (void*) (0x80F6730 | 1)

bool8 SetUpFieldMove_Fly(void)
{
	if (Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
		return TRUE;

	return FALSE;
}

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

bool8 SetUpFieldMove_Waterfall(void)
{
	s16 x, y;

	GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
	if (MetatileBehavior_IsWaterfall(MapGridGetMetatileBehaviorAt(x, y)) == TRUE && IsPlayerSurfingNorthOrSouth() == TRUE)
	{
		gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
		gPostMenuFieldCallback = (void*) 0x8124ADD;
		return TRUE;
	}

	return FALSE;
}

bool8 SetUpFieldMove_Teleport(void)
{
	if (Overworld_MapTypeAllowsTeleportAndFly(gMapHeader.mapType) == TRUE)
	{
		gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
		gPostMenuFieldCallback = FieldCallback_Teleport;
		return TRUE;
	}

	return FALSE;
}

void SetPartyMonFieldSelectionActions(struct Pokemon *mons, u8 slotId)
{
	u8 i, j, k;

	sPartyMenuInternal->numActions = 0;
	AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, MENU_SUMMARY);

	//Add field moves to action list
	for (i = 0, k = 0; i < MAX_MON_MOVES; ++i)
	{
		for (j = 0; j < FIELD_MOVE_END; ++j)
		{
			if (GetMonData(&mons[slotId], i + MON_DATA_MOVE1, NULL) == sFieldMoves[j])
			{
				AppendToList(sPartyMenuInternal->actions, &sPartyMenuInternal->numActions, j + MENU_FIELD_MOVES);
				++k;

				if (sFieldMoves[j] == MOVE_FLY)
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

void sp10A_CanUseCutOnTree(void)
{
	u16 item = ITEM_NONE;

	item = ITEM_HM01_CUT;

	gSpecialVar_0x8004 = PARTY_SIZE;
	if (HasBadgeToUseFieldMove(FIELD_MOVE_CUT))
		gSpecialVar_0x8004 = PartyHasMonWithFieldMovePotential(MOVE_CUT, item, 1);
}

void sp10B_CanUseRockSmashOnRock(void)
{
	u16 item = ITEM_NONE;

	item = ITEM_HM06_ROCK_SMASH;

	gSpecialVar_0x8004 = PARTY_SIZE;
	if (HasBadgeToUseFieldMove(FIELD_MOVE_ROCK_SMASH))
		gSpecialVar_0x8004 = PartyHasMonWithFieldMovePotential(MOVE_ROCK_SMASH, item, 1);
}

void sp10C_CanUseStrengthOnBoulder(void)
{
	u16 item = ITEM_NONE;

	item = ITEM_HM04_STRENGTH;

	gSpecialVar_0x8004 = PARTY_SIZE;
	if (HasBadgeToUseFieldMove(FIELD_MOVE_STRENGTH))
		gSpecialVar_0x8004 = PartyHasMonWithFieldMovePotential(MOVE_STRENGTH, item, 1);
}
