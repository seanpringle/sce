#include "setup.h"

SetupPopup::SetupPopup() {
	name = "setup";
}

void SetupPopup::setup() {
	setupProjectSavePath[0] = 0;
	setupProjectAddSearchPath[0] = 0;
	setupProjectAddIgnorePath[0] = 0;
	auto path = std::filesystem::weakly_canonical(project.ppath);
	snprintf(setupProjectSavePath, sizeof(setupProjectSavePath), "%s", path.string().c_str());
}

void SetupPopup::render() {
	using namespace ImGui;
	if (BeginTabBar("#setup-tabs")) {
		int id = 0;

		if (BeginTabItem("Project##setup-tab-project")) {
			auto width = GetWindowContentRegionWidth();

			BeginTable("#project", 2);

			TableSetupColumn("Config", ImGuiTableColumnFlags_WidthStretch);
			TableSetupColumn("");

			TableNextRow();

			TableNextColumn();
			SetNextItemWidth(GetWindowContentRegionWidth());
			InputText("##save-path-input", setupProjectSavePath, sizeof(setupProjectSavePath));

			TableNextColumn();
			if (Button("save##save-path-button", ImVec2(width*0.25,0)) && setupProjectSavePath[0]) {
				project.save(setupProjectSavePath);
			}

			EndTable();

			NewLine();
			BeginTable("#searchPaths", 2);

			TableSetupColumn("Search Paths", ImGuiTableColumnFlags_WidthStretch);
			TableSetupColumn("");

			TableHeadersRow();

			std::string removeSearchPath;

			for (auto path: project.searchPaths) {
				TableNextRow();

				TableNextColumn();
				Print(path.c_str());

				TableNextColumn();
				if (Button(fmtc("remove##removeSearchPath%d", id++), ImVec2(width*0.25,0))) {
					removeSearchPath = path;
				}
			}

			TableNextRow();

			TableNextColumn();
			SetNextItemWidth(GetWindowContentRegionWidth());
			InputText("##add-path-input", setupProjectAddSearchPath, sizeof(setupProjectAddSearchPath));

			TableNextColumn();
			if (Button("add##add-path-button", ImVec2(width*0.25,0)) && setupProjectAddSearchPath[0]) {
				project.searchPathAdd(setupProjectAddSearchPath);
				setupProjectAddSearchPath[0] = 0;
			}

			EndTable();

			NewLine();
			BeginTable("#ignorePaths", 2);

			TableSetupColumn("Ignore Paths", ImGuiTableColumnFlags_WidthStretch);
			TableSetupColumn("");

			TableHeadersRow();

			std::string removeIgnorePath;

			for (auto path: project.ignorePaths) {
				TableNextRow();

				TableNextColumn();
				Print(path.c_str());

				TableNextColumn();
				if (Button(fmtc("remove##removeIgnorePath%d", id++), ImVec2(width*0.25,0))) {
					removeIgnorePath = path;
				}
			}

			TableNextRow();

			TableNextColumn();
			SetNextItemWidth(GetWindowContentRegionWidth());
			InputText("##add-path-input", setupProjectAddIgnorePath, sizeof(setupProjectAddIgnorePath));

			TableNextColumn();
			if (Button("add##add-path-button", ImVec2(width*0.25,0)) && setupProjectAddIgnorePath[0]) {
				project.ignorePathAdd(setupProjectAddIgnorePath);
				setupProjectAddIgnorePath[0] = 0;
			}

			EndTable();

			if (!removeSearchPath.empty()) {
				project.searchPathDrop(removeSearchPath);
			}

			if (!removeIgnorePath.empty()) {
				project.ignorePathDrop(removeIgnorePath);
			}

			EndTabItem();
		}

		EndTabBar();
	}
}
