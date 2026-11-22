#include "pch.h"
#include "ExportImgui.h"

namespace Gecko::Exports::ImGui
{
	struct EnumValue { const char* Name; int Value; };

	static void RegisterEnum(asIScriptEngine* Engine, const char* Name, std::initializer_list<EnumValue> Values)
	{
		Engine->RegisterEnum(Name);
		for (const auto& Value : Values)
		{
			Engine->RegisterEnumValue(Name, Value.Name, Value.Value);
		}
	}

	// The UI:: enums, with Dear ImGui's own values, since they can differ from the ones in Openplanet's Dear ImGui
	void RegisterEnums(asIScriptEngine* Engine)
	{
		Engine->SetDefaultNamespace("UI");

		/*
		!CompatMeta
		for     UI
		with    InputBlocking
		as      InputBlocking
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "InputBlocking", {
			{ "DoNothing", 0 },
			{ "Block", 1 },
		});

		/*
		!CompatMeta
		for     UI
		with    WindowFlags
		as      WindowFlags
		being   enum
		declare partial
		reason  NoDocking is 0, since this Dear ImGui has no docking
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "WindowFlags", {
			{ "None", ImGuiWindowFlags_None },
			{ "NoTitleBar", ImGuiWindowFlags_NoTitleBar },
			{ "NoResize", ImGuiWindowFlags_NoResize },
			{ "NoMove", ImGuiWindowFlags_NoMove },
			{ "NoScrollbar", ImGuiWindowFlags_NoScrollbar },
			{ "NoScrollWithMouse", ImGuiWindowFlags_NoScrollWithMouse },
			{ "NoCollapse", ImGuiWindowFlags_NoCollapse },
			{ "AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize },
			{ "NoBackground", ImGuiWindowFlags_NoBackground },
			{ "NoSavedSettings", ImGuiWindowFlags_NoSavedSettings },
			{ "NoMouseInputs", ImGuiWindowFlags_NoMouseInputs },
			{ "MenuBar", ImGuiWindowFlags_MenuBar },
			{ "HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar },
			{ "NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing },
			{ "NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus },
			{ "AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar },
			{ "AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar },
			{ "NoNavInputs", ImGuiWindowFlags_NoNavInputs },
			{ "NoNavFocus", ImGuiWindowFlags_NoNavFocus },
			{ "UnsavedDocument", ImGuiWindowFlags_UnsavedDocument },
			{ "NoDocking", 0 },
			{ "NoNav", ImGuiWindowFlags_NoNav },
			{ "NoDecoration", ImGuiWindowFlags_NoDecoration },
			{ "NoInputs", ImGuiWindowFlags_NoInputs },
		});

		/*
		!CompatMeta
		for     UI
		with    ChildFlags
		as      ChildFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "ChildFlags", {
			{ "None", ImGuiChildFlags_None },
			{ "Border", ImGuiChildFlags_Border },
			{ "Borders", ImGuiChildFlags_Borders },
			{ "AlwaysUseWindowPadding", ImGuiChildFlags_AlwaysUseWindowPadding },
			{ "ResizeX", ImGuiChildFlags_ResizeX },
			{ "ResizeY", ImGuiChildFlags_ResizeY },
			{ "AutoResizeX", ImGuiChildFlags_AutoResizeX },
			{ "AutoResizeY", ImGuiChildFlags_AutoResizeY },
			{ "AlwaysAutoResize", ImGuiChildFlags_AlwaysAutoResize },
			{ "FrameStyle", ImGuiChildFlags_FrameStyle },
			{ "NavFlattened", ImGuiChildFlags_NavFlattened },
		});

		/*
		!CompatMeta
		for     UI
		with    ButtonFlags
		as      ButtonFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "ButtonFlags", {
			{ "None", ImGuiButtonFlags_None },
			{ "MouseButtonLeft", ImGuiButtonFlags_MouseButtonLeft },
			{ "MouseButtonRight", ImGuiButtonFlags_MouseButtonRight },
			{ "MouseButtonMiddle", ImGuiButtonFlags_MouseButtonMiddle },
		});

		/*
		!CompatMeta
		for     UI
		with    Col
		as      Col
		being   enum
		declare partial
		reason  DockingPreview and DockingEmptyBg are left out, since this Dear ImGui has no docking, and so is DragDropTargetBg, which is newer than it
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "Col", {
			{ "Text", ImGuiCol_Text },
			{ "TextDisabled", ImGuiCol_TextDisabled },
			{ "WindowBg", ImGuiCol_WindowBg },
			{ "ChildBg", ImGuiCol_ChildBg },
			{ "PopupBg", ImGuiCol_PopupBg },
			{ "Border", ImGuiCol_Border },
			{ "BorderShadow", ImGuiCol_BorderShadow },
			{ "FrameBg", ImGuiCol_FrameBg },
			{ "FrameBgHovered", ImGuiCol_FrameBgHovered },
			{ "FrameBgActive", ImGuiCol_FrameBgActive },
			{ "TitleBg", ImGuiCol_TitleBg },
			{ "TitleBgActive", ImGuiCol_TitleBgActive },
			{ "TitleBgCollapsed", ImGuiCol_TitleBgCollapsed },
			{ "MenuBarBg", ImGuiCol_MenuBarBg },
			{ "ScrollbarBg", ImGuiCol_ScrollbarBg },
			{ "ScrollbarGrab", ImGuiCol_ScrollbarGrab },
			{ "ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered },
			{ "ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive },
			{ "CheckMark", ImGuiCol_CheckMark },
			{ "SliderGrab", ImGuiCol_SliderGrab },
			{ "SliderGrabActive", ImGuiCol_SliderGrabActive },
			{ "Button", ImGuiCol_Button },
			{ "ButtonHovered", ImGuiCol_ButtonHovered },
			{ "ButtonActive", ImGuiCol_ButtonActive },
			{ "Header", ImGuiCol_Header },
			{ "HeaderHovered", ImGuiCol_HeaderHovered },
			{ "HeaderActive", ImGuiCol_HeaderActive },
			{ "Separator", ImGuiCol_Separator },
			{ "SeparatorHovered", ImGuiCol_SeparatorHovered },
			{ "SeparatorActive", ImGuiCol_SeparatorActive },
			{ "ResizeGrip", ImGuiCol_ResizeGrip },
			{ "ResizeGripHovered", ImGuiCol_ResizeGripHovered },
			{ "ResizeGripActive", ImGuiCol_ResizeGripActive },
			{ "TabActive", ImGuiCol_TabActive },
			{ "TabUnfocused", ImGuiCol_TabUnfocused },
			{ "TabUnfocusedActive", ImGuiCol_TabUnfocusedActive },
			{ "Tab", ImGuiCol_Tab },
			{ "TabHovered", ImGuiCol_TabHovered },
			{ "TabSelected", ImGuiCol_TabSelected },
			{ "TabSelectedOverline", ImGuiCol_TabSelectedOverline },
			{ "TabDimmed", ImGuiCol_TabDimmed },
			{ "TabDimmedSelected", ImGuiCol_TabDimmedSelected },
			{ "TabDimmedSelectedOverline", ImGuiCol_TabDimmedSelectedOverline },
			{ "PlotLines", ImGuiCol_PlotLines },
			{ "PlotLinesHovered", ImGuiCol_PlotLinesHovered },
			{ "PlotHistogram", ImGuiCol_PlotHistogram },
			{ "PlotHistogramHovered", ImGuiCol_PlotHistogramHovered },
			{ "TableHeaderBg", ImGuiCol_TableHeaderBg },
			{ "TableBorderStrong", ImGuiCol_TableBorderStrong },
			{ "TableBorderLight", ImGuiCol_TableBorderLight },
			{ "TableRowBg", ImGuiCol_TableRowBg },
			{ "TableRowBgAlt", ImGuiCol_TableRowBgAlt },
			{ "TextLink", ImGuiCol_TextLink },
			{ "TextSelectedBg", ImGuiCol_TextSelectedBg },
			{ "DragDropTarget", ImGuiCol_DragDropTarget },
			{ "UnsavedMarker", ImGuiCol_UnsavedMarker },
			{ "NavHighlight", ImGuiCol_NavHighlight },
			{ "NavCursor", ImGuiCol_NavCursor },
			{ "NavWindowingHighlight", ImGuiCol_NavWindowingHighlight },
			{ "NavWindowingDimBg", ImGuiCol_NavWindowingDimBg },
			{ "ModalWindowDimBg", ImGuiCol_ModalWindowDimBg },
		});

		/*
		!CompatMeta
		for     UI
		with    StyleVar
		as      StyleVar
		being   enum
		declare partial
		reason  DockingSeparatorSize is left out, since this Dear ImGui has no docking
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "StyleVar", {
			{ "Alpha", ImGuiStyleVar_Alpha },
			{ "DisabledAlpha", ImGuiStyleVar_DisabledAlpha },
			{ "WindowPadding", ImGuiStyleVar_WindowPadding },
			{ "WindowRounding", ImGuiStyleVar_WindowRounding },
			{ "WindowBorderSize", ImGuiStyleVar_WindowBorderSize },
			{ "WindowMinSize", ImGuiStyleVar_WindowMinSize },
			{ "WindowTitleAlign", ImGuiStyleVar_WindowTitleAlign },
			{ "ChildRounding", ImGuiStyleVar_ChildRounding },
			{ "ChildBorderSize", ImGuiStyleVar_ChildBorderSize },
			{ "PopupRounding", ImGuiStyleVar_PopupRounding },
			{ "PopupBorderSize", ImGuiStyleVar_PopupBorderSize },
			{ "FramePadding", ImGuiStyleVar_FramePadding },
			{ "FrameRounding", ImGuiStyleVar_FrameRounding },
			{ "FrameBorderSize", ImGuiStyleVar_FrameBorderSize },
			{ "ItemSpacing", ImGuiStyleVar_ItemSpacing },
			{ "ItemInnerSpacing", ImGuiStyleVar_ItemInnerSpacing },
			{ "IndentSpacing", ImGuiStyleVar_IndentSpacing },
			{ "CellPadding", ImGuiStyleVar_CellPadding },
			{ "ScrollbarSize", ImGuiStyleVar_ScrollbarSize },
			{ "ScrollbarRounding", ImGuiStyleVar_ScrollbarRounding },
			{ "GrabMinSize", ImGuiStyleVar_GrabMinSize },
			{ "GrabRounding", ImGuiStyleVar_GrabRounding },
			{ "TabRounding", ImGuiStyleVar_TabRounding },
			{ "TabBorderSize", ImGuiStyleVar_TabBorderSize },
			{ "TabBarBorderSize", ImGuiStyleVar_TabBarBorderSize },
			{ "TabBarOverlineSize", ImGuiStyleVar_TabBarOverlineSize },
			{ "TableAngledHeadersAngle", ImGuiStyleVar_TableAngledHeadersAngle },
			{ "TableAngledHeadersTextAlign", ImGuiStyleVar_TableAngledHeadersTextAlign },
			{ "ButtonTextAlign", ImGuiStyleVar_ButtonTextAlign },
			{ "SelectableTextAlign", ImGuiStyleVar_SelectableTextAlign },
			{ "SeparatorTextBorderSize", ImGuiStyleVar_SeparatorTextBorderSize },
			{ "SeparatorTextAlign", ImGuiStyleVar_SeparatorTextAlign },
			{ "SeparatorTextPadding", ImGuiStyleVar_SeparatorTextPadding },
		});

		/*
		!CompatMeta
		for     UI
		with    FocusedFlags
		as      FocusedFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "FocusedFlags", {
			{ "None", ImGuiFocusedFlags_None },
			{ "ChildWindows", ImGuiFocusedFlags_ChildWindows },
			{ "RootWindow", ImGuiFocusedFlags_RootWindow },
			{ "AnyWindow", ImGuiFocusedFlags_AnyWindow },
			{ "RootAndChildWindows", ImGuiFocusedFlags_RootAndChildWindows },
		});

		/*
		!CompatMeta
		for     UI
		with    ComboFlags
		as      ComboFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "ComboFlags", {
			{ "None", ImGuiComboFlags_None },
			{ "PopupAlignLeft", ImGuiComboFlags_PopupAlignLeft },
			{ "HeightSmall", ImGuiComboFlags_HeightSmall },
			{ "HeightRegular", ImGuiComboFlags_HeightRegular },
			{ "HeightLarge", ImGuiComboFlags_HeightLarge },
			{ "HeightLargest", ImGuiComboFlags_HeightLargest },
			{ "NoArrowButton", ImGuiComboFlags_NoArrowButton },
			{ "NoPreview", ImGuiComboFlags_NoPreview },
		});

		/*
		!CompatMeta
		for     UI
		with    SelectableFlags
		as      SelectableFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "SelectableFlags", {
			{ "None", ImGuiSelectableFlags_None },
			{ "NoAutoClosePopups", ImGuiSelectableFlags_NoAutoClosePopups },
			{ "SpanAllColumns", ImGuiSelectableFlags_SpanAllColumns },
			{ "AllowDoubleClick", ImGuiSelectableFlags_AllowDoubleClick },
			{ "Disabled", ImGuiSelectableFlags_Disabled },
			{ "AllowOverlap", ImGuiSelectableFlags_AllowOverlap },
		});

		/*
		!CompatMeta
		for     UI
		with    TreeNodeFlags
		as      TreeNodeFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "TreeNodeFlags", {
			{ "None", ImGuiTreeNodeFlags_None },
			{ "Selected", ImGuiTreeNodeFlags_Selected },
			{ "Framed", ImGuiTreeNodeFlags_Framed },
			{ "AllowOverlap", ImGuiTreeNodeFlags_AllowOverlap },
			{ "NoTreePushOnOpen", ImGuiTreeNodeFlags_NoTreePushOnOpen },
			{ "NoAutoOpenOnLog", ImGuiTreeNodeFlags_NoAutoOpenOnLog },
			{ "DefaultOpen", ImGuiTreeNodeFlags_DefaultOpen },
			{ "OpenOnDoubleClick", ImGuiTreeNodeFlags_OpenOnDoubleClick },
			{ "OpenOnArrow", ImGuiTreeNodeFlags_OpenOnArrow },
			{ "Leaf", ImGuiTreeNodeFlags_Leaf },
			{ "Bullet", ImGuiTreeNodeFlags_Bullet },
			{ "FramePadding", ImGuiTreeNodeFlags_FramePadding },
			{ "SpanAvailWidth", ImGuiTreeNodeFlags_SpanAvailWidth },
			{ "SpanFullWidth", ImGuiTreeNodeFlags_SpanFullWidth },
			{ "SpanTextWidth", ImGuiTreeNodeFlags_SpanTextWidth },
			{ "SpanLabelWidth", ImGuiTreeNodeFlags_SpanLabelWidth },
			{ "SpanAllColumns", ImGuiTreeNodeFlags_SpanAllColumns },
			{ "NavLeftJumpsToParent", ImGuiTreeNodeFlags_NavLeftJumpsToParent },
			{ "CollapsingHeader", ImGuiTreeNodeFlags_CollapsingHeader },
		});

		/*
		!CompatMeta
		for     UI
		with    ItemFlags
		as      ItemFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "ItemFlags", {
			{ "None", ImGuiItemFlags_None },
			{ "NoTabStop", ImGuiItemFlags_NoTabStop },
			{ "NoNav", ImGuiItemFlags_NoNav },
			{ "NoNavDefaultFocus", ImGuiItemFlags_NoNavDefaultFocus },
			{ "ButtonRepeat", ImGuiItemFlags_ButtonRepeat },
			{ "AutoClosePopups", ImGuiItemFlags_AutoClosePopups },
		});

		/*
		!CompatMeta
		for     UI
		with    InputTextFlags
		as      InputTextFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "InputTextFlags", {
			{ "None", ImGuiInputTextFlags_None },
			{ "CharsDecimal", ImGuiInputTextFlags_CharsDecimal },
			{ "CharsHexadecimal", ImGuiInputTextFlags_CharsHexadecimal },
			{ "CharsScientific", ImGuiInputTextFlags_CharsScientific },
			{ "CharsUppercase", ImGuiInputTextFlags_CharsUppercase },
			{ "CharsNoBlank", ImGuiInputTextFlags_CharsNoBlank },
			{ "AllowTabInput", ImGuiInputTextFlags_AllowTabInput },
			{ "EnterReturnsTrue", ImGuiInputTextFlags_EnterReturnsTrue },
			{ "EscapeClearsAll", ImGuiInputTextFlags_EscapeClearsAll },
			{ "CtrlEnterForNewLine", ImGuiInputTextFlags_CtrlEnterForNewLine },
			{ "ReadOnly", ImGuiInputTextFlags_ReadOnly },
			{ "Password", ImGuiInputTextFlags_Password },
			{ "AlwaysOverwrite", ImGuiInputTextFlags_AlwaysOverwrite },
			{ "AutoSelectAll", ImGuiInputTextFlags_AutoSelectAll },
			{ "ParseEmptyRefVal", ImGuiInputTextFlags_ParseEmptyRefVal },
			{ "DisplayEmptyRefVal", ImGuiInputTextFlags_DisplayEmptyRefVal },
			{ "NoHorizontalScroll", ImGuiInputTextFlags_NoHorizontalScroll },
			{ "NoUndoRedo", ImGuiInputTextFlags_NoUndoRedo },
			{ "ElideLeft", ImGuiInputTextFlags_ElideLeft },
			{ "CallbackCompletion", ImGuiInputTextFlags_CallbackCompletion },
			{ "CallbackHistory", ImGuiInputTextFlags_CallbackHistory },
			{ "CallbackAlways", ImGuiInputTextFlags_CallbackAlways },
			{ "CallbackCharFilter", ImGuiInputTextFlags_CallbackCharFilter },
			{ "WordWrap", ImGuiInputTextFlags_WordWrap },
		});

		/*
		!CompatMeta
		for     UI
		with    SliderFlags
		as      SliderFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "SliderFlags", {
			{ "None", ImGuiSliderFlags_None },
			{ "AlwaysClamp", ImGuiSliderFlags_AlwaysClamp },
			{ "Logarithmic", ImGuiSliderFlags_Logarithmic },
			{ "NoRoundToFormat", ImGuiSliderFlags_NoRoundToFormat },
			{ "NoInput", ImGuiSliderFlags_NoInput },
			{ "WrapAround", ImGuiSliderFlags_WrapAround },
		});

		/*
		!CompatMeta
		for     UI
		with    ColorEditFlags
		as      ColorEditFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "ColorEditFlags", {
			{ "None", ImGuiColorEditFlags_None },
			{ "NoAlpha", ImGuiColorEditFlags_NoAlpha },
			{ "NoPicker", ImGuiColorEditFlags_NoPicker },
			{ "NoOptions", ImGuiColorEditFlags_NoOptions },
			{ "NoSmallPreview", ImGuiColorEditFlags_NoSmallPreview },
			{ "NoInputs", ImGuiColorEditFlags_NoInputs },
			{ "NoTooltip", ImGuiColorEditFlags_NoTooltip },
			{ "NoLabel", ImGuiColorEditFlags_NoLabel },
			{ "NoSidePreview", ImGuiColorEditFlags_NoSidePreview },
			{ "NoDragDrop", ImGuiColorEditFlags_NoDragDrop },
			{ "NoBorder", ImGuiColorEditFlags_NoBorder },
			{ "AlphaOpaque", ImGuiColorEditFlags_AlphaOpaque },
			{ "AlphaNoBg", ImGuiColorEditFlags_AlphaNoBg },
			{ "AlphaPreviewHalf", ImGuiColorEditFlags_AlphaPreviewHalf },
			{ "AlphaBar", ImGuiColorEditFlags_AlphaBar },
			{ "HDR", ImGuiColorEditFlags_HDR },
			{ "DisplayRGB", ImGuiColorEditFlags_DisplayRGB },
			{ "DisplayHSV", ImGuiColorEditFlags_DisplayHSV },
			{ "DisplayHex", ImGuiColorEditFlags_DisplayHex },
			{ "Uint8", ImGuiColorEditFlags_Uint8 },
			{ "Float", ImGuiColorEditFlags_Float },
			{ "PickerHueBar", ImGuiColorEditFlags_PickerHueBar },
			{ "PickerHueWheel", ImGuiColorEditFlags_PickerHueWheel },
			{ "InputRGB", ImGuiColorEditFlags_InputRGB },
			{ "InputHSV", ImGuiColorEditFlags_InputHSV },
		});

		/*
		!CompatMeta
		for     UI
		with    SeparatorFlags
		as      SeparatorFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "SeparatorFlags", {
			{ "None", ImGuiSeparatorFlags_None },
			{ "Horizontal", ImGuiSeparatorFlags_Horizontal },
			{ "Vertical", ImGuiSeparatorFlags_Vertical },
			{ "SpanAllColumns", ImGuiSeparatorFlags_SpanAllColumns },
		});

		/*
		!CompatMeta
		for     UI
		with    TableFlags
		as      TableFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "TableFlags", {
			{ "None", ImGuiTableFlags_None },
			{ "Resizable", ImGuiTableFlags_Resizable },
			{ "Reorderable", ImGuiTableFlags_Reorderable },
			{ "Hideable", ImGuiTableFlags_Hideable },
			{ "Sortable", ImGuiTableFlags_Sortable },
			{ "NoSavedSettings", ImGuiTableFlags_NoSavedSettings },
			{ "ContextMenuInBody", ImGuiTableFlags_ContextMenuInBody },
			{ "RowBg", ImGuiTableFlags_RowBg },
			{ "BordersInnerH", ImGuiTableFlags_BordersInnerH },
			{ "BordersOuterH", ImGuiTableFlags_BordersOuterH },
			{ "BordersInnerV", ImGuiTableFlags_BordersInnerV },
			{ "BordersOuterV", ImGuiTableFlags_BordersOuterV },
			{ "BordersH", ImGuiTableFlags_BordersH },
			{ "BordersV", ImGuiTableFlags_BordersV },
			{ "BordersInner", ImGuiTableFlags_BordersInner },
			{ "BordersOuter", ImGuiTableFlags_BordersOuter },
			{ "Borders", ImGuiTableFlags_Borders },
			{ "NoBordersInBody", ImGuiTableFlags_NoBordersInBody },
			{ "NoBordersInBodyUntilResize", ImGuiTableFlags_NoBordersInBodyUntilResize },
			{ "SizingFixedFit", ImGuiTableFlags_SizingFixedFit },
			{ "SizingFixedSame", ImGuiTableFlags_SizingFixedSame },
			{ "SizingStretchProp", ImGuiTableFlags_SizingStretchProp },
			{ "SizingStretchSame", ImGuiTableFlags_SizingStretchSame },
			{ "NoHostExtendX", ImGuiTableFlags_NoHostExtendX },
			{ "NoHostExtendY", ImGuiTableFlags_NoHostExtendY },
			{ "NoKeepColumnsVisible", ImGuiTableFlags_NoKeepColumnsVisible },
			{ "PreciseWidths", ImGuiTableFlags_PreciseWidths },
			{ "NoClip", ImGuiTableFlags_NoClip },
			{ "PadOuterX", ImGuiTableFlags_PadOuterX },
			{ "NoPadOuterX", ImGuiTableFlags_NoPadOuterX },
			{ "NoPadInnerX", ImGuiTableFlags_NoPadInnerX },
			{ "ScrollX", ImGuiTableFlags_ScrollX },
			{ "ScrollY", ImGuiTableFlags_ScrollY },
			{ "SortMulti", ImGuiTableFlags_SortMulti },
			{ "SortTristate", ImGuiTableFlags_SortTristate },
		});

		/*
		!CompatMeta
		for     UI
		with    TableRowFlags
		as      TableRowFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "TableRowFlags", {
			{ "None", ImGuiTableRowFlags_None },
			{ "Headers", ImGuiTableRowFlags_Headers },
		});

		/*
		!CompatMeta
		for     UI
		with    TableColumnFlags
		as      TableColumnFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "TableColumnFlags", {
			{ "None", ImGuiTableColumnFlags_None },
			{ "DefaultHide", ImGuiTableColumnFlags_DefaultHide },
			{ "DefaultSort", ImGuiTableColumnFlags_DefaultSort },
			{ "WidthStretch", ImGuiTableColumnFlags_WidthStretch },
			{ "WidthFixed", ImGuiTableColumnFlags_WidthFixed },
			{ "NoResize", ImGuiTableColumnFlags_NoResize },
			{ "NoReorder", ImGuiTableColumnFlags_NoReorder },
			{ "NoHide", ImGuiTableColumnFlags_NoHide },
			{ "NoClip", ImGuiTableColumnFlags_NoClip },
			{ "NoSort", ImGuiTableColumnFlags_NoSort },
			{ "NoSortAscending", ImGuiTableColumnFlags_NoSortAscending },
			{ "NoSortDescending", ImGuiTableColumnFlags_NoSortDescending },
			{ "NoHeaderWidth", ImGuiTableColumnFlags_NoHeaderWidth },
			{ "PreferSortAscending", ImGuiTableColumnFlags_PreferSortAscending },
			{ "PreferSortDescending", ImGuiTableColumnFlags_PreferSortDescending },
			{ "IndentEnable", ImGuiTableColumnFlags_IndentEnable },
			{ "IndentDisable", ImGuiTableColumnFlags_IndentDisable },
			{ "IsEnabled", ImGuiTableColumnFlags_IsEnabled },
			{ "IsVisible", ImGuiTableColumnFlags_IsVisible },
			{ "IsSorted", ImGuiTableColumnFlags_IsSorted },
			{ "IsHovered", ImGuiTableColumnFlags_IsHovered },
		});

		/*
		!CompatMeta
		for     UI
		with    TableBgTarget
		as      TableBgTarget
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "TableBgTarget", {
			{ "None", ImGuiTableBgTarget_None },
			{ "RowBg0", ImGuiTableBgTarget_RowBg0 },
			{ "RowBg1", ImGuiTableBgTarget_RowBg1 },
			{ "CellBg", ImGuiTableBgTarget_CellBg },
		});

		/*
		!CompatMeta
		for     UI
		with    TabBarFlags
		as      TabBarFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "TabBarFlags", {
			{ "None", ImGuiTabBarFlags_None },
			{ "Reorderable", ImGuiTabBarFlags_Reorderable },
			{ "AutoSelectNewTabs", ImGuiTabBarFlags_AutoSelectNewTabs },
			{ "TabListPopupButton", ImGuiTabBarFlags_TabListPopupButton },
			{ "NoCloseWithMiddleMouseButton", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton },
			{ "NoTabListScrollingButtons", ImGuiTabBarFlags_NoTabListScrollingButtons },
			{ "NoTooltip", ImGuiTabBarFlags_NoTooltip },
			{ "DrawSelectedOverline", ImGuiTabBarFlags_DrawSelectedOverline },
			{ "FittingPolicyResizeDown", ImGuiTabBarFlags_FittingPolicyResizeDown },
			{ "FittingPolicyShrink", ImGuiTabBarFlags_FittingPolicyShrink },
			{ "FittingPolicyScroll", ImGuiTabBarFlags_FittingPolicyScroll },
		});

		/*
		!CompatMeta
		for     UI
		with    TabItemFlags
		as      TabItemFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "TabItemFlags", {
			{ "None", ImGuiTabItemFlags_None },
			{ "UnsavedDocument", ImGuiTabItemFlags_UnsavedDocument },
			{ "SetSelected", ImGuiTabItemFlags_SetSelected },
			{ "NoCloseWithMiddleMouseButton", ImGuiTabItemFlags_NoCloseWithMiddleMouseButton },
			{ "NoPushId", ImGuiTabItemFlags_NoPushId },
			{ "NoTooltip", ImGuiTabItemFlags_NoTooltip },
			{ "NoReorder", ImGuiTabItemFlags_NoReorder },
			{ "Leading", ImGuiTabItemFlags_Leading },
			{ "Trailing", ImGuiTabItemFlags_Trailing },
		});

		/*
		!CompatMeta
		for     UI
		with    SortDirection
		as      SortDirection
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "SortDirection", {
			{ "None", ImGuiSortDirection_None },
			{ "Ascending", ImGuiSortDirection_Ascending },
			{ "Descending", ImGuiSortDirection_Descending },
		});

		/*
		!CompatMeta
		for     UI
		with    Cond
		as      Cond
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "Cond", {
			{ "None", ImGuiCond_None },
			{ "Always", ImGuiCond_Always },
			{ "Once", ImGuiCond_Once },
			{ "FirstUseEver", ImGuiCond_FirstUseEver },
			{ "Appearing", ImGuiCond_Appearing },
		});

		/*
		!CompatMeta
		for     UI
		with    Key
		as      Key
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "Key", {
			{ "None", ImGuiKey_None },
			{ "Tab", ImGuiKey_Tab },
			{ "LeftArrow", ImGuiKey_LeftArrow },
			{ "RightArrow", ImGuiKey_RightArrow },
			{ "UpArrow", ImGuiKey_UpArrow },
			{ "DownArrow", ImGuiKey_DownArrow },
			{ "PageUp", ImGuiKey_PageUp },
			{ "PageDown", ImGuiKey_PageDown },
			{ "Home", ImGuiKey_Home },
			{ "End", ImGuiKey_End },
			{ "Insert", ImGuiKey_Insert },
			{ "Delete", ImGuiKey_Delete },
			{ "Backspace", ImGuiKey_Backspace },
			{ "Space", ImGuiKey_Space },
			{ "Enter", ImGuiKey_Enter },
			{ "Escape", ImGuiKey_Escape },
			{ "LeftCtrl", ImGuiKey_LeftCtrl },
			{ "LeftShift", ImGuiKey_LeftShift },
			{ "LeftAlt", ImGuiKey_LeftAlt },
			{ "LeftSuper", ImGuiKey_LeftSuper },
			{ "RightCtrl", ImGuiKey_RightCtrl },
			{ "RightShift", ImGuiKey_RightShift },
			{ "RightAlt", ImGuiKey_RightAlt },
			{ "RightSuper", ImGuiKey_RightSuper },
			{ "Menu", ImGuiKey_Menu },
			{ "N0", ImGuiKey_0 },
			{ "N1", ImGuiKey_1 },
			{ "N2", ImGuiKey_2 },
			{ "N3", ImGuiKey_3 },
			{ "N4", ImGuiKey_4 },
			{ "N5", ImGuiKey_5 },
			{ "N6", ImGuiKey_6 },
			{ "N7", ImGuiKey_7 },
			{ "N8", ImGuiKey_8 },
			{ "N9", ImGuiKey_9 },
			{ "A", ImGuiKey_A },
			{ "B", ImGuiKey_B },
			{ "C", ImGuiKey_C },
			{ "D", ImGuiKey_D },
			{ "E", ImGuiKey_E },
			{ "F", ImGuiKey_F },
			{ "G", ImGuiKey_G },
			{ "H", ImGuiKey_H },
			{ "I", ImGuiKey_I },
			{ "J", ImGuiKey_J },
			{ "K", ImGuiKey_K },
			{ "L", ImGuiKey_L },
			{ "M", ImGuiKey_M },
			{ "N", ImGuiKey_N },
			{ "O", ImGuiKey_O },
			{ "P", ImGuiKey_P },
			{ "Q", ImGuiKey_Q },
			{ "R", ImGuiKey_R },
			{ "S", ImGuiKey_S },
			{ "T", ImGuiKey_T },
			{ "U", ImGuiKey_U },
			{ "V", ImGuiKey_V },
			{ "W", ImGuiKey_W },
			{ "X", ImGuiKey_X },
			{ "Y", ImGuiKey_Y },
			{ "Z", ImGuiKey_Z },
			{ "F1", ImGuiKey_F1 },
			{ "F2", ImGuiKey_F2 },
			{ "F3", ImGuiKey_F3 },
			{ "F4", ImGuiKey_F4 },
			{ "F5", ImGuiKey_F5 },
			{ "F6", ImGuiKey_F6 },
			{ "F7", ImGuiKey_F7 },
			{ "F8", ImGuiKey_F8 },
			{ "F9", ImGuiKey_F9 },
			{ "F10", ImGuiKey_F10 },
			{ "F11", ImGuiKey_F11 },
			{ "F12", ImGuiKey_F12 },
			{ "F13", ImGuiKey_F13 },
			{ "F14", ImGuiKey_F14 },
			{ "F15", ImGuiKey_F15 },
			{ "F16", ImGuiKey_F16 },
			{ "F17", ImGuiKey_F17 },
			{ "F18", ImGuiKey_F18 },
			{ "F19", ImGuiKey_F19 },
			{ "F20", ImGuiKey_F20 },
			{ "F21", ImGuiKey_F21 },
			{ "F22", ImGuiKey_F22 },
			{ "F23", ImGuiKey_F23 },
			{ "F24", ImGuiKey_F24 },
			{ "Apostrophe", ImGuiKey_Apostrophe },
			{ "Comma", ImGuiKey_Comma },
			{ "Minus", ImGuiKey_Minus },
			{ "Period", ImGuiKey_Period },
			{ "Slash", ImGuiKey_Slash },
			{ "Semicolon", ImGuiKey_Semicolon },
			{ "Equal", ImGuiKey_Equal },
			{ "LeftBracket", ImGuiKey_LeftBracket },
			{ "Backslash", ImGuiKey_Backslash },
			{ "RightBracket", ImGuiKey_RightBracket },
			{ "GraveAccent", ImGuiKey_GraveAccent },
			{ "CapsLock", ImGuiKey_CapsLock },
			{ "ScrollLock", ImGuiKey_ScrollLock },
			{ "NumLock", ImGuiKey_NumLock },
			{ "PrintScreen", ImGuiKey_PrintScreen },
			{ "Pause", ImGuiKey_Pause },
			{ "Keypad0", ImGuiKey_Keypad0 },
			{ "Keypad1", ImGuiKey_Keypad1 },
			{ "Keypad2", ImGuiKey_Keypad2 },
			{ "Keypad3", ImGuiKey_Keypad3 },
			{ "Keypad4", ImGuiKey_Keypad4 },
			{ "Keypad5", ImGuiKey_Keypad5 },
			{ "Keypad6", ImGuiKey_Keypad6 },
			{ "Keypad7", ImGuiKey_Keypad7 },
			{ "Keypad8", ImGuiKey_Keypad8 },
			{ "Keypad9", ImGuiKey_Keypad9 },
			{ "KeypadDecimal", ImGuiKey_KeypadDecimal },
			{ "KeypadDivide", ImGuiKey_KeypadDivide },
			{ "KeypadMultiply", ImGuiKey_KeypadMultiply },
			{ "KeypadSubtract", ImGuiKey_KeypadSubtract },
			{ "KeypadAdd", ImGuiKey_KeypadAdd },
			{ "KeypadEnter", ImGuiKey_KeypadEnter },
			{ "KeyPadEnter", ImGuiKey_KeypadEnter },
			{ "KeypadEqual", ImGuiKey_KeypadEqual },
			{ "AppBack", ImGuiKey_AppBack },
			{ "AppForward", ImGuiKey_AppForward },
			{ "MouseLeft", ImGuiKey_MouseLeft },
			{ "MouseRight", ImGuiKey_MouseRight },
			{ "MouseMiddle", ImGuiKey_MouseMiddle },
			{ "MouseX1", ImGuiKey_MouseX1 },
			{ "MouseX2", ImGuiKey_MouseX2 },
			{ "MouseWheelX", ImGuiKey_MouseWheelX },
			{ "MouseWheelY", ImGuiKey_MouseWheelY },
			{ "ModCtrl", ImGuiKey_ModCtrl },
			{ "ModShift", ImGuiKey_ModShift },
			{ "ModAlt", ImGuiKey_ModAlt },
			{ "ModSuper", ImGuiKey_ModSuper },
		});

		/*
		!CompatMeta
		for     UI
		with    MouseButton
		as      MouseButton
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "MouseButton", {
			{ "Left", ImGuiMouseButton_Left },
			{ "Right", ImGuiMouseButton_Right },
			{ "Middle", ImGuiMouseButton_Middle },
		});

		/*
		!CompatMeta
		for     UI
		with    HoveredFlags
		as      HoveredFlags
		being   enum
		declare partial
		reason  DockHierarchy is 0, since this Dear ImGui has no docking
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "HoveredFlags", {
			{ "None", ImGuiHoveredFlags_None },
			{ "ChildWindows", ImGuiHoveredFlags_ChildWindows },
			{ "RootWindow", ImGuiHoveredFlags_RootWindow },
			{ "AnyWindow", ImGuiHoveredFlags_AnyWindow },
			{ "NoPopupHierarchy", ImGuiHoveredFlags_NoPopupHierarchy },
			{ "DockHierarchy", 0 },
			{ "AllowWhenBlockedByPopup", ImGuiHoveredFlags_AllowWhenBlockedByPopup },
			{ "AllowWhenBlockedByActiveItem", ImGuiHoveredFlags_AllowWhenBlockedByActiveItem },
			{ "AllowWhenOverlappedByItem", ImGuiHoveredFlags_AllowWhenOverlappedByItem },
			{ "AllowWhenOverlappedByWindow", ImGuiHoveredFlags_AllowWhenOverlappedByWindow },
			{ "AllowWhenDisabled", ImGuiHoveredFlags_AllowWhenDisabled },
			{ "NoNavOverride", ImGuiHoveredFlags_NoNavOverride },
			{ "AllowWhenOverlapped", ImGuiHoveredFlags_AllowWhenOverlapped },
			{ "RectOnly", ImGuiHoveredFlags_RectOnly },
			{ "RootAndChildWindows", ImGuiHoveredFlags_RootAndChildWindows },
			{ "ForTooltip", ImGuiHoveredFlags_ForTooltip },
			{ "Stationary", ImGuiHoveredFlags_Stationary },
			{ "DelayNone", ImGuiHoveredFlags_DelayNone },
			{ "DelayShort", ImGuiHoveredFlags_DelayShort },
			{ "DelayNormal", ImGuiHoveredFlags_DelayNormal },
			{ "NoSharedDelay", ImGuiHoveredFlags_NoSharedDelay },
		});

		/*
		!CompatMeta
		for     UI
		with    InputFlags
		as      InputFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "InputFlags", {
			{ "None", ImGuiInputFlags_None },
			{ "Repeat", ImGuiInputFlags_Repeat },
			{ "RouteActive", ImGuiInputFlags_RouteActive },
			{ "RouteFocused", ImGuiInputFlags_RouteFocused },
			{ "RouteGlobal", ImGuiInputFlags_RouteGlobal },
			{ "RouteAlways", ImGuiInputFlags_RouteAlways },
			{ "RouteOverFocused", ImGuiInputFlags_RouteOverFocused },
			{ "RouteOverActive", ImGuiInputFlags_RouteOverActive },
			{ "RouteUnlessBgFocused", ImGuiInputFlags_RouteUnlessBgFocused },
			{ "RouteFromRootWindow", ImGuiInputFlags_RouteFromRootWindow },
			{ "Tooltip", ImGuiInputFlags_Tooltip },
		});

		/*
		!CompatMeta
		for     UI
		with    MouseCursor
		as      MouseCursor
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterEnum(Engine, "MouseCursor", {
			{ "None", ImGuiMouseCursor_None },
			{ "Arrow", ImGuiMouseCursor_Arrow },
			{ "TextInput", ImGuiMouseCursor_TextInput },
			{ "ResizeAll", ImGuiMouseCursor_ResizeAll },
			{ "ResizeNS", ImGuiMouseCursor_ResizeNS },
			{ "ResizeEW", ImGuiMouseCursor_ResizeEW },
			{ "ResizeNESW", ImGuiMouseCursor_ResizeNESW },
			{ "ResizeNWSE", ImGuiMouseCursor_ResizeNWSE },
			{ "Hand", ImGuiMouseCursor_Hand },
			{ "NotAllowed", ImGuiMouseCursor_NotAllowed },
		});

		Engine->SetDefaultNamespace("");
	}
}
