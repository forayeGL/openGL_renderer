#pragma once
#include "i_gui_panel.h"
#include <vector>
#include <memory>

struct GLFWwindow;

// GuiSystem owns all IGuiPanel instances and drives the ImGui frame lifecycle.
// To add a new panel, call addPanel() before the first frame.
class GuiSystem {
public:
	GuiSystem() = default;
	~GuiSystem() = default;

	// Call once after GL context and GLFW window are ready.
	void init(GLFWwindow* window, const char* glslVersion = "#version 460");

	// Call once when shutting down.
	void shutdown();

	// Register a panel. Panels are rendered in insertion order.
	void addPanel(std::unique_ptr<IGuiPanel> panel);

	// Drive a full ImGui frame: NewFrame → onRender for each panel → Render.
	void render(GLFWwindow* window);

private:
	std::vector<std::unique_ptr<IGuiPanel>> mPanels;
};
