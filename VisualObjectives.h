#pragma once

namespace VisualObjectives
{
	bool Init();
	void Update();
	Tile* mainTile = NULL;
	Tile* playerMarkerTile = NULL;
	Tile* compassTile = NULL;
	Tile* hudMenuTile;
	Tile* jvoRect = NULL;
	bool JVOVisible = false;
	bool initialized = false;
	int objectiveIndex = 0;
	int lastIndex = 0;

	bool InjectMenuXML(Menu* menu)
	{
		if (!menu) return false;
		hudMenuTile = menu->tile;

		mainTile = hudMenuTile->ReadXML("data\\menus\\prefabs\\JVO\\JVO.xml");
		return mainTile != NULL;
	}
	float GetObjectDimensions(TESObjectREFR* ref, int axis) {
		float scale = ref->scale;
		if (ref->IsActor())
		{
			Actor* actor = (Actor*)ref;
			if (actor->baseProcess && (actor->baseProcess->processLevel <= 1))
			{
				BSBound* bounds = ((MiddleHighProcess*)actor->baseProcess)->boundingBox;
				if (bounds) return (bounds->dimensions[axis] * 2 * scale);
			}
		}
		return 0.0;
	}

	bool Init() {
		HUDMainMenu* hud = HUDMainMenu::GetSingleton();

		if (!InjectMenuXML(hud))
		{
			MessageBoxA(nullptr, "Visual Objectives failed to initialize", "JVO", MB_ICONERROR);
			return false;
		}
		playerMarkerTile = mainTile->GetChild("JVOPlayerMarker");
		Tile* hitPoints = hudMenuTile->GetChild("HitPoints");
		compassTile = hitPoints->GetChild("compass_window");
		jvoRect = hud->AddTileFromTemplate(mainTile, "JVOTemp", 0);
		initialized = true;
		return true;
	}
	void setVisible(bool isVisible) {
		SetTileComponentValue(mainTile, "_JVOVisible", isVisible ? 1 : 0);
		mainTile->SetFloat(kTileValue_visible, isVisible, 1);
	}
	void AddVisualObjective(TESObjectREFR* ref) {
		HUDMainMenu* hud = HUDMainMenu::GetSingleton();
		char objTileName[15];
		sprintf(objTileName, "JVOMarker%i", objectiveIndex);
		Tile* objectiveTile = jvoRect->GetChild(objTileName);
		if (objectiveTile == nullptr) {
			objectiveTile = hud->AddTileFromTemplate(jvoRect, "JVOMarker", 0);
			objectiveTile->name.Set(objTileName);
		}
		objectiveTile->SetFloat(kTileValue_visible, 1, true);
		float distance = g_thePlayer->GetDistance(ref);
		float dX = 0, dY = 0, dZ = 0;
		if (ref->IsActor()) {
			NiAVObject* niBlock = ref->GetNiBlock("Bip01 Head");
			if (niBlock && &niBlock->m_worldTranslate) {
				NiVector3& transltn = niBlock->m_worldTranslate;
				NiPoint3* pos = ref->GetPos();
				dX = transltn.x - pos->x;
				dY = transltn.y - pos->y;
				dZ = transltn.z - pos->z + 20;
			}
		}
		else {
			dX = 0;
			dY = 0;
			dZ = 0.5 * GetObjectDimensions(ref, 2) + 10;
		}
		NiPoint3 w2s(ref->posX + dX, ref->posY + dY, ref->posZ + dZ);
		float x, y, z;
		bool isOnScreen = WorldToScreen(&w2s, x, y, z, 2);
		if (!isOnScreen) {
			x = -1;
			y = -1;
		}
		SetTileComponentValue(objectiveTile, "_X", x);
		SetTileComponentValue(objectiveTile, "_Y", y);
		SetTileComponentValue(objectiveTile, "_JVOInDistance", 1);
		float inFocus = objectiveTile->GetComponentValue("_JVOInFocus")->num;
		char distanceText[50];
		if (inFocus + 2 > 1) {
			if (distance > 1000000) {
				strcpy(distanceText, "Far away");
			}
			else {
				sprintf(distanceText, "%.f m.", distance / 69.99104);
			}
		}
		SetTileComponentValue(objectiveTile, "_JVODistance", distanceText);
		char text[50];
		strcpy(text, ref->GetTheName());
		SetTileComponentValue(objectiveTile, "_JVOText", text);
		int isHostileColor = 0;
		double shouldAttack = 0;
		if (ref->IsActor()) {
			if (!((Actor*)ref)->GetDead() && ((Actor*)g_thePlayer)->IsSneaking() && !ref->IsCreature()) {
				isHostileColor = 1;
			}
			else if ((CdeclCall<bool>(0x50FF60, (Actor*)ref, (Actor*)g_thePlayer, 0, &shouldAttack) && shouldAttack > 0) || ((Actor*)ref)->IsInCombatWith((Actor*)g_thePlayer)) {
				isHostileColor = 1;
			}
			else if (IsFactionEnemy((Actor*)ref, (Actor*)g_thePlayer) || IsFactionEnemy((Actor*)g_thePlayer, (Actor*)ref)) {
				isHostileColor = 1;
			}
		}
		else if (ref->baseForm->typeID >= 0x15 && ref->baseForm->typeID < 0x29) {
			TESForm* refOwner = nullptr, * cellOwner = nullptr;
			ExtraOwnership* xOwn = (ExtraOwnership*)ref->extraDataList.GetByType(kExtraData_Ownership);
			if (xOwn) refOwner = xOwn->owner;
			if (ref->parentCell && ref->parentCell->IsInterior()) {
				xOwn = (ExtraOwnership*)ref->parentCell->extraDataList.GetByType(kExtraData_Ownership);
				if (xOwn) cellOwner = xOwn->owner;
			}
			if (!refOwner && cellOwner != nullptr) {
				if ((cellOwner->refID != 0x1B2A4 && cellOwner->refID != 0x7) || (cellOwner->typeID == 0x8 && ThisCall<bool>(0x6F8FF0, (Actor*)g_thePlayer, cellOwner))) {
					isHostileColor = 1;
				}
			}
			else if (refOwner) {
				if ((refOwner->refID != 0x1B2A4 && refOwner->refID != 0x7) || (refOwner->typeID == 0x8 && ThisCall<bool>(0x6F8FF0, (Actor*)g_thePlayer, refOwner))) {
					isHostileColor = 1;
				}
			}
			if (ref->baseForm->typeID == 0x1C) {
				ExtraLock* xLock = (ExtraLock*)ref->extraDataList.GetByType(kExtraData_Lock);
				if (xLock && xLock->data->flags & 1 == 0) isHostileColor = 0;
			}
		}
		SetTileComponentValue(objectiveTile, "_JVOHostile", isHostileColor);
		objectiveIndex++;
		JVOVisible = true;

	}
	void Update() {

		if (g_interfaceManager->currentMode == 2) JVOVisible = false;
		setVisible(JVOVisible);
		SetTileComponentValue(mainTile, "_JVOInCombat", g_thePlayer->pcInCombat ? 1 : 0);
		SetTileComponentValue(mainTile, "_JVOAlphaCW", compassTile->GetValueFloat(kTileValue_alpha));
		TESObjectREFR* customMarker = g_thePlayer->unk66C ? *(g_thePlayer->unk668) : g_thePlayer->unk650;
		if (customMarker) {
			JVOVisible = true;
			float deltaZ = 0.0;
			if (!customMarker->GetNiNode()) {
				NiPoint3* pos = customMarker->GetPos();
				float posXY[2] = { pos->x, pos->y };
				g_TES->GetTerrainHeight(posXY, &deltaZ);
				if (deltaZ <= 0.0) deltaZ = g_thePlayer->GetPos()->z;
			}
			else {
				deltaZ = 0.5 * GetObjectDimensions(customMarker, 2) + 10;
			}
			NiPoint3 w2s(customMarker->posX, customMarker->posY, customMarker->posZ + deltaZ);
			float x, y, z;
			bool isOnScreen = WorldToScreen(&w2s, x, y, z, 2);
			if (!isOnScreen) {
				x = -1;
				y = -1;
			}
			SetTileComponentValue(playerMarkerTile, "_X", x);
			SetTileComponentValue(playerMarkerTile, "_Y", y);
			SetTileComponentValue(playerMarkerTile, "_JVOInDistance", 1);
			float distance = GetDistance2D(g_thePlayer, customMarker);
			float inFocus = playerMarkerTile->GetComponentValue("_JVOInFocus")->num;
			char distanceText[50];
			if (inFocus + 2 > 1) {
				if (distance > 1000000) {
					strcpy(distanceText, "Far away");
				}
				else {
					sprintf(distanceText, "%.f m.", distance / 69.99104);
				}
			}
			SetTileComponentValue(playerMarkerTile, "_JVODistance", distanceText);
		}
		tList <BGSQuestObjective::Target>* targets = ThisCall<tList<BGSQuestObjective::Target>*>(0x76CF30, g_thePlayer);
		if (targets)
		{

			for (tList<BGSQuestObjective::Target>::Iterator iter = targets->Begin(); !iter.End(); ++iter)
			{

				BGSQuestObjective::Target* data = iter.Get();
				if (data)
				{
					BGSQuestObjective::Target::Data* objective = &data->data;
					if (objective->teleportLinks.size == 0) {
						AddVisualObjective(data->target);
					}
					else {
						AddVisualObjective(objective->teleportLinks.data[0].door);
					}

				}
			}

		}
		int i = targets == nullptr ? 0 : targets->Count();
		int j = jvoRect->children.Size();
		if (j > i) {
			for (j; j > i; j--) {
				auto child = jvoRect->GetNthChild(j - 1);
				if (child && child->data) {
					child->data->SetFloat(kTileValue_visible, 0);
				}
			}
		}

		objectiveIndex = 0;
	}
	Tile* __cdecl LoadingScreenHook(int id) {
		if (initialized) setVisible(false);
		return CdeclCall<Tile*>(0xBEAF00, id);
	}
}

