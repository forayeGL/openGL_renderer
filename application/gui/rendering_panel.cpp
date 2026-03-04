#include "rendering_panel.h"
#include "../../imgui/imgui.h"

RenderingPanel::RenderingPanel(
	int&               renderModeIdx,
	glm::vec3&         clearColor,
	glm::vec3&         ambientColor,
	ScreenMaterial*    screenMat,
	Bloom*             bloom,
	GameCameraControl* cameraControl
)
	: mRenderModeIdx(renderModeIdx)
	, mClearColor(clearColor)
	, mAmbientColor(ambientColor)
	, mScreenMat(screenMat)
	, mBloom(bloom)
	, mCameraControl(cameraControl)
{}

void RenderingPanel::onRender() {
	ImGui::Begin("Rendering");

	const char* modeNames[] = { "Fill", "Wireframe", "Shadow Only" };
	ImGui::Combo("Render Mode", &mRenderModeIdx, modeNames, 3);

	ImGui::SliderFloat("Exposure",      &mScreenMat->mExposure, 0.0f, 10.0f);
	ImGui::ColorEdit3("Clear Color",    &mClearColor.x);
	ImGui::ColorEdit3("Ambient Color",  &mAmbientColor.x);

	if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Threshold",   &mBloom->mThreshold,        0.0f, 100.0f);
		ImGui::SliderFloat("Attenuation", &mBloom->mBloomAttenuation, 0.0f, 1.0f);
		ImGui::SliderFloat("Intensity",   &mBloom->mBloomIntensity,   0.0f, 1.0f);
		ImGui::SliderFloat("Radius",      &mBloom->mBloomRadius,      0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("Camera")) {
		float speed = mCameraControl->getSpeed();
		if (ImGui::SliderFloat("Move Speed", &speed, 0.01f, 2.0f)) {
			mCameraControl->setSpeed(speed);
		}
	}

	ImGui::End();
}
