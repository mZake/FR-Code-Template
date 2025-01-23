#include "../include/battle_setup.h"
#include "../include/bg.h"
#include "../include/daycare.h"
#include "../include/event_data.h"
#include "../include/event_object_movement.h"
#include "../include/field_control_avatar.h"
#include "../include/field_effect.h"
#include "../include/field_effect_helpers.h"
#include "../include/field_fadetransition.h"
#include "../include/field_player_avatar.h"
#include "../include/field_poison.h"
#include "../include/field_screen_effect.h"
#include "../include/field_weather.h"
#include "../include/fieldmap.h"
#include "../include/item.h"
#include "../include/link.h"
#include "../include/list_menu.h"
#include "../include/m4a.h"
#include "../include/map_name_popup.h"
#include "../include/map_preview_screen.h"
#include "../include/metatile_behavior.h"
#include "../include/overworld.h"
#include "../include/party_menu.h"
#include "../include/quest_log.h"
#include "../include/random.h"
#include "../include/safari_zone.h"
#include "../include/script.h"
#include "../include/sound.h"

#include "../include/constants/flags.h"
#include "../include/constants/items.h"
#include "../include/constants/moves.h"
#include "../include/constants/metatile_behaviors.h"
#include "../include/constants/region_map_sections.h"
#include "../include/constants/songs.h"
#include "../include/constants/trainers.h"

u8 CanMonLearnTMTutor(struct Pokemon *mon, u16 item, u8 tutor);

extern u8 EventScript_UseFlash[];

// The values here can be modified i guess
const u8 gFieldMoveBadgeRequirements[] =
{
	[FIELD_MOVE_FLASH] = 1,
	[FIELD_MOVE_CUT] = 2,
	[FIELD_MOVE_FLY] = 3,
	[FIELD_MOVE_STRENGTH] = 4,
	[FIELD_MOVE_SURF] = 5,
	[FIELD_MOVE_ROCK_SMASH] = 6,
	[FIELD_MOVE_WATERFALL] = 0,
};

bool8 HasBadgeToUseFieldMove(u8 id)
{
	return gFieldMoveBadgeRequirements[id] == 0 || FlagGet(FLAG_BADGE01_GET + (gFieldMoveBadgeRequirements[id] - 1));
}

void CanPlayerUseFlashInCurrentLocation(void)
{
	gSpecialVar_Result = gMapHeader.cave == TRUE && !FlagGet(FLAG_SYS_FLASH_ACTIVE);
}

u8 PartyHasMonWithFieldMovePotential(u16 move, u16 item, u8 surfingType)
{
	bool8 isSurfing = TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING);

	if (surfingType == 0 || (surfingType == 1 && !isSurfing) || (surfingType == 2 && isSurfing))
	{
		bool8 hasHM = CheckBagHasItem(item, 1) > 0;

		for (u32 i = 0; i < PARTY_SIZE; ++i)
		{
			struct Pokemon* mon = &gPlayerParty[i];

			if (GetMonData(mon, MON_DATA_SPECIES, NULL) != SPECIES_NONE && !GetMonData(mon, MON_DATA_IS_EGG, NULL))
			{
				if (MonKnowsMove(mon, move))
					return i;

				if (hasHM && CanMonLearnTMTutor(mon, item, 0) == 0)
					return i;
			}
		}
	}

	return PARTY_SIZE;
}

static const u8* TryUseFlashInDarkCave(void)
{
	CanPlayerUseFlashInCurrentLocation();

	if (gSpecialVar_Result && HasBadgeToUseFieldMove(FIELD_MOVE_FLASH))
	{
		if ((gSpecialVar_0x8004 = gFieldEffectArguments[0] = PartyHasMonWithFieldMovePotential(MOVE_FLASH, ITEM_HM05_FLASH, 0)) < PARTY_SIZE)
			return EventScript_UseFlash;
	}

	return NULL;
}
