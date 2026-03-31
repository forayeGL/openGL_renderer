#include "scene_panel.h"
#include "../../imgui/imgui.h"
#include "../../glframework/material/advanced/pbrMaterial.h"
#include "../../glframework/material/grassInstanceMaterial.h"

ScenePanel::ScenePanel(
	std::vector<Mesh*>& dynamicObjects,
	int&                selectedObject,
	AddObjectFn         addFn,
	RemoveObjectFn      removeFn
)
	: mDynamicObjects(dynamicObjects)
	, mSelectedObject(selectedObject)
	, mAddFn(std::move(addFn))
	, mRemoveFn(std::move(removeFn))
{}

void ScenePanel::onRender() {
	ImGui::Begin("Scene");
	ImGui::Text("Objects: %d", (int)mDynamicObjects.size());

	if (ImGui::Button("Add Cube"))   { mAddFn("cube"); }
	ImGui::SameLine();
	if (ImGui::Button("Add Sphere")) { mAddFn("sphere"); }

	ImGui::Separator();

	for (int i = 0; i < (int)mDynamicObjects.size(); i++) {
		char label[64];
		snprintf(label, sizeof(label), "Object %d", i);
		if (ImGui::Selectable(label, mSelectedObject == i)) {
			mSelectedObject = i;
		}
	}

	if (mSelectedObject >= 0 && mSelectedObject < (int)mDynamicObjects.size()) {
		ImGui::Separator();
		ImGui::Text("Selected: Object %d", mSelectedObject);
		auto* obj = mDynamicObjects[mSelectedObject];

		glm::vec3 pos = obj->getPosition();
		if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
			obj->setPosition(pos);
		}

		float angleX = 0.0f, angleY = 0.0f, angleZ = 0.0f;
		if (ImGui::DragFloat("Rotate X", &angleX, 1.0f)) { obj->rotateX(angleX); }
		if (ImGui::DragFloat("Rotate Y", &angleY, 1.0f)) { obj->rotateY(angleY); }
		if (ImGui::DragFloat("Rotate Z", &angleZ, 1.0f)) { obj->rotateZ(angleZ); }

		auto* pbr = dynamic_cast<PbrMaterial*>(obj->mMaterial);
		if (pbr && ImGui::CollapsingHeader("PBR Material", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextUnformatted("Texture Usage");
			ImGui::Checkbox("Use Albedo Map", &pbr->mUseAlbedoMap);
			ImGui::Checkbox("Use Normal Map", &pbr->mUseNormalMap);
			ImGui::Checkbox("Use Metallic Map", &pbr->mUseMetallicMap);
			ImGui::Checkbox("Use Roughness Map", &pbr->mUseRoughnessMap);
			ImGui::Checkbox("Use AO Map", &pbr->mUseAOMap);

			ImGui::Separator();
			ImGui::TextUnformatted("Fallback Parameters");
			ImGui::ColorEdit3("Albedo Value", &pbr->mAlbedoValue.x);
			ImGui::SliderFloat("Metallic Value", &pbr->mMetallicValue, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness Value", &pbr->mRoughnessValue, 0.01f, 1.0f);
			ImGui::SliderFloat("AO Value", &pbr->mAOValue, 0.0f, 1.0f);
			ImGui::Checkbox("Use IBL", &pbr->mUseIBL);
		}

		auto* grass = dynamic_cast<GrassInstanceMaterial*>(obj->mMaterial);
		if (grass && ImGui::CollapsingHeader("Grass Material", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::TextUnformatted("Texture Usage");
			ImGui::Checkbox("Use Albedo Map##grass", &grass->mUseAlbedoMap);
			ImGui::Checkbox("Use Normal Map##grass", &grass->mUseNormalMap);
			ImGui::Checkbox("Use Roughness Map##grass", &grass->mUseRoughnessMap);
			ImGui::Checkbox("Use Metallic Map##grass", &grass->mUseMetallicMap);
			ImGui::Checkbox("Use AO Map##grass", &grass->mUseAOMap);
			ImGui::Checkbox("Use Opacity Map##grass", &grass->mUseOpacityMap);
			ImGui::Checkbox("Use Thickness Map##grass", &grass->mUseThicknessMap);
			ImGui::Checkbox("Use IBL##grass", &grass->mUseIBL);
			ImGui::Checkbox("Use Transmission##grass", &grass->mUseTransmission);

			ImGui::Separator();
			ImGui::TextUnformatted("Fallback Parameters");
			ImGui::ColorEdit3("Albedo##grass", &grass->mAlbedoValue.x);
			ImGui::SliderFloat("Roughness##grass", &grass->mRoughnessValue, 0.01f, 1.0f);
			ImGui::SliderFloat("Metallic##grass", &grass->mMetallicValue, 0.0f, 1.0f);
			ImGui::SliderFloat("AO##grass", &grass->mAOValue, 0.0f, 1.0f);
			ImGui::SliderFloat("Opacity##grass", &grass->mOpacityValue, 0.0f, 1.0f);
			ImGui::SliderFloat("Thickness##grass", &grass->mThicknessValue, 0.0f, 1.0f);
			ImGui::SliderFloat("Alpha Cutoff##grass", &grass->mAlphaCutoff, 0.0f, 1.0f);
			ImGui::SliderFloat("Transmission Strength##grass", &grass->mTransmissionStrength, 0.0f, 2.0f);
			ImGui::ColorEdit3("Subsurface Color##grass", &grass->mSubsurfaceColor.x);
			ImGui::SliderFloat("Specular Boost##grass", &grass->mSpecularBoost, 0.0f, 1.0f);

			ImGui::Separator();
			ImGui::TextUnformatted("Wind");
			ImGui::DragFloat3("Wind Direction##grass", &grass->mWindDirection.x, 0.01f);
			ImGui::SliderFloat("Wind Amplitude##grass", &grass->mWindAmplitude, 0.0f, 0.5f);
			ImGui::SliderFloat("Wind Frequency##grass", &grass->mWindFrequency, 0.0f, 8.0f);
			ImGui::SliderFloat("Gust Amplitude##grass", &grass->mWindGustAmplitude, 0.0f, 0.3f);
			ImGui::SliderFloat("Gust Frequency##grass", &grass->mWindGustFrequency, 0.0f, 12.0f);
			ImGui::SliderFloat("Phase Scale##grass", &grass->mPhaseScale, 0.1f, 10.0f);
			ImGui::SliderFloat("UV Scale##grass", &grass->mUVScale, 0.01f, 20.0f);
			ImGui::SliderFloat("Brightness##grass", &grass->mBrightness, 0.1f, 4.0f);
		}

		if (ImGui::Button("Remove Selected")) {
			mRemoveFn();
		}
	}

	ImGui::End();
}
