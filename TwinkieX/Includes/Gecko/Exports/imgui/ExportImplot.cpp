#include "pch.h"
#include "ExportImgui.h"
#include <Gecko/Exports/GeckoAutoExport.h>
#include <Gecko/Exports/Math/Math.h>
#include <implot/implot_internal.h>

namespace Gecko::Exports::ImGui
{
	using Math::vec2;
	using Math::vec4;

	static std::vector<GeckoExport> s_PlotExports;

#define PLOT_EXPORT(Decl_, Fn_) GECKO_EXPORT_FN(s_PlotExports, "UI::Plot", Decl_, Fn_)

	struct PlotEnumValue { const char* Name; int Value; };

	static void RegisterPlotEnum(asIScriptEngine* Engine, const char* Name, std::initializer_list<PlotEnumValue> Values)
	{
		Engine->RegisterEnum(Name);
		for (const auto& Value : Values)
		{
			Engine->RegisterEnumValue(Name, Value.Name, Value.Value);
		}
	}

	static ImPlotSpec s_NextSpec;

	static bool CanRender()
	{
		return TheScriptManager && TheScriptManager->IsExecutingRenderCallback() && ImPlot::GetCurrentContext();
	}

	static bool InPlot()
	{
		return CanRender() && GImPlot->CurrentPlot;
	}

	static bool InSetup()
	{
		return InPlot() && !GImPlot->CurrentPlot->SetupLocked;
	}

	static bool InSubplots()
	{
		return CanRender() && GImPlot->CurrentSubplot;
	}

	static bool IsAxis(int Axis)
	{
		return Axis >= 0 && Axis < ImAxis_COUNT;
	}

	static bool IsAxisOrAuto(int Axis)
	{
		return Axis == IMPLOT_AUTO || IsAxis(Axis);
	}

	static ImPlotSpec TakeSpec(int Flags)
	{
		const ImPlotSpec& Next = s_NextSpec;
		ImPlotSpec Spec(Next);
		s_NextSpec = ImPlotSpec();
		Spec.Flags = Flags;
		return Spec;
	}

	static ImVec2 ToIm(const vec2& V) { return { V.x, V.y }; }
	static ImVec4 ToIm(const vec4& V) { return { V.x, V.y, V.z, V.w }; }
	static vec2 FromIm(const ImVec2& V) { return { V.x, V.y }; }
	static vec2 FromPoint(const ImPlotPoint& P) { return { (float)P.x, (float)P.y }; }
	static vec4 FromRect(const ImPlotRect& R) { return { (float)R.X.Min, (float)R.Y.Min, (float)R.X.Max, (float)R.Y.Max }; }

	static const float* FloatsOf(CScriptArray* Array) { return Array ? (const float*)Array->GetBuffer() : nullptr; }
	static int CountOf(CScriptArray* Array) { return Array ? (int)Array->GetSize() : 0; }

	static std::vector<const char*> StringsOf(CScriptArray* Array)
	{
		std::vector<const char*> Strings;
		for (asUINT Idx = 0; Array && Idx < Array->GetSize(); Idx++) Strings.push_back(((const std::string*)Array->At(Idx))->c_str());
		return Strings;
	}

	static const char* OrNull(const std::string& Str) { return Str.empty() ? nullptr : Str.c_str(); }

	static bool BeginPlot(const std::string& TitleId, const vec2& Size, int Flags)
	{
		return CanRender() && ImPlot::BeginPlot(TitleId.c_str(), ToIm(Size), Flags);
	}

	static void EndPlot()
	{
		if (InPlot()) ImPlot::EndPlot();
	}

	static bool BeginSubplots(const std::string& TitleId, int Rows, int Cols, const vec2& Size, int Flags)
	{
		return CanRender() && Rows > 0 && Cols > 0 && ImPlot::BeginSubplots(TitleId.c_str(), Rows, Cols, ToIm(Size), Flags);
	}

	static void EndSubplots()
	{
		if (InSubplots()) ImPlot::EndSubplots();
	}

	static void SetupAxis(int Axis, const std::string& Label, int Flags)
	{
		if (InSetup() && IsAxis(Axis)) ImPlot::SetupAxis(Axis, OrNull(Label), Flags);
	}

	static void SetupAxisTicksValues(int Axis, CScriptArray* Values, CScriptArray* Labels, bool KeepDefault)
	{
		if (!InSetup() || !IsAxis(Axis) || !Values || Values->GetSize() == 0) return;

		std::vector<const char*> Strings = StringsOf(Labels);
		bool HasLabels = Labels && Strings.size() >= Values->GetSize();
		ImPlot::SetupAxisTicks(Axis, (const double*)Values->GetBuffer(), (int)Values->GetSize(), HasLabels ? Strings.data() : nullptr, KeepDefault);
	}

	static void SetupAxisTicksRange(int Axis, double Min, double Max, CScriptArray* Labels, bool KeepDefault)
	{
		if (Labels && Labels->GetSize() != 2)
		{
			if (asIScriptContext* Ctx = asGetActiveContext()) Ctx->SetException("2 labels are required");
			return;
		}
		if (!InSetup() || !IsAxis(Axis)) return;

		std::vector<const char*> Strings = StringsOf(Labels);
		ImPlot::SetupAxisTicks(Axis, Min, Max, 2, Labels ? Strings.data() : nullptr, KeepDefault);
	}

	static void SetupAxisLimits(int Axis, double Min, double Max, int Cond)
	{
		if (InSetup() && IsAxis(Axis)) ImPlot::SetupAxisLimits(Axis, Min, Max, Cond);
	}

	static void SetupAxisFormat(int Axis, const std::string& Format)
	{
		if (InSetup() && IsAxis(Axis)) ImPlot::SetupAxisFormat(Axis, Format.c_str());
	}

	static void SetupAxisScale(int Axis, int Scale)
	{
		if (InSetup() && IsAxis(Axis)) ImPlot::SetupAxisScale(Axis, Scale);
	}

	static void SetupAxisLimitsConstraints(int Axis, double Min, double Max)
	{
		if (InSetup() && IsAxis(Axis)) ImPlot::SetupAxisLimitsConstraints(Axis, Min, Max);
	}

	static void SetupAxisZoomConstraints(int Axis, double Min, double Max)
	{
		if (InSetup() && IsAxis(Axis)) ImPlot::SetupAxisZoomConstraints(Axis, Min, Max);
	}

	static void SetupAxes(const std::string& XLabel, const std::string& YLabel, int XFlags, int YFlags)
	{
		if (InSetup()) ImPlot::SetupAxes(OrNull(XLabel), OrNull(YLabel), XFlags, YFlags);
	}

	static void SetupAxesLimits(double XMin, double XMax, double YMin, double YMax, int Cond)
	{
		if (InSetup()) ImPlot::SetupAxesLimits(XMin, XMax, YMin, YMax, Cond);
	}

	static void SetupLegend(int Location, int Flags)
	{
		if (InSetup()) ImPlot::SetupLegend(Location, Flags);
	}

	static void SetupMouseText(int Location, int Flags)
	{
		if (InSetup()) ImPlot::SetupMouseText(Location, Flags);
	}

	static void PushColormap(int Colormap)
	{
		if (CanRender() && Colormap >= 0 && Colormap < ImPlot::GetColormapCount()) ImPlot::PushColormap(Colormap);
	}

	static void PopColormap(int Count)
	{
		if (CanRender() && Count > 0 && Count <= GImPlot->ColormapModifiers.Size) ImPlot::PopColormap(Count);
	}

	static void SetNextLineStyle(const vec4& Color, float Weight)
	{
		s_NextSpec.LineColor = ToIm(Color);
		if (Weight >= 0.0f) s_NextSpec.LineWeight = Weight;
	}

	static void SetNextFillStyle(const vec4& Color, float AlphaMod)
	{
		s_NextSpec.FillColor = ToIm(Color);
		if (AlphaMod >= 0.0f) s_NextSpec.FillAlpha = AlphaMod;
	}

	static void SetNextMarkerStyle(int Marker, float Size, const vec4& Fill, float Weight, const vec4& Outline)
	{
		s_NextSpec.Marker = Marker;
		if (Size >= 0.0f) s_NextSpec.MarkerSize = Size;
		s_NextSpec.MarkerFillColor = ToIm(Fill);
		if (Weight >= 0.0f) s_NextSpec.LineWeight = Weight;
		s_NextSpec.MarkerLineColor = ToIm(Outline);
	}

	static void SetNextErrorBarStyle(const vec4& Color, float Size, float Weight)
	{
		s_NextSpec.LineColor = ToIm(Color);
		if (Size >= 0.0f) s_NextSpec.Size = Size;
		if (Weight >= 0.0f) s_NextSpec.LineWeight = Weight;
	}

	static void ColormapScale(const std::string& Label, double ScaleMin, double ScaleMax, const vec2& Size, const std::string& Format, int Flags, int Colormap)
	{
		if (!CanRender() || (Colormap != IMPLOT_AUTO && (Colormap < 0 || Colormap >= ImPlot::GetColormapCount()))) return;
		ImPlot::ColormapScale(Label.c_str(), ScaleMin, ScaleMax, ToIm(Size), Format.c_str(), Flags, Colormap);
	}

	static void PlotLineValues(const std::string& LabelId, CScriptArray* Values, double XScale, double XStart, int Flags)
	{
		if (InPlot() && Values) ImPlot::PlotLine(LabelId.c_str(), FloatsOf(Values), CountOf(Values), XScale, XStart, TakeSpec(Flags));
	}

	static void PlotLineXY(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, int Flags)
	{
		if (InPlot() && Xs && Ys) ImPlot::PlotLine(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), (std::min)(CountOf(Xs), CountOf(Ys)), TakeSpec(Flags));
	}

	static void PlotBarsValues(const std::string& LabelId, CScriptArray* Values, double BarSize, double Shift, int Flags)
	{
		if (InPlot() && Values) ImPlot::PlotBars(LabelId.c_str(), FloatsOf(Values), CountOf(Values), BarSize, Shift, TakeSpec(Flags));
	}

	static void PlotBarsXY(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, double BarSize, int Flags)
	{
		if (InPlot() && Xs && Ys) ImPlot::PlotBars(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), (std::min)(CountOf(Xs), CountOf(Ys)), BarSize, TakeSpec(Flags));
	}

	static void PlotErrorBars(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, CScriptArray* Err, int Flags)
	{
		if (!InPlot() || !Xs || !Ys || !Err) return;
		int Count = (std::min)({ CountOf(Xs), CountOf(Ys), CountOf(Err) });
		ImPlot::PlotErrorBars(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), FloatsOf(Err), Count, TakeSpec(Flags));
	}

	static void PlotErrorBarsNegPos(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, CScriptArray* Neg, CScriptArray* Pos, int Flags)
	{
		if (!InPlot() || !Xs || !Ys || !Neg || !Pos) return;
		int Count = (std::min)({ CountOf(Xs), CountOf(Ys), CountOf(Neg), CountOf(Pos) });
		ImPlot::PlotErrorBars(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), FloatsOf(Neg), FloatsOf(Pos), Count, TakeSpec(Flags));
	}

	static void PlotShadedValues(const std::string& LabelId, CScriptArray* Values, double YRef, double XScale, double XStart, int Flags)
	{
		if (InPlot() && Values) ImPlot::PlotShaded(LabelId.c_str(), FloatsOf(Values), CountOf(Values), YRef, XScale, XStart, TakeSpec(Flags));
	}

	static void PlotShadedXY(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, double YRef, int Flags)
	{
		if (InPlot() && Xs && Ys) ImPlot::PlotShaded(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), (std::min)(CountOf(Xs), CountOf(Ys)), YRef, TakeSpec(Flags));
	}

	static void PlotShadedBetween(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys1, CScriptArray* Ys2, int Flags)
	{
		if (!InPlot() || !Xs || !Ys1 || !Ys2) return;
		int Count = (std::min)({ CountOf(Xs), CountOf(Ys1), CountOf(Ys2) });
		ImPlot::PlotShaded(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys1), FloatsOf(Ys2), Count, TakeSpec(Flags));
	}

	static void PlotScatterValues(const std::string& LabelId, CScriptArray* Values, double XScale, double XStart, int Flags)
	{
		if (InPlot() && Values) ImPlot::PlotScatter(LabelId.c_str(), FloatsOf(Values), CountOf(Values), XScale, XStart, TakeSpec(Flags));
	}

	static void PlotScatterXY(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, int Flags)
	{
		if (InPlot() && Xs && Ys) ImPlot::PlotScatter(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), (std::min)(CountOf(Xs), CountOf(Ys)), TakeSpec(Flags));
	}

	static void PlotBarGroups(CScriptArray* LabelIds, CScriptArray* Values, double GroupSize, double Shift, int Flags)
	{
		if (!InPlot() || !LabelIds || !Values) return;

		std::vector<const char*> Labels = StringsOf(LabelIds);
		int ItemCount = (int)Labels.size();
		int GroupCount = ItemCount > 0 ? CountOf(Values) / ItemCount : 0;
		if (GroupCount > 0) ImPlot::PlotBarGroups(Labels.data(), FloatsOf(Values), ItemCount, GroupCount, GroupSize, Shift, TakeSpec(Flags));
	}

	static void PlotStemsValues(const std::string& LabelId, CScriptArray* Values, double Ref, double Scale, double Start, int Flags)
	{
		if (InPlot() && Values) ImPlot::PlotStems(LabelId.c_str(), FloatsOf(Values), CountOf(Values), Ref, Scale, Start, TakeSpec(Flags));
	}

	static void PlotStemsXY(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, double Ref, int Flags)
	{
		if (InPlot() && Xs && Ys) ImPlot::PlotStems(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), (std::min)(CountOf(Xs), CountOf(Ys)), Ref, TakeSpec(Flags));
	}

	static void PlotStairsValues(const std::string& LabelId, CScriptArray* Values, double XScale, double XStart, int Flags)
	{
		if (InPlot() && Values) ImPlot::PlotStairs(LabelId.c_str(), FloatsOf(Values), CountOf(Values), XScale, XStart, TakeSpec(Flags));
	}

	static void PlotStairsXY(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, int Flags)
	{
		if (InPlot() && Xs && Ys) ImPlot::PlotStairs(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), (std::min)(CountOf(Xs), CountOf(Ys)), TakeSpec(Flags));
	}

	static void PlotPieChart(CScriptArray* LabelIds, CScriptArray* Values, double X, double Y, double Radius, const std::string& LabelFormat, double Angle0, int Flags)
	{
		if (!InPlot() || !LabelIds || !Values) return;

		std::vector<const char*> Labels = StringsOf(LabelIds);
		int Count = (std::min)((int)Labels.size(), CountOf(Values));
		if (Count > 0) ImPlot::PlotPieChart(Labels.data(), FloatsOf(Values), Count, X, Y, Radius, OrNull(LabelFormat), Angle0, TakeSpec(Flags));
	}

	static void PlotHeatmap(const std::string& LabelId, CScriptArray* Values, int Rows, int Cols, double ScaleMin, double ScaleMax, const std::string& LabelFormat, const vec2& BoundsMin, const vec2& BoundsMax, int Flags)
	{
		if (!InPlot() || !Values || Rows <= 0 || Cols <= 0 || (int64_t)Rows * Cols > CountOf(Values)) return;
		ImPlot::PlotHeatmap(LabelId.c_str(), FloatsOf(Values), Rows, Cols, ScaleMin, ScaleMax, OrNull(LabelFormat), ImPlotPoint(BoundsMin.x, BoundsMin.y), ImPlotPoint(BoundsMax.x, BoundsMax.y), TakeSpec(Flags));
	}

	static double PlotHistogram(const std::string& LabelId, CScriptArray* Values, int Bins, double BarScale, double Min, double Max, int Flags)
	{
		if (!InPlot() || !Values) return 0.0;
		return ImPlot::PlotHistogram(LabelId.c_str(), FloatsOf(Values), CountOf(Values), Bins, BarScale, ImPlotRange(Min, Max), TakeSpec(Flags));
	}

	static double PlotHistogram2D(const std::string& LabelId, CScriptArray* Xs, CScriptArray* Ys, int XBins, int YBins, const vec2& RangeMin, const vec2& RangeMax, int Flags)
	{
		if (!InPlot() || !Xs || !Ys) return 0.0;
		ImPlotRect Range(RangeMin.x, RangeMax.x, RangeMin.y, RangeMax.y);
		return ImPlot::PlotHistogram2D(LabelId.c_str(), FloatsOf(Xs), FloatsOf(Ys), (std::min)(CountOf(Xs), CountOf(Ys)), XBins, YBins, Range, TakeSpec(Flags));
	}

	static void SetAxis(int Axis)
	{
		if (InPlot() && IsAxis(Axis)) ImPlot::SetAxis(Axis);
	}

	static void SetAxes(int XAxis, int YAxis)
	{
		if (InPlot() && IsAxis(XAxis) && IsAxis(YAxis)) ImPlot::SetAxes(XAxis, YAxis);
	}

	static vec2 PixelsToPlotVec(const vec2& Pixels, int XAxis, int YAxis)
	{
		if (!InPlot() || !IsAxisOrAuto(XAxis) || !IsAxisOrAuto(YAxis)) return {};
		return FromPoint(ImPlot::PixelsToPlot(ToIm(Pixels), XAxis, YAxis));
	}

	static vec2 PixelsToPlotXY(float X, float Y, int XAxis, int YAxis)
	{
		if (!InPlot() || !IsAxisOrAuto(XAxis) || !IsAxisOrAuto(YAxis)) return {};
		return FromPoint(ImPlot::PixelsToPlot(X, Y, XAxis, YAxis));
	}

	static vec2 PlotToPixelsVec(const vec2& Plot, int XAxis, int YAxis)
	{
		if (!InPlot() || !IsAxisOrAuto(XAxis) || !IsAxisOrAuto(YAxis)) return {};
		return FromIm(ImPlot::PlotToPixels(ImPlotPoint(Plot.x, Plot.y), XAxis, YAxis));
	}

	static vec2 PlotToPixelsXY(double X, double Y, int XAxis, int YAxis)
	{
		if (!InPlot() || !IsAxisOrAuto(XAxis) || !IsAxisOrAuto(YAxis)) return {};
		return FromIm(ImPlot::PlotToPixels(X, Y, XAxis, YAxis));
	}

	static vec2 GetPlotPos()
	{
		return InPlot() ? FromIm(ImPlot::GetPlotPos()) : vec2{};
	}

	static vec2 GetPlotSize()
	{
		return InPlot() ? FromIm(ImPlot::GetPlotSize()) : vec2{};
	}

	static vec2 GetPlotMousePos(int XAxis, int YAxis)
	{
		if (!InPlot() || !IsAxisOrAuto(XAxis) || !IsAxisOrAuto(YAxis)) return {};
		return FromPoint(ImPlot::GetPlotMousePos(XAxis, YAxis));
	}

	static vec4 GetPlotLimits(int XAxis, int YAxis)
	{
		if (!InPlot() || !IsAxisOrAuto(XAxis) || !IsAxisOrAuto(YAxis)) return {};
		return FromRect(ImPlot::GetPlotLimits(XAxis, YAxis));
	}

	static bool IsPlotHovered()
	{
		return InPlot() && ImPlot::IsPlotHovered();
	}

	static bool IsAxisHovered(int Axis)
	{
		return InPlot() && IsAxis(Axis) && ImPlot::IsAxisHovered(Axis);
	}

	static bool IsSubplotsHovered()
	{
		return InSubplots() && ImPlot::IsSubplotsHovered();
	}

	static bool IsPlotSelected()
	{
		return InPlot() && ImPlot::IsPlotSelected();
	}

	static vec4 GetPlotSelection(int XAxis, int YAxis)
	{
		if (!InPlot() || !IsAxisOrAuto(XAxis) || !IsAxisOrAuto(YAxis)) return {};
		return FromRect(ImPlot::GetPlotSelection(XAxis, YAxis));
	}

	static void CancelPlotSelection()
	{
		if (InPlot()) ImPlot::CancelPlotSelection();
	}

	static void HideNextItem(bool Hidden, int Cond)
	{
		if (InPlot()) ImPlot::HideNextItem(Hidden, Cond);
	}

	/*
	!CompatMeta
	for     UI::Plot
	with    bool BeginPlot(const string&in title_id, const vec2&in size = vec2(-1, 0), int flags = UI::Plot::PlotFlags::None)
	with    void EndPlot()
	with    bool BeginSubplots(const string&in title_id, int rows, int cols, const vec2&in size, int flags = UI::Plot::SubplotFlags::None)
	with    void EndSubplots()
	with    void SetupAxis(Axis axis, const string&in label = "", int flags = UI::Plot::AxisFlags::None)
	with    void SetupAxisTicks(Axis axis, const double[]@ values, const string[]@ labels = null, bool keep_default = false)
	with    void SetupAxisLimits(Axis axis, double v_min, double v_max, Cond cond = UI::Plot::Cond::Once)
	with    void SetupAxisFormat(Axis axis, const string&in fmt)
	with    void SetupAxisScale(Axis axis, Scale scale)
	with    void SetupAxisLimitsConstraints(Axis axis, double v_min, double v_max)
	with    void SetupAxisZoomConstraints(Axis axis, double z_min, double z_max)
	with    void SetupAxes(const string&in x_label, const string&in y_label, int x_flags = UI::Plot::AxisFlags::None, int y_flags = UI::Plot::AxisFlags::None)
	with    void SetupAxesLimits(double x_min, double x_max, double y_min, double y_max, Cond cond = UI::Plot::Cond::Once)
	with    void SetupLegend(Location location, int flags = UI::Plot::LegendFlags::None)
	with    void SetupMouseText(Location location, int flags = UI::Plot::MouseTextFlags::None)
	with    void PushColormap(Colormap cmap)
	with    void PopColormap(int count = 1)
	with    void SetNextLineStyle(const vec4&in col = vec4(0, 0, 0, -1), float weight = -1.0f)
	with    void SetNextFillStyle(const vec4&in col = vec4(0, 0, 0, -1), float alpha_mod = -1.0f)
	with    void SetNextMarkerStyle(Marker marker = UI::Plot::Marker::Auto, float size = -1.0f, const vec4&in fill = vec4(0, 0, 0, -1), float weight = -1.0f, const vec4&in outline = vec4(0, 0, 0, -1))
	with    void SetNextErrorBarStyle(const vec4&in col = vec4(0, 0, 0, -1), float size = -1.0f, float weight = -1.0f)
	with    void ColormapScale(const string&in label, double scale_min, double scale_max, const vec2&in size = vec2(), const string&in format = "%g", int flags = UI::Plot::ColormapScaleFlags::None, Colormap cmap = UI::Plot::Colormap::Auto)
	with    void PlotLine(const string&in label_id, const float[]@ values, double x_scale = 1.0, double x_start = 0.0, int flags = UI::Plot::LineFlags::None)
	with    void PlotLine(const string&in label_id, const float[]@ xs, const float[]@ ys, int flags = UI::Plot::LineFlags::None)
	with    void PlotBars(const string&in label_id, const float[]@ values, double bar_size = 0.67, double shift = 0.0, int flags = UI::Plot::BarsFlags::None)
	with    void PlotBars(const string&in label_id, const float[]@ xs, const float[]@ ys, double bar_size = 0.67, int flags = UI::Plot::BarsFlags::None)
	with    void PlotErrorBars(const string&in label_id, const float[]@ xs, const float[]@ ys, const float[]@ err, int flags = UI::Plot::ErrorBarsFlags::None)
	with    void PlotErrorBars(const string&in label_id, const float[]@ xs, const float[]@ ys, const float[]@ neg, const float[]@ pos, int flags = UI::Plot::ErrorBarsFlags::None)
	with    void PlotShaded(const string&in label_id, const float[]@ values, double yref = 0.0, double xscale = 1.0, double xstart = 0.0, int flags = UI::Plot::ShadedFlags::None)
	with    void PlotShaded(const string&in label_id, const float[]@ xs, const float[]@ ys, double yref = 0.0, int flags = UI::Plot::ShadedFlags::None)
	with    void PlotShaded(const string&in label_id, const float[]@ xs, const float[]@ ys1, const float[]@ ys2, int flags = UI::Plot::ShadedFlags::None)
	with    void PlotScatter(const string&in label_id, const float[]@ values, double xscale = 1.0, double xstart = 0.0, int flags = UI::Plot::ScatterFlags::None)
	with    void PlotScatter(const string&in label_id, const float[]@ xs, const float[]@ ys, int flags = UI::Plot::ScatterFlags::None)
	with    void PlotBarGroups(const string[]@ label_ids, const float[]@ values, double group_size = 0.67, double shift = 0.0, int flags = UI::Plot::BarGroupsFlags::None)
	with    void PlotStems(const string&in label_id, const float[]@ values, double ref = 0.0, double scale = 1.0, double start = 0.0, int flags = UI::Plot::StemsFlags::None)
	with    void PlotStems(const string&in label_id, const float[]@ xs, const float[]@ ys, double ref = 0.0, int flags = UI::Plot::StemsFlags::None)
	with    void PlotStairs(const string&in label_id, const float[]@ values, double xscale = 1.0, double xstart = 0.0, int flags = UI::Plot::StairsFlags::None)
	with    void PlotStairs(const string&in label_id, const float[]@ xs, const float[]@ ys, int flags = UI::Plot::StairsFlags::None)
	with    void PlotPieChart(const string[]@ label_ids, const float[]@ values, double x = 0.0, double y = 0.0, double radius = 1.0, const string&in label_fmt = "%.1f", double angle0 = 90.0, int flags = UI::Plot::PieChartFlags::None)
	with    void PlotHeatmap(const string&in label_id, const float[]@ values, int rows, int cols, double scale_min = 0.0, double scale_max = 0.0, const string&in label_fmt = "%.1f", const vec2&in bounds_min = vec2(), const vec2&in bounds_max = vec2(1, 1), int flags = UI::Plot::HeatmapFlags::None)
	with    double PlotHistogram(const string&in label_id, const float[]@ values, int bins = UI::Plot::Bin::Sturges, double bar_scale = 1.0, double min = 0.0, double max = 0.0, int flags = UI::Plot::HistogramFlags::None)
	with    double PlotHistogram2D(const string&in label_id, const float[]@ xs, const float[]@ ys, int x_bins = UI::Plot::Bin::Sturges, int y_bins = UI::Plot::Bin::Sturges, const vec2&in range_min = vec2(), const vec2&in range_max = vec2(), int flags = UI::Plot::HistogramFlags::None)
	with    void SetAxis(Axis axis)
	with    void SetAxes(Axis x_axis, Axis y_axis)
	with    vec2 PixelsToPlot(const vec2&in pix, Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)
	with    vec2 PixelsToPlot(float x, float y, Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)
	with    vec2 PlotToPixels(const vec2&in plt, Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)
	with    vec2 PlotToPixels(double x, double y, Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)
	with    vec2 GetPlotPos()
	with    vec2 GetPlotSize()
	with    vec2 GetPlotMousePos(Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)
	with    vec4 GetPlotLimits(Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)
	with    bool IsPlotHovered()
	with    bool IsAxisHovered(Axis axis)
	with    bool IsSubplotsHovered()
	with    bool IsPlotSelected()
	with    vec4 GetPlotSelection(Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)
	with    void CancelPlotSelection()
	with    void HideNextItem(bool hidden = true, Cond cond = UI::Plot::Cond::Once)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	PLOT_EXPORT("bool BeginPlot(const string&in title_id, const vec2&in size = vec2(-1, 0), int flags = UI::Plot::PlotFlags::None)", BeginPlot);
	PLOT_EXPORT("void EndPlot()", EndPlot);
	PLOT_EXPORT("bool BeginSubplots(const string&in title_id, int rows, int cols, const vec2&in size, int flags = UI::Plot::SubplotFlags::None)", BeginSubplots);
	PLOT_EXPORT("void EndSubplots()", EndSubplots);
	PLOT_EXPORT("void SetupAxis(Axis axis, const string&in label = \"\", int flags = UI::Plot::AxisFlags::None)", SetupAxis);
	PLOT_EXPORT("void SetupAxisTicks(Axis axis, const double[]@+ values, const string[]@+ labels = null, bool keep_default = false)", SetupAxisTicksValues);
	PLOT_EXPORT("void SetupAxisLimits(Axis axis, double v_min, double v_max, Cond cond = UI::Plot::Cond::Once)", SetupAxisLimits);
	PLOT_EXPORT("void SetupAxisFormat(Axis axis, const string&in fmt)", SetupAxisFormat);
	PLOT_EXPORT("void SetupAxisScale(Axis axis, Scale scale)", SetupAxisScale);
	PLOT_EXPORT("void SetupAxisLimitsConstraints(Axis axis, double v_min, double v_max)", SetupAxisLimitsConstraints);
	PLOT_EXPORT("void SetupAxisZoomConstraints(Axis axis, double z_min, double z_max)", SetupAxisZoomConstraints);
	PLOT_EXPORT("void SetupAxes(const string&in x_label, const string&in y_label, int x_flags = UI::Plot::AxisFlags::None, int y_flags = UI::Plot::AxisFlags::None)", SetupAxes);
	PLOT_EXPORT("void SetupAxesLimits(double x_min, double x_max, double y_min, double y_max, Cond cond = UI::Plot::Cond::Once)", SetupAxesLimits);
	PLOT_EXPORT("void SetupLegend(Location location, int flags = UI::Plot::LegendFlags::None)", SetupLegend);
	PLOT_EXPORT("void SetupMouseText(Location location, int flags = UI::Plot::MouseTextFlags::None)", SetupMouseText);
	PLOT_EXPORT("void PushColormap(Colormap cmap)", PushColormap);
	PLOT_EXPORT("void PopColormap(int count = 1)", PopColormap);
	PLOT_EXPORT("void SetNextLineStyle(const vec4&in col = vec4(0, 0, 0, -1), float weight = -1.0f)", SetNextLineStyle);
	PLOT_EXPORT("void SetNextFillStyle(const vec4&in col = vec4(0, 0, 0, -1), float alpha_mod = -1.0f)", SetNextFillStyle);
	PLOT_EXPORT("void SetNextMarkerStyle(Marker marker = UI::Plot::Marker::Auto, float size = -1.0f, const vec4&in fill = vec4(0, 0, 0, -1), float weight = -1.0f, const vec4&in outline = vec4(0, 0, 0, -1))", SetNextMarkerStyle);
	PLOT_EXPORT("void SetNextErrorBarStyle(const vec4&in col = vec4(0, 0, 0, -1), float size = -1.0f, float weight = -1.0f)", SetNextErrorBarStyle);
	PLOT_EXPORT("void ColormapScale(const string&in label, double scale_min, double scale_max, const vec2&in size = vec2(), const string&in format = \"%g\", int flags = UI::Plot::ColormapScaleFlags::None, Colormap cmap = UI::Plot::Colormap::Auto)", ColormapScale);
	PLOT_EXPORT("void PlotLine(const string&in label_id, const float[]@+ values, double x_scale = 1.0, double x_start = 0.0, int flags = UI::Plot::LineFlags::None)", PlotLineValues);
	PLOT_EXPORT("void PlotLine(const string&in label_id, const float[]@+ xs, const float[]@+ ys, int flags = UI::Plot::LineFlags::None)", PlotLineXY);
	PLOT_EXPORT("void PlotBars(const string&in label_id, const float[]@+ values, double bar_size = 0.67, double shift = 0.0, int flags = UI::Plot::BarsFlags::None)", PlotBarsValues);
	PLOT_EXPORT("void PlotBars(const string&in label_id, const float[]@+ xs, const float[]@+ ys, double bar_size = 0.67, int flags = UI::Plot::BarsFlags::None)", PlotBarsXY);
	PLOT_EXPORT("void PlotErrorBars(const string&in label_id, const float[]@+ xs, const float[]@+ ys, const float[]@+ err, int flags = UI::Plot::ErrorBarsFlags::None)", PlotErrorBars);
	PLOT_EXPORT("void PlotErrorBars(const string&in label_id, const float[]@+ xs, const float[]@+ ys, const float[]@+ neg, const float[]@+ pos, int flags = UI::Plot::ErrorBarsFlags::None)", PlotErrorBarsNegPos);
	PLOT_EXPORT("void PlotShaded(const string&in label_id, const float[]@+ values, double yref = 0.0, double xscale = 1.0, double xstart = 0.0, int flags = UI::Plot::ShadedFlags::None)", PlotShadedValues);
	PLOT_EXPORT("void PlotShaded(const string&in label_id, const float[]@+ xs, const float[]@+ ys, double yref = 0.0, int flags = UI::Plot::ShadedFlags::None)", PlotShadedXY);
	PLOT_EXPORT("void PlotShaded(const string&in label_id, const float[]@+ xs, const float[]@+ ys1, const float[]@+ ys2, int flags = UI::Plot::ShadedFlags::None)", PlotShadedBetween);
	PLOT_EXPORT("void PlotScatter(const string&in label_id, const float[]@+ values, double xscale = 1.0, double xstart = 0.0, int flags = UI::Plot::ScatterFlags::None)", PlotScatterValues);
	PLOT_EXPORT("void PlotScatter(const string&in label_id, const float[]@+ xs, const float[]@+ ys, int flags = UI::Plot::ScatterFlags::None)", PlotScatterXY);
	PLOT_EXPORT("void PlotBarGroups(const string[]@+ label_ids, const float[]@+ values, double group_size = 0.67, double shift = 0.0, int flags = UI::Plot::BarGroupsFlags::None)", PlotBarGroups);
	PLOT_EXPORT("void PlotStems(const string&in label_id, const float[]@+ values, double ref = 0.0, double scale = 1.0, double start = 0.0, int flags = UI::Plot::StemsFlags::None)", PlotStemsValues);
	PLOT_EXPORT("void PlotStems(const string&in label_id, const float[]@+ xs, const float[]@+ ys, double ref = 0.0, int flags = UI::Plot::StemsFlags::None)", PlotStemsXY);
	PLOT_EXPORT("void PlotStairs(const string&in label_id, const float[]@+ values, double xscale = 1.0, double xstart = 0.0, int flags = UI::Plot::StairsFlags::None)", PlotStairsValues);
	PLOT_EXPORT("void PlotStairs(const string&in label_id, const float[]@+ xs, const float[]@+ ys, int flags = UI::Plot::StairsFlags::None)", PlotStairsXY);
	PLOT_EXPORT("void PlotPieChart(const string[]@+ label_ids, const float[]@+ values, double x = 0.0, double y = 0.0, double radius = 1.0, const string&in label_fmt = \"%.1f\", double angle0 = 90.0, int flags = UI::Plot::PieChartFlags::None)", PlotPieChart);
	PLOT_EXPORT("void PlotHeatmap(const string&in label_id, const float[]@+ values, int rows, int cols, double scale_min = 0.0, double scale_max = 0.0, const string&in label_fmt = \"%.1f\", const vec2&in bounds_min = vec2(), const vec2&in bounds_max = vec2(1, 1), int flags = UI::Plot::HeatmapFlags::None)", PlotHeatmap);
	PLOT_EXPORT("double PlotHistogram(const string&in label_id, const float[]@+ values, int bins = UI::Plot::Bin::Sturges, double bar_scale = 1.0, double min = 0.0, double max = 0.0, int flags = UI::Plot::HistogramFlags::None)", PlotHistogram);
	PLOT_EXPORT("double PlotHistogram2D(const string&in label_id, const float[]@+ xs, const float[]@+ ys, int x_bins = UI::Plot::Bin::Sturges, int y_bins = UI::Plot::Bin::Sturges, const vec2&in range_min = vec2(), const vec2&in range_max = vec2(), int flags = UI::Plot::HistogramFlags::None)", PlotHistogram2D);
	PLOT_EXPORT("void SetAxis(Axis axis)", SetAxis);
	PLOT_EXPORT("void SetAxes(Axis x_axis, Axis y_axis)", SetAxes);
	PLOT_EXPORT("vec2 PixelsToPlot(const vec2&in pix, Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)", PixelsToPlotVec);
	PLOT_EXPORT("vec2 PixelsToPlot(float x, float y, Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)", PixelsToPlotXY);
	PLOT_EXPORT("vec2 PlotToPixels(const vec2&in plt, Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)", PlotToPixelsVec);
	PLOT_EXPORT("vec2 PlotToPixels(double x, double y, Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)", PlotToPixelsXY);
	PLOT_EXPORT("vec2 GetPlotPos()", GetPlotPos);
	PLOT_EXPORT("vec2 GetPlotSize()", GetPlotSize);
	PLOT_EXPORT("vec2 GetPlotMousePos(Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)", GetPlotMousePos);
	PLOT_EXPORT("vec4 GetPlotLimits(Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)", GetPlotLimits);
	PLOT_EXPORT("bool IsPlotHovered()", IsPlotHovered);
	PLOT_EXPORT("bool IsAxisHovered(Axis axis)", IsAxisHovered);
	PLOT_EXPORT("bool IsSubplotsHovered()", IsSubplotsHovered);
	PLOT_EXPORT("bool IsPlotSelected()", IsPlotSelected);
	PLOT_EXPORT("vec4 GetPlotSelection(Axis x_axis = UI::Plot::Axis::Auto, Axis y_axis = UI::Plot::Axis::Auto)", GetPlotSelection);
	PLOT_EXPORT("void CancelPlotSelection()", CancelPlotSelection);
	PLOT_EXPORT("void HideNextItem(bool hidden = true, Cond cond = UI::Plot::Cond::Once)", HideNextItem);

	/*
	!CompatMeta
	for     UI::Plot
	with    void SetupAxisTicks(Axis axis, double v_min, double v_max, const string[]@ labels = null, bool keep_default = false)
	being   function
	declare full
	since   v0.2.0
	!CompatMeta
	*/
	PLOT_EXPORT("void SetupAxisTicks(Axis axis, double v_min, double v_max, const string[]@+ labels = null, bool keep_default = false)", SetupAxisTicksRange);

	void RegisterPlot(asIScriptEngine* Engine)
	{
		Engine->SetDefaultNamespace("UI::Plot");

		/*
		!CompatMeta
		for     UI::Plot
		with    PlotFlags
		with    SubplotFlags
		with    Axis
		with    AxisFlags
		with    Location
		with    LegendFlags
		with    MouseTextFlags
		with    Scale
		with    Marker
		with    Bin
		with    Cond
		with    Colormap
		with    ColormapScaleFlags
		with    LineFlags
		with    BarsFlags
		with    ErrorBarsFlags
		with    ShadedFlags
		with    ScatterFlags
		with    BarGroupsFlags
		with    StemsFlags
		with    StairsFlags
		with    PieChartFlags
		with    HeatmapFlags
		with    HistogramFlags
		being   enum
		declare full
		since   v0.2.0
		!CompatMeta
		*/
		RegisterPlotEnum(Engine, "PlotFlags", {
			{ "None", ImPlotFlags_None },
			{ "NoTitle", ImPlotFlags_NoTitle },
			{ "NoLegend", ImPlotFlags_NoLegend },
			{ "NoMouseText", ImPlotFlags_NoMouseText },
			{ "NoInputs", ImPlotFlags_NoInputs },
			{ "NoMenus", ImPlotFlags_NoMenus },
			{ "NoBoxSelect", ImPlotFlags_NoBoxSelect },
			{ "NoFrame", ImPlotFlags_NoFrame },
			{ "Equal", ImPlotFlags_Equal },
			{ "Crosshairs", ImPlotFlags_Crosshairs },
			{ "CanvasOnly", ImPlotFlags_CanvasOnly },
		});

		RegisterPlotEnum(Engine, "SubplotFlags", {
			{ "None", ImPlotSubplotFlags_None },
			{ "NoTitle", ImPlotSubplotFlags_NoTitle },
			{ "NoLegend", ImPlotSubplotFlags_NoLegend },
			{ "NoMenus", ImPlotSubplotFlags_NoMenus },
			{ "NoResize", ImPlotSubplotFlags_NoResize },
			{ "NoAlign", ImPlotSubplotFlags_NoAlign },
			{ "ShareItems", ImPlotSubplotFlags_ShareItems },
			{ "LinkRows", ImPlotSubplotFlags_LinkRows },
			{ "LinkCols", ImPlotSubplotFlags_LinkCols },
			{ "LinkAllX", ImPlotSubplotFlags_LinkAllX },
			{ "LinkAllY", ImPlotSubplotFlags_LinkAllY },
			{ "ColMajor", ImPlotSubplotFlags_ColMajor },
		});

		RegisterPlotEnum(Engine, "Axis", {
			{ "Auto", IMPLOT_AUTO },
			{ "X1", ImAxis_X1 },
			{ "X2", ImAxis_X2 },
			{ "X3", ImAxis_X3 },
			{ "Y1", ImAxis_Y1 },
			{ "Y2", ImAxis_Y2 },
			{ "Y3", ImAxis_Y3 },
		});

		RegisterPlotEnum(Engine, "AxisFlags", {
			{ "None", ImPlotAxisFlags_None },
			{ "NoLabel", ImPlotAxisFlags_NoLabel },
			{ "NoGridLines", ImPlotAxisFlags_NoGridLines },
			{ "NoTickMarks", ImPlotAxisFlags_NoTickMarks },
			{ "NoTickLabels", ImPlotAxisFlags_NoTickLabels },
			{ "NoInitialFit", ImPlotAxisFlags_NoInitialFit },
			{ "NoMenus", ImPlotAxisFlags_NoMenus },
			{ "NoSideSwitch", ImPlotAxisFlags_NoSideSwitch },
			{ "NoHighlight", ImPlotAxisFlags_NoHighlight },
			{ "Opposite", ImPlotAxisFlags_Opposite },
			{ "Foreground", ImPlotAxisFlags_Foreground },
			{ "Invert", ImPlotAxisFlags_Invert },
			{ "AutoFit", ImPlotAxisFlags_AutoFit },
			{ "RangeFit", ImPlotAxisFlags_RangeFit },
			{ "PanStretch", ImPlotAxisFlags_PanStretch },
			{ "LockMin", ImPlotAxisFlags_LockMin },
			{ "LockMax", ImPlotAxisFlags_LockMax },
			{ "Lock", ImPlotAxisFlags_Lock },
			{ "NoDecorations", ImPlotAxisFlags_NoDecorations },
			{ "AuxDefault", ImPlotAxisFlags_AuxDefault },
		});

		RegisterPlotEnum(Engine, "Location", {
			{ "Center", ImPlotLocation_Center },
			{ "North", ImPlotLocation_North },
			{ "South", ImPlotLocation_South },
			{ "West", ImPlotLocation_West },
			{ "East", ImPlotLocation_East },
			{ "NorthWest", ImPlotLocation_NorthWest },
			{ "NorthEast", ImPlotLocation_NorthEast },
			{ "SouthWest", ImPlotLocation_SouthWest },
			{ "SouthEast", ImPlotLocation_SouthEast },
		});

		RegisterPlotEnum(Engine, "LegendFlags", {
			{ "None", ImPlotLegendFlags_None },
			{ "NoButtons", ImPlotLegendFlags_NoButtons },
			{ "NoHighlightItem", ImPlotLegendFlags_NoHighlightItem },
			{ "NoHighlightAxis", ImPlotLegendFlags_NoHighlightAxis },
			{ "NoMenus", ImPlotLegendFlags_NoMenus },
			{ "Outside", ImPlotLegendFlags_Outside },
			{ "Horizontal", ImPlotLegendFlags_Horizontal },
			{ "Sort", ImPlotLegendFlags_Sort },
		});

		RegisterPlotEnum(Engine, "MouseTextFlags", {
			{ "None", ImPlotMouseTextFlags_None },
			{ "NoAuxAxes", ImPlotMouseTextFlags_NoAuxAxes },
			{ "NoFormat", ImPlotMouseTextFlags_NoFormat },
			{ "ShowAlways", ImPlotMouseTextFlags_ShowAlways },
		});

		RegisterPlotEnum(Engine, "Scale", {
			{ "Linear", ImPlotScale_Linear },
			{ "Time", ImPlotScale_Time },
			{ "Log10", ImPlotScale_Log10 },
			{ "SymLog", ImPlotScale_SymLog },
		});

		RegisterPlotEnum(Engine, "Marker", {
			{ "Auto", ImPlotMarker_Auto },
			{ "None", ImPlotMarker_None },
			{ "Circle", ImPlotMarker_Circle },
			{ "Square", ImPlotMarker_Square },
			{ "Diamond", ImPlotMarker_Diamond },
			{ "Up", ImPlotMarker_Up },
			{ "Down", ImPlotMarker_Down },
			{ "Left", ImPlotMarker_Left },
			{ "Right", ImPlotMarker_Right },
			{ "Cross", ImPlotMarker_Cross },
			{ "Plus", ImPlotMarker_Plus },
			{ "Asterisk", ImPlotMarker_Asterisk },
		});

		RegisterPlotEnum(Engine, "Bin", {
			{ "Sqrt", ImPlotBin_Sqrt },
			{ "Sturges", ImPlotBin_Sturges },
			{ "Rice", ImPlotBin_Rice },
			{ "Scott", ImPlotBin_Scott },
		});

		RegisterPlotEnum(Engine, "Cond", {
			{ "None", ImPlotCond_None },
			{ "Always", ImPlotCond_Always },
			{ "Once", ImPlotCond_Once },
		});

		RegisterPlotEnum(Engine, "Colormap", {
			{ "Auto", IMPLOT_AUTO },
			{ "Deep", ImPlotColormap_Deep },
			{ "Dark", ImPlotColormap_Dark },
			{ "Pastel", ImPlotColormap_Pastel },
			{ "Paired", ImPlotColormap_Paired },
			{ "Viridis", ImPlotColormap_Viridis },
			{ "Plasma", ImPlotColormap_Plasma },
			{ "Hot", ImPlotColormap_Hot },
			{ "Cool", ImPlotColormap_Cool },
			{ "Pink", ImPlotColormap_Pink },
			{ "Jet", ImPlotColormap_Jet },
			{ "Twilight", ImPlotColormap_Twilight },
			{ "RdBu", ImPlotColormap_RdBu },
			{ "BrBG", ImPlotColormap_BrBG },
			{ "PiYG", ImPlotColormap_PiYG },
			{ "Spectral", ImPlotColormap_Spectral },
			{ "Greys", ImPlotColormap_Greys },
		});

		RegisterPlotEnum(Engine, "ColormapScaleFlags", {
			{ "None", ImPlotColormapScaleFlags_None },
			{ "NoLabel", ImPlotColormapScaleFlags_NoLabel },
			{ "Opposite", ImPlotColormapScaleFlags_Opposite },
			{ "Invert", ImPlotColormapScaleFlags_Invert },
		});

		RegisterPlotEnum(Engine, "LineFlags", {
			{ "None", ImPlotLineFlags_None },
			{ "Segments", ImPlotLineFlags_Segments },
			{ "Loop", ImPlotLineFlags_Loop },
			{ "SkipNaN", ImPlotLineFlags_SkipNaN },
			{ "NoClip", ImPlotLineFlags_NoClip },
			{ "Shaded", ImPlotLineFlags_Shaded },
		});

		RegisterPlotEnum(Engine, "BarsFlags", {
			{ "None", ImPlotBarsFlags_None },
			{ "Horizontal", ImPlotBarsFlags_Horizontal },
		});

		RegisterPlotEnum(Engine, "ErrorBarsFlags", {
			{ "None", ImPlotErrorBarsFlags_None },
			{ "Horizontal", ImPlotErrorBarsFlags_Horizontal },
		});

		RegisterPlotEnum(Engine, "ShadedFlags", {
			{ "None", ImPlotShadedFlags_None },
		});

		RegisterPlotEnum(Engine, "ScatterFlags", {
			{ "None", ImPlotScatterFlags_None },
			{ "NoClip", ImPlotScatterFlags_NoClip },
		});

		RegisterPlotEnum(Engine, "BarGroupsFlags", {
			{ "None", ImPlotBarGroupsFlags_None },
			{ "Horizontal", ImPlotBarGroupsFlags_Horizontal },
			{ "Stacked", ImPlotBarGroupsFlags_Stacked },
		});

		RegisterPlotEnum(Engine, "StemsFlags", {
			{ "None", ImPlotStemsFlags_None },
			{ "Horizontal", ImPlotStemsFlags_Horizontal },
		});

		RegisterPlotEnum(Engine, "StairsFlags", {
			{ "None", ImPlotStairsFlags_None },
			{ "PreStep", ImPlotStairsFlags_PreStep },
			{ "Shaded", ImPlotStairsFlags_Shaded },
		});

		RegisterPlotEnum(Engine, "PieChartFlags", {
			{ "None", ImPlotPieChartFlags_None },
			{ "Normalize", ImPlotPieChartFlags_Normalize },
			{ "IgnoreHidden", ImPlotPieChartFlags_IgnoreHidden },
		});

		RegisterPlotEnum(Engine, "HeatmapFlags", {
			{ "None", ImPlotHeatmapFlags_None },
			{ "ColMajor", ImPlotHeatmapFlags_ColMajor },
		});

		RegisterPlotEnum(Engine, "HistogramFlags", {
			{ "None", ImPlotHistogramFlags_None },
			{ "Horizontal", ImPlotHistogramFlags_Horizontal },
			{ "Cumulative", ImPlotHistogramFlags_Cumulative },
			{ "Density", ImPlotHistogramFlags_Density },
			{ "NoOutliers", ImPlotHistogramFlags_NoOutliers },
			{ "ColMajor", ImPlotHistogramFlags_ColMajor },
		});

		Engine->SetDefaultNamespace("");

		RegisterAll(Engine, s_PlotExports);
	}
}
