#include "includes.h"
//#include "lua/funcs.h"
#include "themes.h"
#include "res/fonts/font.h"
#include "gilua/luaHook.h"
//#include "lua/luaHook.cpp"
#include "lua/funcs.hpp"
#include "imgui/TextEditor.h"
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include "imgui/L2DFileDialog.h"
#include <chrono>
#include <thread>

#include "gilua/logtextbuf.h"
#include <Windows.h>
#include <ShObjIdl.h>
#include <ObjBase.h>

#include "json/json.hpp"
//using json = nlohmann::json;
//config json;
using namespace std;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);

}

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

bool init = false;
static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use the UNORM format to specify RGB88 color space
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.MipSlice = 0;
			pDevice->CreateRenderTargetView(pBackBuffer, &rtvDesc, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
			ImGuiIO& io = ImGui::GetIO();
			ImFontConfig fontmenu;
			fontmenu.FontDataOwnedByAtlas = false;
			/*ImFontConfig fontcoding;
			fontcoding.FontDataOwnedByAtlas = false;*/

			ImWchar ranges[] = { 0x0020, 0x00FF, 0x0100, 0x024F, 0x0370, 0x03FF, 0x0400, 0x04FF, 0x3040, 0x309F, 0x30A0, 0x30FF, 0x4E00, 0x9FBF, 0xAC00, 0xD7AF, 0xFF00, 0xFFEF, 0 };

			io.Fonts->AddFontFromMemoryTTF((void*)rawData, sizeof(rawData), 18.f, &fontmenu, ranges);
			io.Fonts->Build();

			//ImFont* jetbr = io.Fonts->AddFontFromMemoryTTF((void*)jetbrains, sizeof(jetbrains), 18.f, &fontcoding);
			//io.Fonts->Build();

			ImGui_ImplDX11_InvalidateDeviceObjects();

			ImFontConfig fontcoding;
			fontcoding.FontDataOwnedByAtlas = false;
			ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)jetbrains, sizeof(jetbrains), 18.f, &fontcoding, ranges);
			io.Fonts->Build();

			/*ImGui::GetIO().Fonts->AddFontFromMemoryTTF((void*)anime, sizeof(anime), 18.f, &fontcoding, ranges);
			io.Fonts->Build();*/

			//ImGui::GetIO().Fonts->Build();

			ImGui_ImplDX11_InvalidateDeviceObjects();
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}
	
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();

		// imgui code between newframe and render

		ImGui::NewFrame();
		ImGui::GetStyle().IndentSpacing = 16.0f;

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[fontindex_menu]);

		setlocale(LC_ALL, "C");

		static bool showEditor = false;
		static bool isopened = true;
		static bool show_compile_log = false;

		static char* file_dialog_buffer = nullptr;
		static char path3[500] = "";

		static float TimeScale = 1.0f;
		static bool themeInit = false;

		if (!themeInit)
		{
			settheme(theme_index);
			setstyle(style_index);
			themeInit = true;
		}
				
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F11)))
		{
			TimeScale = 1.0f;
			string result = "CS.UnityEngine.Time.timeScale = 1.0";
			luahookfunc(result.c_str());
		}

		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F12), false))
		{
			isopened = !isopened;
		}

		if (isopened) {
			ImGui::Begin("Dumper");
			ImGui::BeginTabBar("Dumper");

			static char Dump_Path[1024] = "";

			if (ImGui::BeginTabItem("Dumping"))
			{

				ImGui::InputTextWithHint("Dump output path", "C:/Users/User/Desktop", Dump_Path, sizeof(Dump_Path));
				ImGui::SameLine();
				HelpMarker("Provide a valid path to dump. \nProvide an existing folder as it cannot create new folders.");

				ImGui::Separator();

				if (ImGui::Button("Dump CSharp")) {
					if(strlen(Dump_Path) != 0)
					{
						string result = "local DUMP_FOLDER = \"" + string(Dump_Path) + "\"" + char_dumpcs_part1 + char_dumpcs_part2;
						luahookfunc(result.c_str());
					}
				}
				if(ImGui::IsItemHovered() && strlen(Dump_Path) != 0){
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "WARNING! This will freeze your game, give it 20 minutes or so to complete.");
				}
				else{}
				
				if(ImGui::Button("Dump current hierarchy"))
				{
					if(strlen(Dump_Path) != 0)
					{
						string result = "local dump_path = \"" + string(Dump_Path) + "\"" + char_dump_hierarchy;
						luahookfunc(result.c_str());
					}
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("About"))
			{
				// Content for About
				ImGui::Text("Genshin Impact CSharp Dumper");
				ImGui::Text("ImGui version: %s", ImGui::GetVersion());

				ImVec4 linkColor = ImVec4(34.0f / 255.0f, 132.0f / 255.0f, 230.0f / 255.0f, 1.0f);

				ImGui::Text("Forked from Minty");
				ImGui::Separator();

				ImGui::TextColored(linkColor, "Minty GitHub (click)");
				if (ImGui::IsItemClicked()) {
					system("start https://github.com/kindawindytoday/Minty");
				}
				ImGui::Separator();

				ImGui::EndTabItem();
			}
	
			ImGui::EndTabBar();
			ImGui::End();
		}
		ImGui::PopFont();
		ImGui::EndFrame();
		ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
	}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH) {
		CloseHandle(CreateThread(NULL, 0, &initLua, NULL, NULL, NULL));
		CreateThread(NULL, 0, &MainThread, NULL, NULL, NULL);
	}
	return TRUE;
}