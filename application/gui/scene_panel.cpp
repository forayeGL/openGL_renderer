#include "scene_panel.h"
#include "../../imgui/imgui.h"

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

		if (ImGui::Button("Remove Selected")) {
			mRemoveFn();
		}
	}

	ImGui::End();
}
