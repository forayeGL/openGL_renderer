#pragma once
#include "i_gui_panel.h"
#include "../../glframework/renderer/bloom.h"
#include "../../glframework/material/screenMaterial.h"
#include "../../glframework/core.h"
#include "../../application/camera/GameCameraControl.h"

// RenderingPanel: render mode, exposure, bloom, clear/ambient color, camera speed.
class RenderingPanel : public IGuiPanel {
public:
	RenderingPanel(
		int&               renderModeIdx,
		int&               shadowType,
		glm::vec3&         clearColor,
		glm::vec3&         ambientColor,
       bool&              enableTAA,
		float&             taaBlendFactor,
		bool&              taaNeighborhoodClamp,
		ScreenMaterial*    screenMat,
		Bloom*             bloom,
		GameCameraControl* cameraControl
	);

	void onRender() override;

private:
	int&               mRenderModeIdx;
	int&               mShadowType;
	glm::vec3&         mClearColor;
	glm::vec3&         mAmbientColor;
  bool&              mEnableTAA;
	float&             mTaaBlendFactor;
	bool&              mTaaNeighborhoodClamp;
	ScreenMaterial*    mScreenMat;
	Bloom*             mBloom;
	GameCameraControl* mCameraControl;
};
