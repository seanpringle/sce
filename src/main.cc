
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_sdl.h"
#include "../imgui/imgui_impl_opengl3.h"
#include <SDL.h>
#include <GL/glew.h>

#include "theme.h"
#include "config.h"
#include "project.h"
#include "view.h"
#include <filesystem>
#include <experimental/filesystem>

using namespace std::literals::chrono_literals;

Theme theme;
Config config;
Project project;

struct ViewTitle {
	char text[100] = {0};
};

std::vector<ViewTitle> viewTitles;

int main(int argc, const char* argv[])
{
	config.args(argc, argv);

	for (int i = 1; i < argc; i++) {
		project.open(argv[i]);
	}

	ensuref(0 == SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO), "%s", SDL_GetError());

	const char* glsl_version = "#version 130";

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

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

	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();

	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	auto& fonts = ImGui::GetIO().Fonts;
	fonts->Clear();

	auto fontUI = config.font.ui.face && std::filesystem::exists(config.font.ui.face)
		? fonts->AddFontFromFileTTF(config.font.ui.face, config.font.ui.size)
		: fonts->AddFontDefault();

	auto fontView = config.font.view.face && std::filesystem::exists(config.font.view.face)
		? fonts->AddFontFromFileTTF(config.font.view.face, config.font.view.size)
		: fonts->AddFontDefault();

	fonts->Build();

	ImGui_ImplOpenGL3_DestroyFontsTexture();
	ImGui_ImplOpenGL3_CreateFontsTexture();

	ImVec4 clear_color = ImVec4(0,0,0,1);

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

	for (bool done = false; !done;)
	{
		bool inputActivity = false;

		SDL_Event event;
		if (SDL_WaitEvent(&event)) {

			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
				gotFocus = now();
			}

			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
				lostFocus = now();
			}

			inputActivity = ImGui_ImplSDL2_ProcessEvent(&event)
				&& gotFocus > lostFocus && gotFocus < now() - 300ms;

			if (event.type == SDL_QUIT) {
				done = true;
			}

			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window)) {
				done = true;
			}
		}

		SDL_GetWindowSize(window, &config.window.width, &config.window.height);

		if (inputActivity) {
			using namespace ImGui;

			if (io.KeyCtrl && IsKeyDown(SDL_SCANCODE_PAGEUP)) project.active--;
			if (io.KeyCtrl && IsKeyDown(SDL_SCANCODE_PAGEDOWN)) project.active++;

			project.sanity();

			find = io.KeyCtrl && IsKeyDown(SDL_SCANCODE_F);
			line = io.KeyCtrl && IsKeyDown(SDL_SCANCODE_G);
			tags = io.KeyCtrl && IsKeyDown(SDL_SCANCODE_R);
			open = io.KeyCtrl && IsKeyDown(SDL_SCANCODE_P);
			comp = io.KeyCtrl && IsKeyDown(SDL_SCANCODE_TAB);

			if (io.KeyCtrl && IsKeyDown(SDL_SCANCODE_W) && project.view()) {
				if (!project.view()->modified) project.close();
			}
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		{
			using namespace ImGui;

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

				if (find) {
					OpenPopup("#find");
				}

				if (line) {
					OpenPopup("#line");
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

				bool viewHasInput = project.views.size()
					&& !IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)
					&& GetMousePos().x > 300
				;

				if (inputActivity && viewHasInput) {
					project.view()->input();
				}

				viewTitles.clear();
				viewTitles.resize(project.views.size());

				if (BeginTable("#layout", 2)) {
					TableSetupColumn("#side", ImGuiTableColumnFlags_WidthFixed, 300);
					TableSetupColumn("#view", ImGuiTableColumnFlags_WidthStretch);
					TableNextRow();
					TableNextColumn();
					PushFont(fontUI);
					if (BeginListBox("#open", ImVec2(300,-1))) {
						for (uint i = 0; i < project.views.size(); i++) {
							auto view = project.views[i];
							char* title = viewTitles[i].text;
							uint size = sizeof(viewTitles[i].text);

							const char* modified = view->modified ? "*": "";
							std::snprintf(title, size, "%s%s", view->path.c_str(), modified);

							PushStyleColor(ImGuiCol_Text, view->modified ? ImColorSRGB(0xffff00ff) : GetColorU32(ImGuiCol_Text));
							if (Selectable(title, project.active == (int)i)) project.active = i;
							PopStyleColor(1);
						}
						EndListBox();
					}
					PopFont();
					TableNextColumn();
					if (project.views.size()) {
						PushFont(fontView);
						project.view()->draw();
						PopFont();
					}
					EndTable();
				}

				auto nextPopup = [&](int h = -1) {
					h = std::min(config.window.height, h);
					int w = std::max(config.window.width/3, 400);
					SetNextWindowPos(ImVec2((config.window.width-w)/2,0));
					SetNextWindowSize(ImVec2(w,h));
				};

				nextPopup();

				if (BeginPopup("#find")) {
					if (find) {
						SetKeyboardFocusHere();
						find = false;
					}
					if (InputText("find#find-input", findInput, sizeof(findInput), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_AutoSelectAll)) {
						CloseCurrentPopup();
						project.view()->interpret(fmt("find %s", findInput));
					}
					if (IsKeyDown(SDL_SCANCODE_ESCAPE)) {
						CloseCurrentPopup();
					}
					EndPopup();
				}

				nextPopup();

				if (BeginPopup("#line")) {
					if (line) {
						SetKeyboardFocusHere();
						line = false;
					}
					if (InputText("line#line-input", lineInput, sizeof(lineInput), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_AutoSelectAll)) {
						CloseCurrentPopup();
						project.view()->interpret(fmt("go %s", lineInput));
					}
					if (IsKeyDown(SDL_SCANCODE_ESCAPE)) {
						CloseCurrentPopup();
					}
					EndPopup();
				}

				nextPopup(300);

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
						std::vector<int> visible;

						for (int i = 0; i < (int)compStrings.size(); i++) {
							auto& compString = compStrings[i];
							if (filter.size() > compString.size()) continue;
							if (compString.find(filter) == std::string::npos) continue;
							visible.push_back(i);
						}

						if (IsKeyDown(SDL_SCANCODE_DOWN)) compSelected++;
						if (IsKeyDown(SDL_SCANCODE_UP)) compSelected--;
						compSelected = std::max(0, std::min((int)visible.size()-1, compSelected));

						auto compInsert = [&](std::string compString) {
							for (int i = 0; i < (int)compString.size(); i++) {
								if (i < (int)compPrefix.size()) continue;
								project.view()->insert(compString[i]);
							}
						};

						if (IsKeyDown(SDL_SCANCODE_RETURN)) {
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
							if (i == compSelected) {
								SetScrollHereY();
							}
						}

						EndListBox();
					}

					if (IsKeyDown(SDL_SCANCODE_ESCAPE)) {
						CloseCurrentPopup();
					}
					EndPopup();
				}

				nextPopup(300);

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
						std::vector<int> visible;

						for (int i = 0; i < (int)tagRegions.size(); i++) {
							auto& tagString = tagStrings[i];
							if (filter.size() > tagString.size()) continue;
							if (tagString.find(filter) == std::string::npos) continue;
							visible.push_back(i);
						}

						if (IsKeyDown(SDL_SCANCODE_DOWN)) tagSelected++;
						if (IsKeyDown(SDL_SCANCODE_UP)) tagSelected--;
						tagSelected = std::max(0, std::min((int)visible.size()-1, tagSelected));

						if (IsKeyDown(SDL_SCANCODE_RETURN)) {
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
							if (i == tagSelected) {
								SetScrollHereY();
							}
						}

						EndListBox();
					}

					if (IsKeyDown(SDL_SCANCODE_ESCAPE)) {
						CloseCurrentPopup();
					}
					EndPopup();
				}

				nextPopup(300);

				if (BeginPopup("#open")) {
					if (open) {
						SetKeyboardFocusHere();
						open = false;
						openPaths.clear();
						openInput[0] = 0;

						//auto path = std::filesystem::current_path().string();

						{
							using namespace std::experimental::filesystem;

							for (const directory_entry& entry: recursive_directory_iterator(".")) {
								auto path = entry.path().string();
								if (path.size() > 1 && path.substr(0,2) == "./") {
									path = path.substr(2);
								}
								if (!is_regular_file(entry)) continue;
								if (path.find(".git") != std::string::npos) continue;
								if (path == "build" || path.find("build/") == 0) continue;
								openPaths.push_back(path);
							}
						}

						std::sort(openPaths.begin(), openPaths.end());
					}

					InputText("#open-input", openInput, sizeof(openInput));

					if (BeginListBox("#open-matches", ImVec2(-1,-1))) {
						auto filter = std::string(openInput);
						std::vector<int> visible;

						for (int i = 0; i < (int)openPaths.size(); i++) {
							auto& openPath = openPaths[i];
							if (filter.size() > openPath.size()) continue;
							if (openPath.find(filter) == std::string::npos) continue;
							visible.push_back(i);
						}

						if (IsKeyDown(SDL_SCANCODE_DOWN)) openSelected++;
						if (IsKeyDown(SDL_SCANCODE_UP)) openSelected--;
						openSelected = std::max(0, std::min((int)visible.size()-1, openSelected));

						auto openView = [&](auto& openPath) {
							project.open(openPath);
						};

						if (IsKeyDown(SDL_SCANCODE_RETURN)) {
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
							if (i == openSelected) {
								SetScrollHereY();
							}
						}

						EndListBox();
					}

					if (IsKeyDown(SDL_SCANCODE_ESCAPE)) {
						CloseCurrentPopup();
					}
					EndPopup();
				}

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
