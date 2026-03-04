#include "gui_system.h"
#include "../../imgui/imgui.h"
#include "../../imgui/imgui_impl_glfw.h"
#include "../../imgui/imgui_impl_opengl3.h"
#include "../../glframework/core.h"

void GuiSystem::init(GLFWwindow* window, const char* glslVersion) {
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glslVersion);

	for (auto& panel : mPanels) {
		panel->onInit();
	}
}

void GuiSystem::shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void GuiSystem::addPanel(std::unique_ptr<IGuiPanel> panel) {
	mPanels.push_back(std::move(panel));
}

void GuiSystem::render(GLFWwindow* window) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	for (auto& panel : mPanels) {
		panel->onRender();
	}

	ImGui::Render();

	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
