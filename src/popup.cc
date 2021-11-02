#include "popup.h"

bool Popup::run() {
	using namespace ImGui;
	if (BeginPopup(fmtc("#%s", name))) {
		if (activate) {
			activate = false;
			setup();
		}

		render();

		if (IsKeyPressed(KeyMap[KEY_ESCAPE])) {
			CloseCurrentPopup();
		}

		EndPopup();
		return immediate;
	}

	return false;
}
