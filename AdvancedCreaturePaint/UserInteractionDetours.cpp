#include "stdafx.h"
#include "UserInteractionDetours.h"
#include "RigblockPaintList.h"
#include "PaintBrushCategoryWinProc.h"

// We use this file for the user-side of the mod: the paint categories, clicking on parts, etc

const uint32_t ACP_CATEGORY_ID = id("ACP_ce_category_paintbrush");
const uint32_t ACP_PAINT_DEFAULT = id("VE_Old_Car_Matte_01");
enum PartMode {
	Single,
	Symmetric,
	Similar,
	Categorical // TODO: allow painting by rigblock type via Ctrl + Shift
};

// These are variables we need to keep; important we clean any EASTL structures on shutdown!
Graphics::Model* sLastClickedRigblock = nullptr;
PaintBrushCategoryWinProc* sCategoryWinProc = nullptr;
Clock sLastClickClock;
bool bHoveredCreature = false;
bool bHoveredPart = false;
bool bKeyCtrlDown = false;
bool bKeyShiftDown = false;
PartMode partPaintMode = Single;



// Checks if the Editor is currently in the advanced creature paint category
bool IsAdvancedPaintCategory()
{
	if (!Editor.IsMode(Editors::Mode::PaintMode)) return false;
	auto activeCategory = Editor.mpPaintPaletteUI->mpActiveCategory;
	return activeCategory && activeCategory->mpCategory->mCategoryID == ACP_CATEGORY_ID;
}


void PaletteCategoryUI_LayoutPagePanel__detour::detoured()
{
	// At this point the PaletteCategoryUI has some windows, but it still hasn't inserted the pages
	// It is important to do this here, because we need to scale mpPagePanel BEFORE it adds items to it

	original_function(this);

	if (this->mpCategory->mCategoryID == ACP_CATEGORY_ID)
	{
		sCategoryWinProc = new PaintBrushCategoryWinProc();
		sCategoryWinProc->AddToCategoryUI(this);
	}
}


bool Editor_HandleMessage__detour::detoured(uint32_t messageID, void* msg)
{
	// This message is sent when clicking on a paint or a color, and by default it repaints the creature
	// If we are in advanced paint mode we don't want that
	if (messageID == Editors::ColorChangedMessage::ID)
	{
		if (IsAdvancedPaintCategory())
		{
			return true;
		}
		else
		{
			return original_function(this, messageID, msg);
		}
	}
	else
	{
		return original_function(this, messageID, msg);
	}
}

Graphics::Model* IntersectPaintableRigblockModel(float mouseX, float mouseY)
{
	Vector3 point, direction;
	GameModeManager.GetViewer()->GetCameraToPoint(mouseX, mouseY, point, direction);

	// Find what model the user is pointing to, but only consider rigblocks
	Graphics::FilterSettings filter;
	filter.SetRequiredGroup((uint32_t)Graphics::ModelGroups::Rigblock);
	filter.SetExcludedGroup((uint32_t)Graphics::ModelGroups::Vertebra);
	return Editor.mpMainModelWorld->FindFirstModelAlongLine(
		point, point + 1000.0f * direction, nullptr, nullptr, nullptr, filter);
}

bool Editor_OnMouseDown__detour::detoured(MouseButton mouseButton, float mouseX, float mouseY, MouseState mouseState)
{
	sLastClickedRigblock = nullptr;
	if (IsAdvancedPaintCategory() && mouseButton == MouseButton::kMouseButtonLeft)
	{
		sLastClickedRigblock = IntersectPaintableRigblockModel(mouseX, mouseY);
		if (sLastClickedRigblock)
		{
			sLastClickClock = Clock(Clock::Mode::Milliseconds);
			sLastClickClock.Start();
			bHoveredPart = true;
			bKeyCtrlDown = mouseState.IsCtrlDown;
			bKeyShiftDown = mouseState.IsShiftDown;
			CursorManager.SetActiveCursor(0x958A6A36);
		}
	}
	return original_function(this, mouseButton, mouseX, mouseY, mouseState);
}

void SetRigblockPaint(
	Editors::EditorRigblock* rigblock, 
	int rigblockIndex,
	const Editors::EditorRigblockPaint& paint, 
	int skinpaintIndex)
{
	bool assigned = false;
	for (unsigned int i = 0; i < rigblock->mPaints.size(); ++i)
	{
		if (rigblock->mPaints[i].first == skinpaintIndex) {
			rigblock->mPaints[i].second = paint;
			assigned = true;
			break;
		}
	}
	if (!assigned)
	{
		rigblock->mPaints.push_back({ skinpaintIndex, paint });
	}


	auto creatureData = (AdvancedCreatureDataResource*)(Editor.GetSkin()->GetMesh()->mpCreatureData);
	AdvancedCreatureDataResource::PaintInfo paintInfo;
	paintInfo.rigblockIndex = rigblockIndex;
	paintInfo.paint.region = skinpaintIndex;
	paintInfo.FromRigblockPaint(paint);
	creatureData->SetPaintInfo(paintInfo);
}

void RemoveRigblockPaint(
	Editors::EditorRigblock* rigblock,
	int rigblockIndex,
	int skinpaintIndex)
{
	for (unsigned int i = 0; i < rigblock->mPaints.size(); ++i)
	{
		if (rigblock->mPaints[i].first == skinpaintIndex) {
			rigblock->mPaints.erase(rigblock->mPaints.begin() + i);
			break;
		}
	}

	auto creatureData = (AdvancedCreatureDataResource*)(Editor.GetSkin()->GetMesh()->mpCreatureData);
	creatureData->RemovePaintInfo(rigblockIndex, skinpaintIndex);
}

bool Editor_OnMouseUp__detour::detoured(MouseButton mouseButton, float mouseX, float mouseY, MouseState mouseState)
{
	if (IsAdvancedPaintCategory() && mouseButton == MouseButton::kMouseButtonLeft &&
		sLastClickClock.GetElapsedTime() < 500 &&  // accept clicks within half a second
		Editor.GetSkin() && Editor.GetSkin()->GetMesh() && Editor.GetEditorModel())
	{
		bHoveredPart = false;
		CursorManager.SetActiveCursor(0x958A6A35);
		auto model = IntersectPaintableRigblockModel(mouseX, mouseY);
		if (model)
		{
			uint32_t instanceID, groupID;
			Editor.mpMainModelWorld->GetModelResourceIDs(model, &instanceID, &groupID);
		}

		// If the model is the same as when the user clicked down
		if (model == sLastClickedRigblock)
		{
			EditorRigblockPtr rigblock = nullptr;
			if (model && model->GetOwner())
			{
				rigblock = object_cast<Editors::EditorRigblock>(model->GetOwner());
			}

			// Even though we excluded them in the filter, apparently sometimes it still detects vertebras
			if (rigblock && !rigblock->mBooleanAttributes[Editors::EditorRigblockBoolAttributes::kEditorRigblockModelIsVertebra])
			{
				// Find the main rigblock index
				auto& rigblocks = Editor.GetSkin()->GetMesh()->mRigblocks;
				auto it = eastl::find(rigblocks.begin(), rigblocks.end(), rigblock);
				if (it == rigblocks.end()) {
					SporeDebugPrint("ERROR: rigblock not found in EditorModel::mRigblocks");
				}
				int rigblockIndex = eastl::distance(rigblocks.begin(), it);

				eastl::vector<int> regions;
				if (sCategoryWinProc->RegionIsSelected(PaintBrushCategoryWinProc::kRegionBase)) regions.push_back(0);
				if (sCategoryWinProc->RegionIsSelected(PaintBrushCategoryWinProc::kRegionCoat)) regions.push_back(1);
				if (sCategoryWinProc->RegionIsSelected(PaintBrushCategoryWinProc::kRegionDetail)) regions.push_back(2);
				if (sCategoryWinProc->RegionIsSelected(PaintBrushCategoryWinProc::kRegionIdentity)) regions.push_back(3);
				if (sCategoryWinProc->RegionIsSelected(PaintBrushCategoryWinProc::kRegionTextured)) regions.push_back(4);

				bool bApplyPaint = sCategoryWinProc->GetActiveMode() == PaintBrushCategoryWinProc::Mode::Paint;

				partPaintMode = Single;
				if (mouseState.IsCtrlDown && bKeyCtrlDown == bool(mouseState.IsCtrlDown)) {
					partPaintMode = Symmetric;
				}
				if (mouseState.IsShiftDown && bKeyShiftDown == bool(mouseState.IsShiftDown)) {
					if (partPaintMode == Single) {
						partPaintMode = Similar;
					}
					if (partPaintMode == Symmetric) {
						partPaintMode = Categorical;
					}
				}

				{
					Editors::EditorRigblockPaint paint = Editor.mpPaintPaletteUI->mpActiveCategory->GetSelectedRigblockPaint();
					// fill in the paint data if it is not set, to prevent glitched normals
					if (paint.mPaintID == 0x0) {
						paint.mPaintID = ACP_PAINT_DEFAULT;
					}
					

					// Paint one part
					if (partPaintMode == Single) {
						for (int region : regions)
						{
							if (bApplyPaint) {
								SetRigblockPaint(rigblock.get(), rigblockIndex, paint, region);
							}
							else {
								RemoveRigblockPaint(rigblock.get(), rigblockIndex, region);
							}
							
						}
					}
					// Paint 2 parts
					else if (partPaintMode == Symmetric) {
						// Use the rigblock index to get the symmetric block index and rigblock
						auto blockdata = Editor.GetSkin()->GetMesh()->mpCreatureData->mRigblocks[rigblockIndex];
						auto rigblockSymmIndex = blockdata.mSymmetricIndex;
						
						// Found symmetric block
						if (rigblockSymmIndex != -1) {
							auto rigblockSymmetric = rigblocks[rigblockSymmIndex];
							for (int region : regions)
							{
								if (bApplyPaint) {
									SetRigblockPaint(rigblock.get(), rigblockIndex, paint, region);
									SetRigblockPaint(rigblockSymmetric.get(), rigblockSymmIndex, paint, region);
								}
								else {
									RemoveRigblockPaint(rigblock.get(), rigblockIndex, region);
									RemoveRigblockPaint(rigblockSymmetric.get(), rigblockSymmIndex, region);
								}
							}
						}
						// No symmetric block
						else {
							for (int region : regions)
							{
								if (bApplyPaint) {
									SetRigblockPaint(rigblock.get(), rigblockIndex, paint, region);
								}
								else {
									RemoveRigblockPaint(rigblock.get(), rigblockIndex, region);
								}
							}
						}

						

					}
					// Paint all similar parts
					else if (partPaintMode == Similar) {
						for (auto block : GetSimilarRigblocks(rigblock)) {
							// Find the index for each similar rigblock
							auto localitem = eastl::find(rigblocks.begin(), rigblocks.end(), block);
							if (localitem == rigblocks.end()) {
								SporeDebugPrint("ERROR: rigblock not found in EditorModel::mRigblocks");
							}
							int localRigblockIndex = eastl::distance(rigblocks.begin(), localitem);

							for (int region : regions)
							{
								if (bApplyPaint) {
									SetRigblockPaint(block, localRigblockIndex, paint, region);
								}
								else {
									RemoveRigblockPaint(block, localRigblockIndex, region);
								}
							}
						}
					}
					// Paint all parts in the same category / rigblock type
					else if (partPaintMode == Categorical) {
						for (auto block : GetCategoricalRigblocks(rigblock)) {
							// Find the index for each similar rigblock
							auto localitem = eastl::find(rigblocks.begin(), rigblocks.end(), block);
							if (localitem == rigblocks.end()) {
								SporeDebugPrint("ERROR: rigblock not found in EditorModel::mRigblocks");
							}
							int localRigblockIndex = eastl::distance(rigblocks.begin(), localitem);

							for (int region : regions)
							{
								if (bApplyPaint) {
									SetRigblockPaint(block, localRigblockIndex, paint, region);
								}
								else {
									RemoveRigblockPaint(block, localRigblockIndex, region);
								}
							}
						}
					}

					
				}

				Editor.field_4B2 = true;
				Editor.GetSkin()->PaintSkin(Editor.GetEditorModel());

				Editor.CommitEditHistory(true);
			}
		}
	}
	sLastClickedRigblock = nullptr;
	// We want the original function anyways because otherwise the camera doesn't work correctly
	return original_function(this, mouseButton, mouseX, mouseY, mouseState);
}

// Detour the cursor setting func
bool SetCursor__detour::detoured(uint32_t id) {
	if (IsAdvancedPaintCategory())
	{
		if (bHoveredCreature) {
			id = 0x958A6A35;
			if (bHoveredPart) {
				id = 0x958A6A36;
			}
		}
	}
	return original_function(this, id);
}

// We use this detour to ensure the creature doesn't move while using the advanced creature paint
void Editor_Update__detour::detoured(float delta1, float delta2)
{
	original_function(this, delta1, delta2);

	if (IsAdvancedPaintCategory())
	{
		Vector3 point, direction;
		GameModeManager.GetViewer()->GetCameraToPoint(mMousePosition.x, mMousePosition.y, point, direction);

		// If the user moves the mouse into the creature, stop its animation so the user can edit it easily
		Graphics::FilterSettings filter;
		filter.collisionMode = Graphics::CollisionMode::BoundingBox;
		filter.SetRequiredGroup((uint32_t)Graphics::ModelGroups::AnimatedCreature);
		auto model = Editor.mpMainModelWorld->FindFirstModelAlongLine(
			point, point + 1000.0f * direction, nullptr, nullptr, nullptr, filter);

		bHoveredCreature = bool(model);
		if (model)
		{
			if (bHoveredCreature && !bHoveredPart) {
				CursorManager.SetActiveCursor(0x958A6A35);
			}
			cEditorAnimEventPtr animEvent = new Editors::cEditorAnimEvent();
			animEvent->MessagePost(0x6581B78E, 0, Editor.GetEditorModel());
		}
	}
}