#pragma once
#include "stdafx.h"

using namespace Palettes;

const uint32_t kACPPaletteGroupID = id("mineral_paint_palette");
const vector<uint32_t> kSaveTypesSkin = {
	TypeIDs::crt,
	TypeIDs::cll,
	TypeIDs::flr,
};

bool bLoadedPaintPalettes = false;
eastl::vector<ResourceKey> ACPPaintModules = {};

void LoadACPPalettes() {
	// pull module prop keys from kACPPaletteGroupID folder
	ResourceManager.GetRecordKeyList(ACPPaintModules,
		&Resource::StandardFileFilter(
			ResourceKey::kWildcardID,
			kACPPaletteGroupID,
			TypeIDs::prop,
			ResourceKey::kWildcardID
		)
	);
}

static bool IsSkinEditor() {
	return Editors::GetEditor() && kSaveTypesSkin.find(Editor.mSaveExtension) > -1;
}

//------------------
// Fix Paths
//------------------

// Convert all paintbrush categories to use the relevant ACP / building paintbrush category
member_detour(PalettePageLoad__detour, PalettePage, bool(const ResourceKey&, uint32_t, uint32_t, uint32_t, uint32_t))
{
	bool detoured(const ResourceKey& name, uint32_t thumbnailGroupID, uint32_t arg_8, uint32_t layoutID, uint32_t arg_10) {
		if (original_function(this, name, thumbnailGroupID, arg_8, layoutID, arg_10)) {
			// Convert vehicle paintbrush category into building paintbrush category
			if (this->mParentCategory.instanceID == id("ve_category_paintbrush_mode")) {
				this->mParentCategory.groupID = 0x406B6E29; // building_paint_palette_category~
				this->mParentCategory.instanceID = id("be_category_paintbrush_mode");
			}
			// Convert building paint pages to use ACP category if in the organic editors
			if (this->mParentCategory.instanceID == id("be_category_paintbrush_mode") && IsSkinEditor()) {
				this->mParentCategory.groupID = 0x406B6E0D; // creature_paint_palette_category~
				this->mParentCategory.instanceID = id("ACP_ce_category_paintbrush");
			}
			return true;
		}
		return false;
	}
};

// Make all mineral / ACP block paints read from buildingpainticons~
member_detour(PaletteItem_Load__detour, PaletteItem, bool(const ResourceKey&, int, uint32_t)) {
	bool detoured(const ResourceKey& name, int nPagePartPriority, uint32_t thumbnailGroupID)
	{
		if (original_function(this, name, nPagePartPriority, thumbnailGroupID)) {
			if (this->mTypeID == PaletteItem::ItemType::kItemBlockPaint) {
				this->mThumbnailName.groupID = 0xB10F04D4; // buildingpainticons~
			}
			return true;
		}
		return false;
	}
};

//------------------
// Load Palettes
//------------------

// Detect when paint palette is loaded and reset bLoadedPaintPalettes
member_detour(PaletteReadProp__detour, PaletteMain, void(const ResourceKey&, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)) {
	void detoured(const ResourceKey& name, uint32_t creationTypeID, uint32_t arg_8,
			uint32_t layoutID, uint32_t categoryLayoutID, uint32_t pageLayoutID, uint32_t arg_18)
	{
		bLoadedPaintPalettes = false;
		original_function(this, name, creationTypeID, arg_8, layoutID, categoryLayoutID, pageLayoutID, arg_18);
	}
};

// Load additional ACP paint palette
member_detour(PaletteReadModuleProp__detour, PaletteMain, void(const ResourceKey&)) {
	void PaletteReadModuleProp__detour::detoured(const ResourceKey& name)
	{
		original_function(this, name);

		if (!bLoadedPaintPalettes) {
			PropertyListPtr pPropList;
			if (PropManager.GetPropertyList(name.instanceID, name.groupID, pPropList))
			{
				bool bLoadACP;
				if (App::Property::GetBool(pPropList.get(), id("loadMineralPaints"), bLoadACP) && bLoadACP) {
					LoadACPPalettes();
				}
			}
		}
	}

	void PaletteReadModuleProp__detour::LoadACPPalettes()
	{
		for (size_t i = 0; i < ACPPaintModules.size(); i++) {
			// Don't load the building editor paint palette in the vehicle/building editor (prevents loading pages twice)
			if (Editors::GetEditor() && Editor.mSaveExtension == TypeIDs::vcl && Editor.mSaveExtension == TypeIDs::bld && ACPPaintModules[i].instanceID == id("ACP_be_paint_palette"))
				continue;

			original_function(this, ACPPaintModules[i]);
		}

		bLoadedPaintPalettes = true;
	}
};