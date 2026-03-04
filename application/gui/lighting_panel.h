#pragma once
#include "i_gui_panel.h"
#include "../../glframework/light/directionalLight.h"
#include "../../glframework/light/pointLight.h"
#include "../../glframework/renderer/ubo_manager.h"
#include <vector>
#include <functional>

// LightingPanel: directional light controls, point light list with add/remove.
class LightingPanel : public IGuiPanel {
public:
	using AddPointLightFn    = std::function<void()>;
	using RemovePointLightFn = std::function<void(int)>;

	LightingPanel(
		DirectionalLight*         dirLight,
		std::vector<PointLight*>& pointLights,
		AddPointLightFn           addFn,
		RemovePointLightFn        removeFn
	);

	void onRender() override;

private:
	DirectionalLight*         mDirLight;
	std::vector<PointLight*>& mPointLights;
	AddPointLightFn           mAddFn;
	RemovePointLightFn        mRemoveFn;
};
