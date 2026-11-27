TMText changes in ImGui (v1.92.4-docking):
- TwinkieX/Includes/imgui/imgui.h: ImDrawList::AddText takes ignore_tm_formatting
- TwinkieX/Includes/imgui/imgui_internal.h: ImFontCalcTextSizeEx takes ignore_tm_formatting
- TwinkieX/Includes/imgui/imgui_draw.cpp: line 1710
- TwinkieX/Includes/imgui/imgui_draw.cpp: line 5547
- TwinkieX/Includes/imgui/imgui_widgets.cpp: InputText calls ImFontCalcTextSizeEx with ignore_tm_formatting

Other changes:
- TwinkieX/Includes/imgui/imgui_impl_dx9.cpp: ImGui_ImplDX9_SwapBuffers marks hr as unused (C4189)
