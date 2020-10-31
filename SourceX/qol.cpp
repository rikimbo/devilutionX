/**
 * @file qol.cpp
 *
 * Quality of life features
 */
#include "all.h"
#include "../3rdParty/Storm/Source/storm.h"

DEVILUTION_BEGIN_NAMESPACE

int GetTextWidth(char *s)
{
	int l = 0;
	while (*s) {
		l += fontkern[fontframe[gbFontTransTbl[(BYTE)*s++]]] + 1;
	}
	return l;
}

int GetConfigIntValue(const char *valuename, int base)
{
	if (!SRegLoadValue("devilutionx", valuename, 0, &base)) {
		SRegSaveValue("devilutionx", valuename, 0, base);
	}
	return base;
}

int highlightItemsMode = 0;
// 0 = disabled
// 1 = highlight when alt pressed
// 2 = hide when alt pressed
// 3 = always highlight
bool altPressed = false;
bool drawXPBar = false;
bool drawHPBar = false;

inline void FastDrawHorizLine(int x, int y, int width, BYTE col)
{
	memset(&gpBuffer[SCREENXY(x,y)], col, width);
}

inline void FillRect(int x, int y, int width, int height, BYTE col)
{
	for (int j = 0; j < height; j++) {
		FastDrawHorizLine(x, y + j, width, col);
	}
}

inline void FillSquare(int x, int y, int size, BYTE col)
{
	FillRect(x, y, size, size, col);
}

void DrawMonsterHealthBar()
{
	if (!drawHPBar)
		return;
	if (currlevel == 0)
		return;
	if (pcursmonst == -1)
		return;

	int width = 250;
	int height = 25;
	int x = 0; // x offset from the center of the screen
	int y = 20; // y position
	int xOffset = 0; // empty space between left/right borders and health bar
	int yOffset = 1; // empty space between top/bottom borders and health bar
	int borderSize = 2; // size of the border around health bar
	BYTE borderColors[] = { 242 /*undead*/, 232 /*demon*/, 182 /*beast*/ };
	BYTE filledColor = 142;     // filled health bar color
	bool fillCorners = false; // true to fill border corners, false to cut them off
	int square = 10; // resistance / immunity / vulnerability square size
	char *immuText = "IMMU: ", *resText = "RES: ", *vulnText = ":VULN"; // text displayed for immunities / resistances / vulnerabilities
	int resSize = 3; // how many damage types
	BYTE resistColors[] = { 148, 140, 129 }; // colors for these damage types
	WORD immunes[] = { IMMUNE_MAGIC, IMMUNE_FIRE, IMMUNE_LIGHTNING }; // immunity flags for damage types
	WORD resists[] = { RESIST_MAGIC, RESIST_FIRE, RESIST_LIGHTNING }; // resistance flags for damage types

	MonsterStruct *mon = &monster[pcursmonst];
	BYTE borderColor = borderColors[(BYTE)mon->MData->mMonstClass];
	WORD mres = mon->mMagicRes;
	bool drawImmu = false;
	int xPos = (SCREEN_WIDTH - width) / 2 + x;
	int xPos2 = xPos + width / 2;
	int yPos = y;
	int immuOffset = GetTextWidth(immuText) - 5;
	int resOffset = GetTextWidth(resText);
	int vulOffset = width - square - GetTextWidth(vulnText) - 4;
	int corners = (fillCorners ? borderSize : 0);
	int currentLife = mon->_mhitpoints, maxLife = mon->_mmaxhp;
	if (currentLife > maxLife)
		maxLife = currentLife;
	if (mon->_uniqtype != 0)
		borderSize <<= 1;

	FillRect(xPos, yPos, (width * currentLife) / maxLife, height, filledColor);
	for (int j = 0; j < borderSize; j++) {
		FastDrawHorizLine(xPos - xOffset - corners, yPos - borderSize - yOffset + j, (xOffset + corners) * 2 + width, borderColor);
		FastDrawHorizLine(xPos - xOffset, yPos + height + yOffset + j, width + corners + xOffset * 2, borderColor);
	}
	for (int j = -yOffset; j < yOffset + height + corners; ++j) {
		FastDrawHorizLine(xPos - xOffset - borderSize, yPos + j, borderSize, borderColor);
		FastDrawHorizLine(xPos + xOffset + width, yPos + j, borderSize, borderColor);
	}
	for (int k = 0; k < resSize; ++k) {
		if (mres & immunes[k]) {
			drawImmu = true;
			FillSquare(xPos + immuOffset, yPos + height - square, square, resistColors[k]);
			immuOffset += square + 2;
		} else if ((mres & resists[k])) {
			FillSquare(xPos + resOffset, yPos + yOffset + height + borderSize + 2, square, resistColors[k]);
			resOffset += square + 2;
		} else {
			FillSquare(xPos + vulOffset, yPos + yOffset + height + borderSize + 2, square, resistColors[k]);
			vulOffset -= square + 2;
		}
	}

	char text[64];
	strcpy(text, mon->mName);
	if (mon->leader > 0)
		strcat(text, " (minion)");
	PrintGameStr(xPos2 - GetTextWidth(text) / 2, yPos + 10, text, (mon->_uniqtype != 0 ? COL_GOLD : COL_WHITE));

	sprintf(text, "%d", (maxLife >> 6));
	PrintGameStr(xPos2 + GetTextWidth("/"), yPos + 23, text, COL_WHITE);

	sprintf(text, "%d", (currentLife >> 6));
	PrintGameStr(xPos2 - GetTextWidth(text) - GetTextWidth("/"), yPos + 23, text, COL_WHITE);

	PrintGameStr(xPos2 - GetTextWidth("/") / 2, yPos + 23, "/", COL_WHITE);

	sprintf(text, "kills: %d", monstkills[mon->MType->mtype]);
	PrintGameStr(xPos2 - GetTextWidth("kills:") / 2 - 30, yPos + yOffset + height + borderSize + 12, text, COL_WHITE);


	if (drawImmu)
		PrintGameStr(xPos2 - width / 2, yPos + height, immuText, COL_GOLD);

	PrintGameStr(xPos2 - width / 2, yPos + yOffset + height + borderSize + 12, resText, COL_GOLD);
	PrintGameStr(xPos2 + width / 2 - GetTextWidth(vulnText), yPos + yOffset + height + borderSize + 12, vulnText, COL_RED);
}

void DrawXPBar()
{
	if (!drawXPBar)
		return;
	int barSize = 306; // *ScreenWidth / 640;
	int offset = 3;
	int barRows = 3; // *ScreenHeight / 480;
	int dividerHeight = 3;
	int numDividers = 10;
	int barColor = 242; /*242white, 142red, 200yellow, 182blue*/
	int emptyBarColor = 0;
	int frameColor = 242;
	int yPos = BORDER_TOP + SCREEN_HEIGHT - 8;

	PrintGameStr(145 + (SCREEN_WIDTH - 640) / 2, SCREEN_HEIGHT - 4, "XP", COL_WHITE);
	int charLevel = plr[myplr]._pLevel;
	if (charLevel != MAXCHARLEVEL - 1) {
		int curXp = ExpLvlsTbl[charLevel];
		int prevXp = ExpLvlsTbl[charLevel - 1];
		int prevXpDelta = curXp - prevXp;
		int prevXpDelta_1 = plr[myplr]._pExperience - prevXp;
		if (plr[myplr]._pExperience >= prevXp) {
			int visibleBar = barSize * (unsigned __int64)prevXpDelta_1 / prevXpDelta;

			for (int i = 0; i < visibleBar; ++i) {
				for (int j = 0; j < barRows; ++j) {
					ENG_set_pixel((BUFFER_WIDTH - barSize) / 2 + i + offset, yPos - barRows / 2 + j, barColor);
				}
			}

			for (int i = visibleBar; i < barSize; ++i) {
				for (int j = 0; j < barRows; ++j) {
					ENG_set_pixel((BUFFER_WIDTH - barSize) / 2 + i + offset, yPos - barRows / 2 + j, emptyBarColor);
				}
			}
			//draw frame
			//horizontal
			for (int i = -1; i <= barSize; ++i) {
				ENG_set_pixel((BUFFER_WIDTH - barSize) / 2 + i + offset, yPos - barRows / 2 - 1, frameColor);
				ENG_set_pixel((BUFFER_WIDTH - barSize) / 2 + i + offset, yPos + barRows / 2 + 1, frameColor);
			}
			//vertical
			for (int i = -dividerHeight; i < barRows + dividerHeight; ++i) {
				if (i >= 0 && i < barRows) {
					ENG_set_pixel((BUFFER_WIDTH - barSize) / 2 - 1 + offset, yPos - barRows / 2 + i, frameColor);
					ENG_set_pixel((BUFFER_WIDTH - barSize) / 2 + barSize + offset, yPos - barRows / 2 + i, frameColor);
				}
				for (int j = 1; j < numDividers; ++j) {
					ENG_set_pixel((BUFFER_WIDTH - barSize) / 2 - 1 + offset + (barSize * j / numDividers), yPos - barRows / 2 + i, frameColor);
				}
			}
			//draw frame
		}
	}
}

class drawingQueue {
public:
	int ItemID;
	int Row;
	int Col;
	int x;
	int y;
	int width;
	int height;
	int color;
	char text[64];
	drawingQueue(int x2, int y2, int width2, int height2, int Row2, int Col2, int ItemID2, int q2, char *text2)
	{
		x = x2;
		y = y2;
		Row = Row2;
		Col = Col2;
		ItemID = ItemID2;
		width = width2;
		height = height2;
		color = q2;
		strcpy(text, text2);
	}
};

std::vector<drawingQueue> drawQ;

void adjustCoordsToZoom(int &x, int &y)
{
	if (zoomflag)
		return;
	int distToCenterX = abs(SCREEN_WIDTH / 2 - x);
	int distToCenterY = abs(SCREEN_HEIGHT / 2 - y);
	if (x <= SCREEN_WIDTH / 2) {
		x = SCREEN_WIDTH / 2 - distToCenterX * 2;
	} else {
		x = SCREEN_WIDTH / 2 + distToCenterX * 2;
	}

	if (y <= SCREEN_HEIGHT / 2) {
		y = SCREEN_HEIGHT / 2 - distToCenterY * 2;
	} else {
		y = SCREEN_HEIGHT / 2 + distToCenterY * 2;
	}
	x += SCREEN_WIDTH / 2 - 20;
	y += SCREEN_HEIGHT / 2 - 175;
}

void AddItemToDrawQueue(int x, int y, int id)
{
	if (highlightItemsMode == 0 || (highlightItemsMode == 1 && !altPressed) || (highlightItemsMode == 2 && altPressed))
		return;
	ItemStruct *it = &item[id];

	char textOnGround[64];
	if (it->_itype == ITYPE_GOLD) {
		sprintf(textOnGround, "%i gold", it->_ivalue);
	} else {
		sprintf(textOnGround, "%s", it->_iIdentified ? it->_iIName : it->_iName);
	}

	int centerXOffset = GetTextWidth((char *)textOnGround);

	adjustCoordsToZoom(x, y);
	x -= centerXOffset / 2 + 20;
	y -= 193;
	char clr = COL_WHITE;
	if (it->_iMagical == ITEM_QUALITY_MAGIC)
		clr = COL_BLUE;
	if (it->_iMagical == ITEM_QUALITY_UNIQUE)
		clr = COL_GOLD;
	drawQ.push_back(drawingQueue(x, y, GetTextWidth((char *)textOnGround), 13, it->_ix, it->_iy, id, clr, textOnGround));
}

void DrawBackground(int xPos, int yPos, int width, int height, int borderX, int borderY, BYTE backgroundColor, BYTE borderColor)
{
	xPos += BORDER_LEFT;
	yPos += BORDER_TOP;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (x < borderX || x + borderX >= width || y < borderY || y + borderY >= height)
				continue;
			int val = ((yPos - height) + y) * BUFFER_WIDTH + xPos + x;
			gpBuffer[val] = backgroundColor;
		}
	}
}

void HighlightItemsNameOnMap()
{
	if (highlightItemsMode == 0 || (highlightItemsMode == 1 && !altPressed) || (highlightItemsMode == 2 && altPressed))
		return;
	const int borderX = 5;
	for (unsigned int i = 0; i < drawQ.size(); ++i) {
		std::map<int, bool> backtrace;

		bool canShow;
		do {
			canShow = true;
			for (unsigned int j = 0; j < i; ++j) {
				if (abs(drawQ[j].y - drawQ[i].y) < drawQ[i].height + 2) {
					int newpos = drawQ[j].x;
					if (drawQ[j].x >= drawQ[i].x && drawQ[j].x - drawQ[i].x < drawQ[i].width + borderX) {
						newpos -= drawQ[i].width + borderX;
						if (backtrace.find(newpos) != backtrace.end())
							newpos = drawQ[j].x + drawQ[j].width + borderX;
					} else if (drawQ[j].x < drawQ[i].x && drawQ[i].x - drawQ[j].x < drawQ[j].width + borderX) {
						newpos += drawQ[j].width + borderX;
						if (backtrace.find(newpos) != backtrace.end())
							newpos = drawQ[j].x - drawQ[i].width - borderX;
					} else
						continue;
					canShow = false;
					drawQ[i].x = newpos;
					backtrace[newpos] = true;
				}
			}
		} while (!canShow);
	}

	for (unsigned int i = 0; i < drawQ.size(); ++i) {
		drawingQueue t = drawQ[i];

		int sx = t.x;
		int sy = t.y;

		int sx2 = sx;
		int sy2 = sy + 1;

		if (sx < 0 || sx >= SCREEN_WIDTH || sy < 0 || sy >= SCREEN_HEIGHT) {
			continue;
		}
		if (sx2 < 0 || sx2 >= SCREEN_WIDTH || sy2 < 0 || sy2 >= SCREEN_HEIGHT) {
			continue;
		}

		int bgcolor = 0;
		int CursorX = MouseX;
		int CursorY = MouseY + t.height;

		if (CursorX >= sx && CursorX <= sx + t.width + 1 && CursorY >= sy && CursorY <= sy + t.height) {
			if ((invflag || sbookflag) && MouseX > RIGHT_PANEL && MouseY <= SPANEL_HEIGHT) {
			} else if ((chrflag || questlog) && MouseX < SPANEL_WIDTH && MouseY <= SPANEL_HEIGHT) {
			} else if (MouseY >= PANEL_TOP && MouseX >= PANEL_LEFT && MouseX <= PANEL_LEFT + PANEL_WIDTH) {
			} else {
				cursmx = t.Row;
				cursmy = t.Col;
				pcursitem = t.ItemID;
			}
		}
		if (pcursitem == t.ItemID)
			bgcolor = 134;
		DrawBackground(sx2, sy2, t.width + 1, t.height, 0, 0, bgcolor, bgcolor);
		PrintGameStr(sx, sy, t.text, t.color);
	}
	drawQ.clear();
}

void diablo_parse_config()
{
	drawHPBar = GetConfigIntValue("monster health bar", 0) != 0;
	drawXPBar = GetConfigIntValue("xp bar", 0) != 0;
	highlightItemsMode = GetConfigIntValue("highlight items", 0);
}

void SaveHotkeys()
{
	BYTE *oldtbuff = tbuff;
	DWORD dwLen = codec_get_encoded_len(4 * (sizeof(int) + sizeof(char)));
	BYTE *SaveBuff = DiabloAllocPtr(dwLen);
	tbuff = SaveBuff;

	CopyInts(&plr[myplr]._pSplHotKey, 4, tbuff);
	CopyBytes(&plr[myplr]._pSplTHotKey, 4, tbuff);

	dwLen = codec_get_encoded_len(tbuff - SaveBuff);
	pfile_write_save_file("hotkeys", SaveBuff, tbuff - SaveBuff, dwLen);
	mem_free_dbg(SaveBuff);
	tbuff = oldtbuff;
}

void LoadHotkeys()
{
	BYTE *oldtbuff = tbuff;
	DWORD dwLen;
	BYTE *LoadBuff = pfile_read("hotkeys", &dwLen);
	if (LoadBuff != NULL) {
		tbuff = LoadBuff;

		CopyInts(tbuff, 4, &plr[myplr]._pSplHotKey);
		CopyBytes(tbuff, 4, &plr[myplr]._pSplTHotKey);

		mem_free_dbg(LoadBuff);
		tbuff = oldtbuff;
	}
}

void RepeatClicks()
{
	switch (sgbMouseDown) {
	case 1: {
		if ((SDL_GetModState() & KMOD_SHIFT)) {
			if (plr[myplr]._pwtype == WT_RANGED) {
				NetSendCmdLoc(TRUE, CMD_RATTACKXY, cursmx, cursmy);
			} else {
				NetSendCmdLoc(TRUE, CMD_SATTACKXY, cursmx, cursmy);
			}
		} else {
			NetSendCmdLoc(TRUE, CMD_WALKXY, cursmx, cursmy);
		}
		break;
	}
	case 2: {
		/*
		repeated casting disabled for spells that change cursor and ones that wouldn't benefit from casting them more than 1 time
		it has to be done here, otherwise there is a delay between casting a spell and changing the cursor, during which more casts get queued
		*/
		int spl = plr[myplr]._pRSpell;
		if (spl != SPL_TELEKINESIS &&
			spl != SPL_RESURRECT &&
			spl != SPL_HEALOTHER &&
			spl != SPL_IDENTIFY &&
			spl != SPL_RECHARGE &&
			spl != SPL_DISARM &&
			spl != SPL_REPAIR &&
			spl != SPL_GOLEM &&
			spl != SPL_INFRA &&
			spl != SPL_TOWN
			) 
			CheckPlrSpell();
		break;
	}
	}
}

DEVILUTION_END_NAMESPACE
