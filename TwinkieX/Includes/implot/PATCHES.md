Changes in ImPlot v1.1:
- TwinkieX/Includes/implot/implot.h: includes <imgui/imgui.h>
- TwinkieX/Includes/implot/implot_internal.h: includes <imgui/imgui_internal.h>
- TwinkieX/Includes/implot/implot.cpp, implot_items.cpp, implot_demo.cpp: include pch.h first
- TwinkieX/Includes/implot/implot.cpp: FloorTime uses [[fallthrough]] (C5262)
- TwinkieX/Includes/implot/implot_items.cpp: disables C4626 and C5027 for the renderer structs
