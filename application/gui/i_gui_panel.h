#pragma once

// Abstract base for all ImGui panels.
// Each panel owns a slice of the UI and the data it needs.
// Override onRender() to draw ImGui widgets.
class IGuiPanel {
public:
	virtual ~IGuiPanel() = default;

	// Called every frame inside ImGui::NewFrame() / ImGui::Render().
	virtual void onRender() = 0;

	// Optional: called once after OpenGL/ImGui context is ready.
	virtual void onInit() {}
};
