.align 2
.thumb

.include "xse_commands.s"
.include "xse_defines.s"

.global EventScript_UseFlash

EventScript_UseFlash:
	lockall
	bufferpartypokemon 0x0 0x8004
	bufferattack 0x1 0x94
	setflag 0x806
	msgbox 0x81BDFD7 MSG_NORMAL
	checksound
	sound 0xC8
	animateflash 0x0
	setflashradius 0x0
	releaseall
	end
