
#include <GL/glew.h>
#include <SDL.h>

#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"
#include "imgui_tables.cpp"
#include "imgui_freetype.cpp"
#include "imgui_impl_sdl2.cpp"
#include "imgui_impl_sdlrenderer.cpp"
#include "imgui_internal.h"

#include <string>

namespace ImGui {
	void TweakDefaults() {
		// Disable CTRL+Tab shortcuts (global): assign a "None" route to steal the route to our two shortcuts
		ImGui::SetShortcutRouting(ImGuiMod_Ctrl | ImGuiKey_Tab, ImGuiKeyOwner_None);
		ImGui::SetShortcutRouting(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Tab, ImGuiKeyOwner_None);
	}

	ImU32 ImColorSRGB(unsigned int hexValue) {

		unsigned char ru = ((unsigned int)hexValue >> 24) & 0xFF;
		unsigned char gu = ((unsigned int)hexValue >> 16) & 0xFF;
		unsigned char bu = ((unsigned int)hexValue >>  8) & 0xFF;
		unsigned char au = ((unsigned int)hexValue >>  0) & 0xFF;

		float rf = (float)ru / 255.0f;
		float gf = (float)gu / 255.0f;
		float bf = (float)bu / 255.0f;
		float af = (float)au / 255.0f;

		return GetColorU32((ImVec4){rf,gf,bf,af});
	}

	void TextColored(unsigned int hexValue, const char* text) {

		unsigned char ru = ((unsigned int)hexValue >> 24) & 0xFF;
		unsigned char gu = ((unsigned int)hexValue >> 16) & 0xFF;
		unsigned char bu = ((unsigned int)hexValue >>  8) & 0xFF;
		unsigned char au = ((unsigned int)hexValue >>  0) & 0xFF;

		float rf = (float)ru / 255.0f;
		float gf = (float)gu / 255.0f;
		float bf = (float)bu / 255.0f;
		float af = (float)au / 255.0f;

		TextColored((ImVec4){rf,gf,bf,af}, "%s", text);
	}

	void Print(std::string s) {
		TextUnformatted(s.c_str());
	}

	void PrintRight(std::string s) {
		ImVec2 size = CalcTextSize(s.c_str());
		ImVec2 space = GetContentRegionAvail();
		ImGui::SetCursorPosX(GetCursorPosX() + space.x - size.x - GetStyle().ItemSpacing.x);
		TextUnformatted(s.c_str());
	}
}
