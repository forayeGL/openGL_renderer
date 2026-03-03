#include "application/rendererApp.h"

int main() {
	RendererApp app(2560, 1440);

	if (!app.init()) {
		return -1;
	}

	app.run();
	app.shutdown();

	return 0;
}