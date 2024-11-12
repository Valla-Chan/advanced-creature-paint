#pragma once
#include "stdafx.h"

// TODO: allow painting by rigblock type via Ctrl + Shift
enum PartType {
	Limb,
	Mouth,
	Eye,
	Hand,
	Foot,
	WeaponCharge,
	WeaponSpit,
	WeaponStrike,
	Wing,
	Fin,
	Captain,
	DI_Armor,
	DI_Mask,
	DI_wBlade,
	DI_wStaff,
	DI_wGun,
	DI_wMisc,
	Detail,
	Hat,
	Mask,
	Shoulder,
	Chest,
	Symbol,
	OutfitDetail,
};

// Return vector of rigblocks with the same texture and name
vector<Editors::EditorRigblock*> GetSimilarRigblocks(EditorRigblockPtr rigblock) {
	vector<Editors::EditorRigblock*> rigblockList = {};
	auto model = Editor.GetEditorModel();

	LocalizedString mBlockname;
	uint32_t mSkinDiffuseID;
	uint32_t mSkinTintID;
	ResourceKey mCenterKey;

	// Store the main properties we want to compare.
	PropertyListPtr mpMainPropList = rigblock->mpPropList;
	if (mpMainPropList)
	{
		App::Property::GetText(mpMainPropList.get(), 0x8F6FC401, mBlockname); // blockname
		App::Property::GetKeyInstanceID(mpMainPropList.get(), 0x02424655, mSkinDiffuseID); // skinpaintDiffuseTexture
		App::Property::GetKeyInstanceID(mpMainPropList.get(), 0x02424657, mSkinTintID); // skinpaintTintMaskTexture
		App::Property::GetKey(mpMainPropList.get(), 0x3A3B9D21, mCenterKey); // modelCenterFile

		for (auto block : model->mRigblocks) {
			// if this is the same rigblock as the ref, add it to the list.
			if (block == rigblock) {
				rigblockList.push_back(block.get());
				continue;
			}

			LocalizedString mLocalBlockname;
			uint32_t mLocalSkinDiffuseID;
			uint32_t mLocalSkinTintID;

			PropertyListPtr mpPropList = block->mpPropList;
			if (mpPropList)
			{
				App::Property::GetText(mpPropList.get(), 0x8F6FC401, mLocalBlockname); // blockname
				App::Property::GetKeyInstanceID(mpPropList.get(), 0x02424655, mLocalSkinDiffuseID); // skinpaintDiffuseTexture
				App::Property::GetKeyInstanceID(mpPropList.get(), 0x02424657, mLocalSkinTintID); // skinpaintTintMaskTexture

				// Compare values to see if this is probably the same rigblock
				// Name matches
				if (mBlockname.GetText() == mLocalBlockname.GetText()) {

					// Texture matches
					if (mSkinDiffuseID == mLocalSkinDiffuseID && mSkinTintID == mLocalSkinTintID) {
						// Comparison succeeded, add to list.
						rigblockList.push_back(block.get());
					}

					// Texture does not match, may be a center variant
					else {
						uint32_t mMainCenterModel;
						uint32_t mThisModel;
						// Load center file resource
						PropertyListPtr mpPropListCenter;
						if (PropManager.GetPropertyList(mCenterKey.instanceID, mCenterKey.groupID, mpPropListCenter))
						{
							App::Property::GetKeyInstanceID(mpPropListCenter.get(), 0x00F9EFBB, mMainCenterModel); // modelMeshLOD0 in original part's center variant
							App::Property::GetKeyInstanceID(mpPropList.get(), 0x00F9EFBB, mThisModel); // modelMeshLOD0 in this part

							if (mMainCenterModel == mThisModel) {
								// Center comparison succeeded, add to list.
								rigblockList.push_back(block.get());
							}
						}
					}
				}

			}
		}
	}

	return rigblockList;
}

// TODO
/*
vector<Editors::EditorRigblock*> GetCategoricalRigblocks(EditorRigblockPtr rigblock) {
	vector<Editors::EditorRigblock*> rigblockList = {};
	auto model = Editor.GetEditorModel();

	LocalizedString mBlockname;
	uint32_t mSkinDiffuseID;
	uint32_t mSkinTintID;
	ResourceKey mCenterKey;

	// Categorize the reference rigblock
	PropertyListPtr mpMainPropList = rigblock->mpPropList;
	if (mpMainPropList)
	{

	}

	return rigblockList;
}*/