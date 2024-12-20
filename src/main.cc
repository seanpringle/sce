#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_sdl2.h"
#include "../imgui/imgui_impl_sdlrenderer.h"
#include <SDL.h>

#include <filesystem>
#include <chrono>
#include <algorithm>
#include <functional>
#include <regex>
#include <thread>

#include "theme.h"
#include "config.h"
#include "project.h"
#include "view.h"
#include "keys.h"
#include "catenate.h"
#include "channel.h"
#include "workers.h"
#include "repo.h"
#include "filetree.h"

using namespace std::literals::chrono_literals;

Theme theme;
Config config;
Project project;

ImFont* fontProp = nullptr;
ImFont* fontMono = nullptr;
ImFont* fontView = nullptr;
ImFont* fontSidebar = nullptr;
ImFont* fontPopup = nullptr;

ImGuiKey KeyMap[100] = {
	[KEY_ESCAPE] = (ImGuiKey)SDL_SCANCODE_ESCAPE,
	[KEY_BACKSPACE] = (ImGuiKey)SDL_SCANCODE_BACKSPACE,
	[KEY_DELETE] = (ImGuiKey)SDL_SCANCODE_DELETE,
	[KEY_TAB] = (ImGuiKey)SDL_SCANCODE_TAB,
	[KEY_HOME] = (ImGuiKey)SDL_SCANCODE_HOME,
	[KEY_END] = (ImGuiKey)SDL_SCANCODE_END,
	[KEY_PAGEDOWN] = (ImGuiKey)SDL_SCANCODE_PAGEDOWN,
	[KEY_PAGEUP] = (ImGuiKey)SDL_SCANCODE_PAGEUP,
	[KEY_RETURN] = (ImGuiKey)SDL_SCANCODE_RETURN,
	[KEY_LEFT] = (ImGuiKey)SDL_SCANCODE_LEFT,
	[KEY_RIGHT] = (ImGuiKey)SDL_SCANCODE_RIGHT,
	[KEY_UP] = (ImGuiKey)SDL_SCANCODE_UP,
	[KEY_DOWN] = (ImGuiKey)SDL_SCANCODE_DOWN,
	[KEY_SPACE] = (ImGuiKey)SDL_SCANCODE_SPACE,
	[KEY_TICK] = (ImGuiKey)SDL_SCANCODE_APOSTROPHE,
	[KEY_A] = (ImGuiKey)SDL_SCANCODE_A,
	[KEY_B] = (ImGuiKey)SDL_SCANCODE_B,
	[KEY_C] = (ImGuiKey)SDL_SCANCODE_C,
	[KEY_D] = (ImGuiKey)SDL_SCANCODE_D,
	[KEY_F] = (ImGuiKey)SDL_SCANCODE_F,
	[KEY_F1] = (ImGuiKey)SDL_SCANCODE_F1,
	[KEY_F2] = (ImGuiKey)SDL_SCANCODE_F2,
	[KEY_F3] = (ImGuiKey)SDL_SCANCODE_F3,
	[KEY_F4] = (ImGuiKey)SDL_SCANCODE_F4,
	[KEY_F5] = (ImGuiKey)SDL_SCANCODE_F5,
	[KEY_F6] = (ImGuiKey)SDL_SCANCODE_F6,
	[KEY_F7] = (ImGuiKey)SDL_SCANCODE_F7,
	[KEY_F8] = (ImGuiKey)SDL_SCANCODE_F8,
	[KEY_F9] = (ImGuiKey)SDL_SCANCODE_F9,
	[KEY_F10] = (ImGuiKey)SDL_SCANCODE_F10,
	[KEY_F11] = (ImGuiKey)SDL_SCANCODE_F11,
	[KEY_F12] = (ImGuiKey)SDL_SCANCODE_F12,
	[KEY_G] = (ImGuiKey)SDL_SCANCODE_G,
	[KEY_H] = (ImGuiKey)SDL_SCANCODE_H,
	[KEY_K] = (ImGuiKey)SDL_SCANCODE_K,
	[KEY_L] = (ImGuiKey)SDL_SCANCODE_L,
	[KEY_N] = (ImGuiKey)SDL_SCANCODE_N,
	[KEY_P] = (ImGuiKey)SDL_SCANCODE_P,
	[KEY_R] = (ImGuiKey)SDL_SCANCODE_R,
	[KEY_S] = (ImGuiKey)SDL_SCANCODE_S,
	[KEY_V] = (ImGuiKey)SDL_SCANCODE_V,
	[KEY_W] = (ImGuiKey)SDL_SCANCODE_W,
	[KEY_X] = (ImGuiKey)SDL_SCANCODE_X,
	[KEY_Y] = (ImGuiKey)SDL_SCANCODE_Y,
	[KEY_Z] = (ImGuiKey)SDL_SCANCODE_Z,
};

namespace {
	void fileTreeBuffers();
	void fileTreeBrowse();
	void fileTreeRefresh();

	#include "popup.cc"
	#include "popup/filter.cc"

	#include "popup/setup.cc"
	SetupPopup setupPopup;

	#include "popup/diff.cc"
	DiffPopup diffPopup;

	#include "popup/filter/tags.cc"
	FilterPopupTags tagsPopup;

	#include "popup/filter/open.cc"
	FilterPopupOpen openPopup;

	#include "popup/filter/change.cc"
	FilterPopupChange changePopup;

	#include "popup/filter/complete.cc"
	FilterPopupComplete completePopup;

	#include "popup/filter/refs.cc"
	FilterPopupRefs refsPopup;

	#include "popup/filter/cmd.cc"
	FilterPopupCmd cmdPopup;

	struct ViewTitle {
		char text[100] = {0};
	};

	std::vector<ViewTitle> viewTitles;

	bool command = false;
	bool find = false;
	bool line = false;

	std::string displayPath(std::string& ipath) {
		using namespace std::filesystem;
		for (auto& spath: project.searchPaths) {
			if (starts_with(ipath, spath)) {
				return ipath.substr(spath.size()+1);
			}
		}
		return ipath;
	};

	struct FileTreeAnnotated : FileTree {
		struct Annotation {
			std::chrono::time_point<std::chrono::system_clock> updated;
			bool created = false;
			bool modified = false;
			Repo::Diff diff;
		};

		std::map<std::string,Annotation> annotations;

		bool isOpen(const std::string& fpath) override {
			int index = project.find(fpath);
			return index >= 0;
		}

		bool isModified(const std::string& fpath) override {
			int index = project.find(fpath);
			return index >= 0 ? project.views[index]->modified: false;
		}

		bool isActive(const std::string& fpath) override {
			int index = project.find(fpath);
			return index == project.active;
		}

		void onSelect(const std::string& spath) override {
			project.open(spath);
		}

		void annotate(const std::string& fpath) override {
			using namespace ImGui;

			auto it = annotations.find(fpath);
			if (it == annotations.end()) {
				auto repo = Repo::open(fpath);
				if (repo && repo->ok()) {
					auto& anno = annotations[fpath];
					auto status = repo->status(fpath);
					anno.created = status.created();
					anno.modified = status.modified();
					anno.updated = std::chrono::system_clock::now();
					if (anno.modified) {
						anno.diff = repo->diff(fpath);
					}
					it = annotations.find(fpath);
				}
			}

			if (it != annotations.end()) {
				auto& anno = it->second;
				if (anno.created) {
					SameLine();
					PrintRight("A");
				}
				if (anno.modified) {
					SameLine();
					PrintRight("M");
					if (IsItemClicked(ImGuiMouseButton_Right) && anno.diff.ok()) {
						diffPopup.path = fpath;
						diffPopup.activate = true;
					}
					else
					if (IsItemHovered() && anno.diff.ok()) {
						auto& diff = anno.diff;
						BeginTooltip();
							PushFont(fontView);
							if (diff.ok()) {
								for (auto line: discatenate(diff.patch, "\n")) {
									int pushed = 0;
									if (line.size() && line.front() == '+') {
										PushStyleColor(ImGuiCol_Text, ImColorSRGB(0x009900ff));
										pushed++;
									}
									if (line.size() && line.front() == '-') {
										PushStyleColor(ImGuiCol_Text, ImColorSRGB(0xcc0000ff));
										pushed++;
									}
									TextUnformatted(&line.front(), &line.back()+1);
									PopStyleColor(pushed);
								}
							}
							PopFont();
						EndTooltip();
					}
				}
			}
		}
	};

	FileTreeAnnotated ftree;

	void fileTree(const std::set<std::string>& subset = {}) {
		size_t tokens = std::max(size_t(10), ftree.annotations.size()/10);
		for (auto it = ftree.annotations.begin(); it != ftree.annotations.end() && tokens > 0; ) {
			if (it->second.updated < std::chrono::system_clock::now()-1s) {
				it = ftree.annotations.erase(it);
				--tokens;
			}
			else {
				++it;
			}
		}
		ftree.render(project.searchPaths, subset);
	}

	void fileTreeBuffers() {
		std::set<std::string> viewPaths;
		for (auto view: project.views)
			viewPaths.insert(view->path);
		fileTree(viewPaths);
	}

	void fileTreeBrowse() {
		fileTree();
	}

	void fileTreeRefresh() {
		ftree.cache(project.searchPaths);
	}
}

int main(int argc, const char* argv[]) {
	git_libgit2_init();

	config.args(argc, argv);
	project.load(fmt("%s/.sce-project", std::getenv("HOME")));

	ensuref(0 == SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS), "%s", SDL_GetError());

	SDL_Window* window = SDL_CreateWindow("SCE",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		config.window.width, config.window.height,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
	);

	ensuref(window, "%s", SDL_GetError());

	uint32_t flags = SDL_RENDERER_ACCELERATED;
	if (config.window.vsync) flags |= SDL_RENDERER_PRESENTVSYNC;
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, flags);

	ensuref(renderer, "%s", SDL_GetError());

	bool fullscreen = false;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.WantSaveIniSettings = false;
//	io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
//	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	ImGui::StyleColorsClassic();

	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);

	float ddpi = 0.0f;
	float hdpi = 0.0f;
	float vdpi = 0.0f;
	SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
	if (ddpi < 0.01f) { ddpi = 96; hdpi = 96; vdpi = 96; }
	notef("DPI: diagonal %0.2f horizontal %0.2f vertical %0.2f", ddpi, hdpi, vdpi);

	auto fontpx = [&](float pt) {
		return (int)std::ceil(pt/72*vdpi);
	};

	auto& fonts = ImGui::GetIO().Fonts;
	fonts->Clear();

	ImFont* fontDef = fonts->AddFontDefault();

	static const ImWchar uniPlane0[] = { 0x0020, 0xFFFF, 0, };

	bool haveFontProp = config.font.prop.face.size() && std::filesystem::exists(config.font.prop.face);
	bool haveFontMono = config.font.mono.face.size() && std::filesystem::exists(config.font.mono.face);

	fontProp = haveFontProp
		? fonts->AddFontFromFileTTF(config.font.prop.face.c_str(), fontpx(config.font.prop.size), nullptr, uniPlane0)
		: fontDef;

	fontMono = haveFontMono
		? fonts->AddFontFromFileTTF(config.font.mono.face.c_str(), fontpx(config.font.mono.size), nullptr, uniPlane0)
		: fontDef;

	fontView = haveFontMono && (config.view.font > 1.0f || config.view.font < 1.0f)
		? fonts->AddFontFromFileTTF(config.font.mono.face.c_str(), fontpx(config.font.mono.size * config.view.font), nullptr, uniPlane0)
		: fontMono;

	fontSidebar = haveFontProp && (config.sidebar.font > 1.0f || config.sidebar.font < 1.0f)
		? fonts->AddFontFromFileTTF(config.font.prop.face.c_str(), fontpx(config.font.prop.size * config.sidebar.font), nullptr, uniPlane0)
		: fontProp;

	fontPopup = haveFontProp && (config.popup.font > 1.0f || config.popup.font < 1.0f)
		? fonts->AddFontFromFileTTF(config.font.prop.face.c_str(), fontpx(config.font.prop.size * config.popup.font), nullptr, uniPlane0)
		: fontProp;

	fonts->Build();

	ImGui_ImplSDLRenderer_DestroyFontsTexture();
	ImGui_ImplSDLRenderer_CreateFontsTexture();

	ImVec4 clear_color = ImVec4(0,0,0,1);

	auto now = [&]() {
		return std::chrono::system_clock::now();
	};

	auto gotFocus = now();
	auto lostFocus = now() - 24h;

	//SDL_ShowCursor(SDL_DISABLE);

	// Parts of IMGUI such as IsKeyPressed assume a rapid refresh rate to work
	// properly, but since we're using SDL_WaitEvent that assumption breaks.
	// Setting immediate=true anywhere forces a quick additional refresh pass
	// before falling back into waiting for events.
	bool immediate = false;

	for (bool done = false; !done;)
	{
		SDL_Event event;

		auto processEvent = [&]() {
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
				gotFocus = now();
			}

			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
				lostFocus = now();
				if (project.autosave) project.save();
			}

			immediate = ImGui_ImplSDL2_ProcessEvent(&event);

			if (event.type == SDL_QUIT) {
				done = true;
			}

			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
				done = true;
			}

			if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL || event.type == SDL_MOUSEMOTION) {
				immediate = true;
			}

			SDL_PumpEvents();
			// compress subsequent events of the same type
			if ((std::set<int>{SDL_MOUSEMOTION, SDL_MOUSEWHEEL, SDL_TEXTINPUT}).count(event.type)) {
				SDL_Event extra;
				while (SDL_PeepEvents(&extra, 1, SDL_GETEVENT, event.type, event.type)) {
					ImGui_ImplSDL2_ProcessEvent(&extra);
				}
			}
		};

		if (immediate) {
			immediate = false;
			std::this_thread::sleep_for(1ms);
			if (SDL_PollEvent(&event)) processEvent();
		}
		else {
			while (!SDL_WaitEvent(&event));
			processEvent();
		}

		SDL_GetWindowSize(window, &config.window.width, &config.window.height);

		ImGui_ImplSDLRenderer_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();
		ImGui::TweakDefaults();

		bool suppressInput = gotFocus < lostFocus || gotFocus > now() - 300ms;

		{
			using namespace ImGui;

			std::chrono::time_point<std::chrono::system_clock> popupLastOpen;
			if (IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) popupLastOpen = std::chrono::system_clock::now();

			bool suppressInputView = popupLastOpen > std::chrono::system_clock::now()-100ms;

			if (!suppressInput) {

				if (io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_PAGEUP])) project.activeUpTree();
				if (io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_PAGEDOWN])) project.activeDownTree();

				find = io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_F]);
				line = io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_G]);
				command = io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_TICK]);

				tagsPopup.activate = io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_R]);
				refsPopup.activate = io.KeyCtrl && io.KeyShift && IsKeyPressed(KeyMap[KEY_F]);
				completePopup.activate = io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_TAB]);
				openPopup.activate = io.KeyCtrl && !io.KeyAlt && IsKeyPressed(KeyMap[KEY_P]);
				changePopup.activate = io.KeyCtrl && io.KeyAlt && IsKeyPressed(KeyMap[KEY_P]);

				setupPopup.activate = IsKeyPressed(KeyMap[KEY_F6]);

				if (IsKeyPressed(KeyMap[KEY_F12])) {
					config.defaults();
					immediate = true;
				}

				if (IsKeyPressed(KeyMap[KEY_F11])) {
					fullscreen = !fullscreen;
					SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP: 0);
					immediate = true;
				}

				if (!suppressInputView) {

					if (IsKeyReleased(KeyMap[KEY_F1])) {
						project.layout1();
					}

					if (IsKeyReleased(KeyMap[KEY_F2])) {
						project.layout2();
					}

					if (project.view()) {
						if (io.KeyCtrl && IsKeyReleased(KeyMap[KEY_W])) {
							if (!project.view()->modified) {
								project.close();
							}
						}

						if (IsKeyReleased(KeyMap[KEY_F3])) {
							project.relatedOpen();
						}

						if (IsKeyReleased(KeyMap[KEY_F4])) {
							project.cycle();
						}

						if (IsKeyReleased(KeyMap[KEY_F5])) {
							project.view()->save();
						}

						if (IsKeyReleased(KeyMap[KEY_F7])) {
							fileTreeRefresh();
						}

						if (IsKeyReleased(KeyMap[KEY_F8])) {
							project.view()->reload();
						}

						if (io.KeyCtrl && io.KeyShift && IsKeyPressed(KeyMap[KEY_PAGEUP])) {
							project.movePrevGroup();
						}

						if (io.KeyCtrl && io.KeyShift && IsKeyPressed(KeyMap[KEY_PAGEDOWN])) {
							project.moveNextGroup();
						}

						if (io.KeyCtrl && IsKeyReleased(KeyMap[KEY_N])) {
							project.fresh();
						}
					}
				}
			}

			auto vsplit = [&](float space, float split) {
				return split < 1.0f ? space * split: split;
			};

			auto nextPopup = [&](int h = -1) {
				using namespace ImGui;
				h = std::min(config.window.height, h);
				int w = std::min(config.window.width, (int)vsplit(config.window.width, config.popup.width));
				SetNextWindowPos(ImVec2((config.window.width-w)/2,(config.window.height-h)/2));
				SetNextWindowSize(ImVec2(w,h));
			};

			SetNextWindowPos(ImVec2(0,0));
			SetNextWindowSize(ImVec2(config.window.width,config.window.height));

			uint flags = ImGuiWindowFlags_NoNav
				| ImGuiWindowFlags_NoDecoration
				| ImGuiWindowFlags_NoSavedSettings
				| ImGuiWindowFlags_NoBringToFrontOnFocus
				| ImGuiWindowFlags_NoScrollbar
				| ImGuiWindowFlags_NoScrollWithMouse
			;

			auto bg = ImColorSRGB(0x272727ff);

			PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2,2));

			Begin("#bg", nullptr, flags);
				PopStyleVar(1); // WindowPadding
				PushFont(fontProp);

				if (command) {
					cmdPopup.prefix.clear();
					cmdPopup.activate = true;
					find = false;
				}

				if (find) {
					cmdPopup.prefix = "find";
					cmdPopup.activate = true;
					find = false;
				}

				if (line) {
					cmdPopup.prefix = "go";
					cmdPopup.activate = true;
					line = false;
				}

				if (setupPopup.activate) {
					OpenPopup("#setup");
				}

				if (diffPopup.activate) {
					OpenPopup("#diff");
				}

				if (tagsPopup.activate) {
					OpenPopup("#tags");
				}

				if (openPopup.activate) {
					OpenPopup("#open");
				}

				if (changePopup.activate) {
					OpenPopup("#change");
				}

				if (completePopup.activate) {
					OpenPopup("#complete");
				}

				if (refsPopup.activate) {
					refsPopup.needle = project.view()->selected();
					OpenPopup("#refs");
				}

				if (cmdPopup.activate) {
					OpenPopup("#cmd");
				}

				viewTitles.clear();
				viewTitles.resize(project.views.size());

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2,4));

				if (BeginTable("#layout", project.groups.size()+1)) {

					float sidebarWidth = vsplit(config.window.width, config.sidebar.width);
					TableSetupColumn("files", ImGuiTableColumnFlags_WidthFixed, sidebarWidth);

					if (project.layout == 2 && project.groups.size() == 2U && config.layout2.split > 0.01f) {
						float leftWidth = vsplit(config.window.width, config.layout2.split);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, leftWidth);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
					}
					else {
						for (uint i = 0; i < project.groups.size(); i++) {
							TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						}
					}

					TableNextRow();

					TableNextColumn();
					TableSetBgColor(ImGuiTableBgTarget_CellBg, bg);
					SetCursorPosX(GetCursorPosX()+4);
					TextUnformatted(std::filesystem::path(project.ppath).filename().c_str());

					for (uint i = 0; i < project.groups.size(); i++) {
						TableNextColumn();
						auto& group = project.groups[i];
						if (!group.size()) continue;
						auto view = group.front();

						TableSetBgColor(ImGuiTableBgTarget_CellBg,
							project.view() == view
								? GetColorU32(ImGuiCol_TitleBgActive)
								: GetColorU32(ImGuiCol_FrameBg)
						);

						SetCursorPosX(GetCursorPosX() + GetStyle().ItemSpacing.x);
						Text("%s", displayPath(view->path).c_str());
						SameLine(); PrintRight(view->blurb().c_str());
					}

					TableNextRow();

					TableNextColumn();
					TableSetBgColor(ImGuiTableBgTarget_CellBg, bg);
					PushStyleColor(ImGuiCol_FrameBg, bg);
					PushFont(fontSidebar);
					if (BeginTabBar("#left-tabs")) {
						if (BeginTabItem("buffers")) {
							BeginChild("##open-pane");
							fileTreeBuffers();
							EndChild();
							EndTabItem();
						}
						if (BeginTabItem("browse")) {
							BeginChild("##browse-pane");
							fileTreeBrowse();
							EndChild();
							EndTabItem();
						}
						EndTabBar();
					}
					PopFont();
					PopStyleColor(1);

					PushFont(fontView);
					for (auto& group: project.groups) {
						TableNextColumn();
						TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColorSRGB(0x222222ff));
						if (!group.size()) continue;
						auto view = group.front();

						bool viewHasInput = project.view() == view
							&& !suppressInputView
							&& !suppressInput;

						if (viewHasInput) {
							view->input();
						}

						auto gap = GetStyle().ItemSpacing.x;
						SetCursorPos(ImVec2(GetCursorPosX() + gap, GetCursorPosY() + gap));
						view->draw();

						if (IsAnyMouseDown() && view->mouseOver) {
							project.active = project.find(view->path);
							project.bubble();
						}
					}
					PopFont();
					EndTable();
				}

				PopStyleVar(1);
				PushFont(fontPopup);

				nextPopup(config.window.height/3*2);

				immediate = completePopup.run() || immediate;

				nextPopup(config.window.height/3*2);

				immediate = tagsPopup.run() || immediate;

				nextPopup(config.window.height/3*2);

				immediate = refsPopup.run() || immediate;

				nextPopup(config.window.height/3*2);

				immediate = cmdPopup.run() || immediate;

				nextPopup(config.window.height/3*2);

				immediate = openPopup.run() || immediate;

				nextPopup(config.window.height/3*2);

				immediate = changePopup.run() || immediate;

				nextPopup(config.window.height/3*2);

				immediate = setupPopup.run() || immediate;

				nextPopup(config.window.height/3*2);

				immediate = diffPopup.run() || immediate;

				PopFont();
				PopFont();
			End();
		}

		ImGui::Render();
		ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
		SDL_RenderPresent(renderer);
	}

	if (project.ppath.empty()) {
		project.ppath = fmt("%s/.sce-project", std::getenv("HOME"));
	}

	project.save();

	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	Repo::repos.clear();
	git_libgit2_shutdown();
	return 0;
}
