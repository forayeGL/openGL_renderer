#pragma once
#include "i_gui_panel.h"
#include "../../glframework/mesh/mesh.h"
#include "../../glframework/scene.h"
#include <vector>
#include <string>
#include <functional>

// ScenePanel: object list, selection, transform editing, add/remove.
class ScenePanel : public IGuiPanel {
public:
	using AddObjectFn    = std::function<void(const std::string& type)>;
	using RemoveObjectFn = std::function<void()>;

	ScenePanel(
		std::vector<Mesh*>& dynamicObjects,
		int&                selectedObject,
		AddObjectFn         addFn,
		RemoveObjectFn      removeFn
	);

	void onRender() override;

private:
	std::vector<Mesh*>& mDynamicObjects;
	int&                mSelectedObject;
	AddObjectFn         mAddFn;
	RemoveObjectFn      mRemoveFn;
};
