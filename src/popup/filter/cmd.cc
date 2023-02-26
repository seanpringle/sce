#include "cmd.h"

FilterPopupCmd::FilterPopupCmd() {
	name = "cmd";
}

std::string FilterPopupCmd::key() {
	return prefix.size() ? prefix: "_";
}

void FilterPopupCmd::exec(std::string suffix) {
	if (!suffix.size()) return;

	auto& group = history[key()];
	group.erase(std::remove(group.begin(), group.end(), suffix), group.end());
	group.insert(group.begin(), suffix);

	auto cmd = prefix.size() ? fmt("%s %s", prefix, suffix): suffix;
	config.interpret(cmd) || project.interpret(cmd) || project.view()->interpret(cmd);
}

void FilterPopupCmd::init() {
	options = history[key()];
}

void FilterPopupCmd::chosen(int option) {
	exec(options[option]);
}

void FilterPopupCmd::entered() {
	exec(input);
}

std::string FilterPopupCmd::hint() {
	return fmt("%s...", prefix);
}
