#include "lighting_panel.h"
#include "../../imgui/imgui.h"
#include <string>

LightingPanel::LightingPanel(
	DirectionalLight*         dirLight,
	std::vector<PointLight*>& pointLights,
	AddPointLightFn           addFn,
	RemovePointLightFn        removeFn
)
	: mDirLight(dirLight)
	, mPointLights(pointLights)
	, mAddFn(std::move(addFn))
	, mRemoveFn(std::move(removeFn))
{}

void LightingPanel::onRender() {
	ImGui::Begin("Lighting");

	if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
		glm::vec3 dirPos = mDirLight->getPosition();
		if (ImGui::DragFloat3("Dir Position", &dirPos.x, 0.1f)) {
			mDirLight->setPosition(dirPos);
		}
		ImGui::ColorEdit3("Dir Color", &mDirLight->mColor.x);
		ImGui::SliderFloat("Dir Intensity", &mDirLight->mIntensity, 0.0f, 10.0f);
		ImGui::SliderFloat("Dir Specular", &mDirLight->mSpecularIntensity, 0.0f, 5.0f);

		float dirAngleX = 0.0f, dirAngleY = 0.0f;
		if (ImGui::DragFloat("Dir Angle X", &dirAngleX, 0.5f)) { mDirLight->rotateX(dirAngleX); }
		if (ImGui::DragFloat("Dir Angle Y", &dirAngleY, 0.5f)) { mDirLight->rotateY(dirAngleY); }

		if (mDirLight->mShadow) {
			ImGui::SliderFloat("Shadow Bias",    &mDirLight->mShadow->mBias,         0.0f, 0.01f, "%.5f");
			ImGui::SliderFloat("PCF Radius",     &mDirLight->mShadow->mPcfRadius,    0.0f, 0.1f);
			ImGui::SliderFloat("Disk Tightness", &mDirLight->mShadow->mDiskTightness,0.0f, 5.0f);
			ImGui::SliderFloat("Light Size",     &mDirLight->mShadow->mLightSize,    0.0f, 0.5f);
		}
	}

	ImGui::Separator();
	ImGui::Text("Point Lights: %d / %d", (int)mPointLights.size(), MAX_POINT_LIGHTS);
	if ((int)mPointLights.size() < MAX_POINT_LIGHTS) {
		if (ImGui::Button("Add Point Light")) { mAddFn(); }
	}

	for (int i = 0; i < (int)mPointLights.size(); i++) {
		char header[64];
		snprintf(header, sizeof(header), "Point Light %d", i);
		if (ImGui::CollapsingHeader(header)) {
			std::string id = std::to_string(i);
			glm::vec3 plPos = mPointLights[i]->getPosition();
			if (ImGui::DragFloat3(("PL Pos##"   + id).c_str(), &plPos.x, 0.1f)) {
				mPointLights[i]->setPosition(plPos);
			}
			ImGui::ColorEdit3(("PL Color##" + id).c_str(), &mPointLights[i]->mColor.x);
			ImGui::SliderFloat(("PL K2##"   + id).c_str(), &mPointLights[i]->mK2, 0.0f, 2.0f);
			ImGui::SliderFloat(("PL K1##"   + id).c_str(), &mPointLights[i]->mK1, 0.0f, 2.0f);
			ImGui::SliderFloat(("PL Kc##"   + id).c_str(), &mPointLights[i]->mKc, 0.0f, 2.0f);

			bool autoRange = mPointLights[i]->mRange <= 0.0f;
			if (ImGui::Checkbox(("PL Auto Range##" + id).c_str(), &autoRange)) {
				if (autoRange) {
					mPointLights[i]->mRange = -1.0f;
				} else if (mPointLights[i]->mRange <= 0.0f) {
					mPointLights[i]->mRange = 10.0f;
				}
			}
			if (!autoRange) {
				ImGui::SliderFloat(("PL Range##" + id).c_str(), &mPointLights[i]->mRange, 0.1f, 500.0f);
			}

			if (mPointLights[i]->mShadow) {
				ImGui::SliderFloat(("PL Shadow Bias##" + id).c_str(),
					&mPointLights[i]->mShadow->mBias, 0.0f, 0.01f, "%.5f");
			}

			if (ImGui::Button(("Remove##pl" + id).c_str())) {
				mRemoveFn(i);
				break; // iterator invalidated
			}
		}
	}

	ImGui::End();
}
