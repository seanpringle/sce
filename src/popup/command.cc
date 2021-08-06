#include "command.h"

CommandPopup::CommandPopup() {
	name = "command";
}

void CommandPopup::setup() {
	using namespace ImGui;
	SetKeyboardFocusHere();
	input[0] = 0;
}

void CommandPopup::render() {
	using namespace ImGui;
	SetNextItemWidth(-FLT_MIN);

	if (InputTextWithHint("#command-input", fmtc("%s...", prefix), input, sizeof(input), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_AutoSelectAll)){
		auto cmd = prefix.size() ? fmt("%s %s", prefix, input): std::string(input);
		project.interpret(cmd) || project.view()->interpret(cmd);
		CloseCurrentPopup();
	}
}
