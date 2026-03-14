#pragma once
#include "i_gui_panel.h"
#include "../../glframework/renderer/bloom.h"
#include "../../glframework/material/screenMaterial.h"
#include "../../glframework/core.h"
#include "../../application/camera/GameCameraControl.h"

// RenderingPanel: render mode, exposure, bloom, clear/ambient color, camera speed,
// and optional debug axis/grid overlay toggle.
class RenderingPanel : public IGuiPanel {
public:
	RenderingPanel(
		int&               renderModeIdx,
		glm::vec3&         clearColor,
		glm::vec3&         ambientColor,
		ScreenMaterial*    screenMat,
		Bloom*             bloom,
		GameCameraControl* cameraControl,
		bool&              showDebugAxis
	);

	void onRender() override;

private:
	int&               mRenderModeIdx;
	glm::vec3&         mClearColor;
	glm::vec3&         mAmbientColor;
	ScreenMaterial*    mScreenMat;
	Bloom*             mBloom;
	GameCameraControl* mCameraControl;
	bool&              mShowDebugAxis;
};
