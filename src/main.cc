
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_sdl.h"
#include "../imgui/imgui_impl_opengl3.h"
#include <SDL.h>
#include <GL/glew.h>

#include "theme.h"
#include "config.h"
#include "project.h"
#include "view.h"
#include "keys.h"
#include <filesystem>
#include <experimental/filesystem>

using namespace std::literals::chrono_literals;

Theme theme;
Config config;
Project project;

int KeyMap[100] = {
	[KEY_ESCAPE] = SDL_SCANCODE_ESCAPE,
	[KEY_BACKSPACE] = SDL_SCANCODE_BACKSPACE,
	[KEY_DELETE] = SDL_SCANCODE_DELETE,
	[KEY_TAB] = SDL_SCANCODE_TAB,
	[KEY_HOME] = SDL_SCANCODE_HOME,
	[KEY_END] = SDL_SCANCODE_END,
	[KEY_PAGEDOWN] = SDL_SCANCODE_PAGEDOWN,
	[KEY_PAGEUP] = SDL_SCANCODE_PAGEUP,
	[KEY_RETURN] = SDL_SCANCODE_RETURN,
	[KEY_LEFT] = SDL_SCANCODE_LEFT,
	[KEY_RIGHT] = SDL_SCANCODE_RIGHT,
	[KEY_UP] = SDL_SCANCODE_UP,
	[KEY_DOWN] = SDL_SCANCODE_DOWN,
	[KEY_SPACE] = SDL_SCANCODE_SPACE,
	[KEY_B] = SDL_SCANCODE_B,
	[KEY_C] = SDL_SCANCODE_C,
	[KEY_D] = SDL_SCANCODE_D,
	[KEY_F] = SDL_SCANCODE_F,
	[KEY_F1] = SDL_SCANCODE_F1,
	[KEY_F2] = SDL_SCANCODE_F2,
	[KEY_F3] = SDL_SCANCODE_F3,
	[KEY_F12] = SDL_SCANCODE_F12,
	[KEY_G] = SDL_SCANCODE_G,
	[KEY_H] = SDL_SCANCODE_H,
	[KEY_K] = SDL_SCANCODE_K,
	[KEY_L] = SDL_SCANCODE_L,
	[KEY_P] = SDL_SCANCODE_P,
	[KEY_R] = SDL_SCANCODE_R,
	[KEY_S] = SDL_SCANCODE_S,
	[KEY_V] = SDL_SCANCODE_V,
	[KEY_W] = SDL_SCANCODE_W,
	[KEY_X] = SDL_SCANCODE_X,
	[KEY_Y] = SDL_SCANCODE_Y,
	[KEY_Z] = SDL_SCANCODE_Z,
};

std::vector<std::vector<View*>> groups;
int layout = 0;

void forget(View* view) {
	for (auto& src: groups) {
		src.erase(std::remove(src.begin(), src.end(), view), src.end());
	}
}

bool known(View* view) {
	for (auto& src: groups) {
		if (std::find(src.begin(), src.end(), view) != src.end()) return true;
	}
	return false;
}

int group(View* view) {
	if (!view) return 0;
	for (int i = 0; i < (int)groups.size(); i++) {
		auto& group = groups[i];
		auto it = std::find(group.begin(), group.end(), view);
		if (it != group.end()) return i;
	}
	throw view;
}

void sanity() {
	std::set<View*> views = {project.views.begin(), project.views.end()};
	for (auto it = groups.begin(); it != groups.end(); ) {
		auto &group = *it;
		if (!group.size()) {
			it = groups.erase(it);
			continue;
		}
		for (auto view: group) {
			ensure(views.count(view));
			views.erase(view);
		}
		++it;
	}
	ensure(!views.size());
	if (!groups.size()) {
		groups.resize(1);
	}
}

void bubble() {
	auto& grp = groups[group(project.view())];

	if (project.view() && grp.front() != project.view()) {
		forget(project.view());
		grp.insert(grp.begin(), project.view());
		sanity();
	}
}

struct ViewTitle {
	char text[100] = {0};
};

std::vector<ViewTitle> viewTitles;

int main(int argc, const char* argv[]) {
	notef("hello world");

	for (auto arg: config.args(argc, argv)) {
		auto path = std::filesystem::path(arg);
		if (std::filesystem::is_regular_file(path)) {
			project.open(arg);
		}
		if (std::filesystem::is_directory(path)) {
			project.paths.push_back(arg);
		}
	}

	if (!project.paths.size()) {
		project.paths.push_back(".");
	}

	project.active = 0;
	groups.resize(1);

	for (auto view: project.views) {
		groups.front().push_back(view);
	}

	ensuref(0 == SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS), "%s", SDL_GetError());

	const char* glsl_version = "#version 330";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

	SDL_Window* window = SDL_CreateWindow("SCE",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		config.window.width, config.window.height,
		window_flags
	);

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);

	if (config.window.vsync) {
		SDL_GL_SetSwapInterval(1);
	}

	ensuref(GLEW_OK == glewInit(), "glew fail");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.WantSaveIniSettings = false;

	ImGui::StyleColorsClassic();

	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	float ddpi = 0.0f;
	float hdpi = 0.0f;
	float vdpi = 0.0f;
	SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
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

	ImFont* fontProp = haveFontProp
		? fonts->AddFontFromFileTTF(config.font.prop.face.c_str(), fontpx(config.font.prop.size), nullptr, uniPlane0)
		: fontDef;

	ImFont* fontMono = haveFontMono
		? fonts->AddFontFromFileTTF(config.font.mono.face.c_str(), fontpx(config.font.mono.size), nullptr, uniPlane0)
		: fontDef;

	ImFont* fontView = haveFontMono && (config.view.font > 1.0f || config.view.font < 1.0f)
		? fonts->AddFontFromFileTTF(config.font.mono.face.c_str(), fontpx(config.font.mono.size * config.view.font), nullptr, uniPlane0)
		: fontMono;

	ImFont* fontSidebar = haveFontProp && (config.sidebar.font > 1.0f || config.sidebar.font < 1.0f)
		? fonts->AddFontFromFileTTF(config.font.prop.face.c_str(), fontpx(config.font.prop.size * config.sidebar.font), nullptr, uniPlane0)
		: fontProp;

	ImFont* fontPopup = haveFontProp && (config.popup.font > 1.0f || config.popup.font < 1.0f)
		? fonts->AddFontFromFileTTF(config.font.prop.face.c_str(), fontpx(config.font.prop.size * config.popup.font), nullptr, uniPlane0)
		: fontProp;

	fonts->Build();

	ImGui_ImplOpenGL3_DestroyFontsTexture();
	ImGui_ImplOpenGL3_CreateFontsTexture();

	ImVec4 clear_color = ImVec4(0,0,0,1);

	bool command = false;
	const char* commandPrefix = nullptr;
	char commandInput[100];

	bool find = false;
	char findInput[100];
	std::memset(findInput, 0, sizeof(findInput));

	bool line = false;
	char lineInput[100];
	std::memset(lineInput, 0, sizeof(lineInput));

	bool tags = false;
	char tagsInput[100];
	int tagSelected = 0;
	std::vector<ViewRegion> tagRegions;
	std::vector<std::string> tagStrings;
	std::memset(tagsInput, 0, sizeof(tagsInput));

	bool open = false;
	char openInput[100];
	int openSelected = 0;
	std::vector<std::string> openPaths;
	std::memset(openInput, 0, sizeof(openInput));

	bool comp = false;
	char compInput[100];
	int compSelected = 0;
	std::string compPrefix;
	std::vector<std::string> compStrings;
	std::memset(compInput, 0, sizeof(compInput));

	auto now = []() {
		return std::chrono::system_clock::now();
	};

	auto gotFocus = now();
	auto lostFocus = now() - 24h;

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
			}

			immediate = ImGui_ImplSDL2_ProcessEvent(&event);

			if (event.type == SDL_QUIT) {
				done = true;
			}

			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
				done = true;
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
			if (SDL_PollEvent(&event)) processEvent();
		}
		else {
			while (!SDL_WaitEvent(&event));
			processEvent();
		}

		SDL_GetWindowSize(window, &config.window.width, &config.window.height);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		bool suppressInput = gotFocus < lostFocus || gotFocus > now() - 300ms;

		{
			using namespace ImGui;

			if (!suppressInput) {

				if (io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_PAGEUP])) project.active--;
				if (io.KeyCtrl && !io.KeyShift && IsKeyPressed(KeyMap[KEY_PAGEDOWN])) project.active++;

				project.sanity();

				find = io.KeyCtrl && IsKeyPressed(KeyMap[KEY_F]);
				line = io.KeyCtrl && IsKeyPressed(KeyMap[KEY_G]);
				tags = io.KeyCtrl && IsKeyPressed(KeyMap[KEY_R]);
				open = io.KeyCtrl && IsKeyPressed(KeyMap[KEY_P]);
				comp = io.KeyCtrl && IsKeyPressed(KeyMap[KEY_TAB]);

				if (IsKeyPressed(KeyMap[KEY_F12])) {
					done = true;
				}

				if (IsKeyPressed(KeyMap[KEY_F1])) {
					groups.clear();
					groups.resize(1);
					layout = 1;
					for (auto view: project.views) {
						groups[0].push_back(view);
					}
				}

				if (IsKeyPressed(KeyMap[KEY_F2])) {
					groups.clear();
					groups.resize(2);
					layout = 2;
					for (auto view: project.views) {
						auto path = std::filesystem::path(view->path);
						auto ext = path.extension().string();
						if (ext.front() == '.') ext = ext.substr(1);
						int g = config.layout2.left.count(ext) ? 0:1;
						groups[g].push_back(view);
					}
				}

				if (project.view()) {
					if (io.KeyCtrl && IsKeyPressed(KeyMap[KEY_W])) {
						if (!project.view()->modified) {
							forget(project.view());
							project.close();
							sanity();
						}
					}

					if (IsKeyPressed(KeyMap[KEY_F3])) {
						auto active = group(project.view());
						auto path = std::filesystem::path(project.view()->path);
						auto ext = path.extension().string();

						auto open = [&](auto rep) {
							auto rpath = path.replace_extension(rep).string();
							auto view = project.open(rpath);
							if (view && !known(view)) {
								groups[active].push_back(view);
							}
							return view ? true:false;
						};

						if (ext == ".c") {
							open(".h");
						}
						if (ext == ".cc" || ext == ".cpp") {
							open(".hpp") ||
							open(".h");
						}
						if (ext == ".h") {
							open(".c") ||
							open(".cc") ||
							open(".cpp");
						}
						if (ext == ".hpp") {
							open(".cpp") ||
							open(".cc");
						}
						sanity();
					}

					if (io.KeyCtrl && IsKeyPressed(KeyMap[KEY_SPACE]) && groups.size() > 1U) {
						bool found = false;
						auto active = group(project.view());
						for (int i = active+1; !found && i < (int)groups.size(); i++) {
							if (groups[i].size()) {
								project.active = project.find(groups[i].front());
								found = true;
							}
						}
						for (int i = 0; !found && i < active; i++) {
							if (groups[i].size()) {
								project.active = project.find(groups[i].front());
								found = true;
							}
						}
					}

					if (io.KeyCtrl && io.KeyShift && IsKeyPressed(KeyMap[KEY_PAGEUP])) {
						auto active = group(project.view());
						if (active == 0) {
							forget(project.view());
							groups.insert(groups.begin(), {project.view()});
						} else {
							auto& dst = groups[active-1];
							forget(project.view());
							dst.insert(dst.begin(), project.view());
						}
						sanity();
					}

					if (io.KeyCtrl && io.KeyShift && IsKeyPressed(KeyMap[KEY_PAGEDOWN])) {
						auto active = group(project.view());
						if (active == (int)groups.size()-1) {
							forget(project.view());
							groups.push_back({project.view()});
						} else {
							auto& dst = groups[active+1];
							forget(project.view());
							dst.insert(dst.begin(), project.view());
						}
						sanity();
					}
				}
			}

			bubble();

			auto vsplit = [](float space, float split) {
				return split < 1.0f ? space * split: split;
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

			Begin("#bg", nullptr, flags);
				PushFont(fontProp);

				if (find || line) {
					OpenPopup("#command");
				}

				if (find) {
					commandPrefix = "find ";
					command = true;
					find = false;
				}

				if (line) {
					commandPrefix = "go ";
					command = true;
					line = false;
				}

				if (tags) {
					OpenPopup("#tags");
				}

				if (open) {
					OpenPopup("#open");
				}

				if (comp) {
					OpenPopup("#comp");
				}

				viewTitles.clear();
				viewTitles.resize(project.views.size());

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2,4));

				if (BeginTable("#layout", groups.size()+1)) {

					float sidebarWidth = vsplit(config.window.width, config.sidebar.split);
					TableSetupColumn("files", ImGuiTableColumnFlags_WidthFixed, sidebarWidth);

					if (layout == 2 && groups.size() == 2U && config.layout2.split > 0.01f) {
						float leftWidth = vsplit(config.window.width, config.layout2.split);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, leftWidth);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
					}
					else {
						for (uint i = 0; i < groups.size(); i++) {
							TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						}
					}

					TableNextRow();

					TableNextColumn();
					TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ImGuiCol_FrameBg));
					Text("files");

					for (uint i = 0; i < groups.size(); i++) {
						TableNextColumn();
						auto& group = groups[i];
						if (!group.size()) continue;
						auto view = group.front();

						TableSetBgColor(ImGuiTableBgTarget_CellBg,
							GetColorU32(project.view() == view ? ImGuiCol_TitleBgActive: ImGuiCol_FrameBg)
						);

						Text("%s", view->path.c_str());
					}

					TableNextRow();

					TableNextColumn();
					PushFont(fontSidebar);
					if (BeginListBox("#open", ImVec2(-1,-1))) {
						for (uint i = 0; i < project.views.size(); i++) {
							auto view = project.views[i];
							char* title = viewTitles[i].text;
							uint size = sizeof(viewTitles[i].text);

							const char* modified = view->modified ? "*": "";
							std::snprintf(title, size, "%s%s", view->path.c_str(), modified);

							PushStyleColor(ImGuiCol_Text, view->modified ? ImColorSRGB(0xffff00ff) : GetColorU32(ImGuiCol_Text));

							if (Selectable(title, project.active == (int)i)) {
								project.active = i;
								bubble();
							}
							PopStyleColor(1);
						}
						EndListBox();
					}
					PopFont();

					PushFont(fontView);
					for (auto& group: groups) {
						TableNextColumn();
						if (!group.size()) continue;
						auto view = group.front();

						bool viewHasInput = project.view() == view
							&& !IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)
							&& !suppressInput;

						if (viewHasInput) {
							view->input();
						}

						view->draw();

						if (IsAnyMouseDown() && view->mouseOver) {
							project.active = project.find(view->path);
						}
					}
					PopFont();
					EndTable();
				}

				PopStyleVar(1);
				PushFont(fontPopup);

				auto nextPopup = [&](int h = -1) {
					h = std::min(config.window.height, h);
					int w = std::min(config.window.width, (int)vsplit(config.window.width, config.popup.width));
					SetNextWindowPos(ImVec2((config.window.width-w)/2,(config.window.height-h)/2));
					SetNextWindowSize(ImVec2(w,h));
				};

				auto filterOptions = [&](const std::vector<std::string>& haystacks, const std::string& needle) {
					std::vector<int> matches;
					for (int i = 0, l = (int)haystacks.size(); i < l; i++) {
						auto& haystack = haystacks[i];
						if (needle.size() > haystack.size()) continue;
						if (haystack.find(needle) == std::string::npos) continue;
						matches.push_back(i);
					}
					return matches;
				};

				auto selectNavigate = [&](int size, int& selected) {
					if (IsKeyPressed(KeyMap[KEY_DOWN])) selected++;
					if (IsKeyPressed(KeyMap[KEY_UP])) selected--;
					if (IsKeyPressed(KeyMap[KEY_PAGEDOWN])) selected += 10;
					if (IsKeyPressed(KeyMap[KEY_PAGEUP])) selected -= 10;
					selected = std::max(0, std::min(size-1, selected));
				};

				nextPopup();

				if (BeginPopup("#command")) {
					if (command) {
						snprintf(commandInput, sizeof(commandInput), "%s", commandPrefix);
						SetKeyboardFocusHere();
						command = false;
					}

					SetNextItemWidth(-FLT_MIN);

					if (InputTextWithHint(fmtc("#command-input-%s", commandPrefix ? commandPrefix: "any"),
						"command...", commandInput, sizeof(commandInput), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_AutoSelectAll)
					){
						project.interpret(commandInput) || project.view()->interpret(commandInput);
						CloseCurrentPopup();
					}

					if (IsKeyPressed(KeyMap[KEY_ESCAPE])) {
						CloseCurrentPopup();
					}

					EndPopup();
				}

				nextPopup(config.window.height/3*2);

				if (BeginPopup("#comp")) {
					if (comp) {
						SetKeyboardFocusHere();
						comp = false;
						compStrings.clear();
						compPrefix.clear();
						compSelected = 0;
						compInput[0] = 0;
						if (project.views.size()) {
							auto view = project.view();
							compStrings = view->autocomplete();
							if (compStrings.size()) {
								compPrefix = compStrings.front();
								compStrings.erase(compStrings.begin());
								std::snprintf(compInput, sizeof(compInput), "%s", compPrefix.c_str());
							}
						}
					}

					InputText("#comp-input", compInput, sizeof(compInput));

					if (BeginListBox("autocomplete#comp-matches", ImVec2(-1,-1))) {
						auto filter = std::string(compInput);
						std::vector<int> visible = filterOptions(compStrings, filter);
						selectNavigate(visible.size(), compSelected);

						auto compInsert = [&](std::string compString) {
							for (int i = 0; i < (int)compString.size(); i++) {
								if (i < (int)compPrefix.size()) continue;
								project.view()->insert(compString[i]);
							}
						};

						if (IsKeyPressed(KeyMap[KEY_RETURN])) {
							if (visible.size()) {
								compInsert(compStrings[visible[compSelected]]);
							}
							CloseCurrentPopup();
						}

						for (int i = 0; i < (int)visible.size(); i++) {
							auto& compString = compStrings[visible[i]];
							if (Selectable(compString.c_str(), i == compSelected)) {
								compInsert(compString);
								CloseCurrentPopup();
							}
							if (compSelected == i) {
								SetScrollHereY();
							}
						}

						EndListBox();
					}

					if (IsKeyPressed(KeyMap[KEY_ESCAPE])) {
						CloseCurrentPopup();
					}
					EndPopup();
				}

				nextPopup(config.window.height/3*2);

				if (BeginPopup("#tags")) {
					if (tags) {
						SetKeyboardFocusHere();
						tags = false;
						tagRegions.clear();
						tagStrings.clear();
						tagSelected = 0;
						tagsInput[0] = 0;
						if (project.views.size()) {
							auto view = project.view();
							auto it = view->text.begin();
							tagRegions = view->syntax->tags(view->text);
							for (auto region: tagRegions) {
								auto tag = std::string(it+region.offset, it+region.offset+region.length);
								tagStrings.push_back(tag);
							}
						}
					}

					InputText("symbol#tags-input", tagsInput, sizeof(tagsInput));

					if (BeginListBox("#tags-matches", ImVec2(-1,-1))) {
						auto filter = std::string(tagsInput);
						std::vector<int> visible = filterOptions(tagStrings, filter);
						selectNavigate(visible.size(), tagSelected);

						if (IsKeyPressed(KeyMap[KEY_RETURN])) {
							if (visible.size()) {
								auto& tagRegion = tagRegions[visible[tagSelected]];
								project.view()->single(tagRegion);
							}
							CloseCurrentPopup();
						}

						for (int i = 0; i < (int)visible.size(); i++) {
							auto& tagRegion = tagRegions[visible[i]];
							auto& tagString = tagStrings[visible[i]];

							if (Selectable(tagString.c_str(), i == tagSelected)) {
								project.view()->single(tagRegion);
								CloseCurrentPopup();
							}

							if (tagSelected == i) {
								SetScrollHereY();
							}
						}
						EndListBox();
					}

					if (IsKeyPressed(KeyMap[KEY_ESCAPE])) {
						CloseCurrentPopup();
					}
					EndPopup();
				}

				nextPopup(config.window.height/3*2);

				if (BeginPopup("#open")) {
					if (open) {
						SetKeyboardFocusHere();
						open = false;
						openPaths.clear();
						openInput[0] = 0;

						for (auto path: project.paths) {
							using namespace std::experimental::filesystem;

							for (const directory_entry& entry: recursive_directory_iterator(path)) {
								auto path = entry.path().string();
								if (path.size() > 1 && path.substr(0,2) == "./") {
									path = path.substr(2);
								}
								if (!is_regular_file(entry)) continue;
								if (path[0] == '.') continue;
								if (path == "build" || path.find("build/") == 0) continue;
								openPaths.push_back(path);
							}
						}

						std::sort(openPaths.begin(), openPaths.end());
					}

					InputText("open#open-input", openInput, sizeof(openInput));

					if (BeginListBox("#open-matches", ImVec2(-1,-1))) {
						auto filter = std::string(openInput);
						std::vector<int> visible = filterOptions(openPaths, filter);
						selectNavigate(visible.size(), openSelected);

						auto openView = [&](const std::string& openPath) {
							int active = group(project.view());
							auto view = project.open(openPath);
							if (!known(view)) {
								groups[active].push_back(view);
							}
						};

						if (IsKeyPressed(KeyMap[KEY_RETURN])) {
							if (visible.size()) {
								openView(openPaths[visible[openSelected]]);
							}
							CloseCurrentPopup();
						}

						for (int i = 0; i < (int)visible.size(); i++) {
							auto& openPath = openPaths[visible[i]];
							if (Selectable(openPath.c_str(), i == openSelected)) {
								openView(openPath);
								CloseCurrentPopup();
							}
							if (openSelected == i) {
								SetScrollHereY();
							}
						}

						EndListBox();
					}

					if (IsKeyPressed(KeyMap[KEY_ESCAPE])) {
						CloseCurrentPopup();
					}
					EndPopup();
				}
				PopFont();
				PopFont();
			End();
		}

		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
