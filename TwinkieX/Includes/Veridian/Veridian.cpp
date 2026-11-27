#include "pch.h"
#include "Veridian.h"
#include "Twinkie/TwinkUi/TwinkUi.h"

namespace Veridian
{
    VSetCtx* VastVeridian = nullptr;

    std::unordered_map<VSettingType, size_t> TypeToSize = {
        {VSettingType::VInt, sizeof(int64_t)},
        {VSettingType::VUInt, sizeof(uint64_t)},
        {VSettingType::VFloat, sizeof(float)},
        {VSettingType::VString, sizeof(std::string)},
        {VSettingType::VVec2, sizeof(VVec2)},
        {VSettingType::VVec3, sizeof(VVec3)},
        {VSettingType::VVec4, sizeof(VVec4)},
        {VSettingType::VBool, sizeof(bool)},
        {VSettingType::VUnknown, (size_t)-1LL}
    };

    VSetCtx::VSetCtx(std::filesystem::path Filepath)
    {
        this->Filepath = Filepath;

        if (not std::filesystem::exists(Filepath))
        {
            auto Filename = Filepath.filename();

            Filepath.remove_filename();
            std::filesystem::create_directories(Filepath);

            Filepath /= Filename;

            std::fstream(Filepath.wstring(), std::ios::in | std::ios::out | std::ios::trunc).close();
        }

        std::ifstream FileStream(Filepath.wstring());

        if (not FileStream.is_open())
        {
            throw;
        }

        std::string Line;
        std::string CurrentSector = VERIDIAN_FALLBACK_SECTION_NAME;

        while (std::getline(FileStream, Line))
        {
            if (Line.starts_with("["))
            {
                CurrentSector = Line.substr(1, Line.length() - 2);
                continue;
            }
            else if (Line.find("=") != std::string::npos)
            {
                size_t EqualsSign = Line.find("=");

                std::string Name = Line.substr(0, EqualsSign);
                std::string Result = Line.substr(EqualsSign + 1);

                SettingValues[CurrentSector].Values[Name] = VSetting();

                VSetting& NewSetting = SettingValues[CurrentSector].Values[Name];

                NewSetting.Name = Name;
                NewSetting.SettingStr = Result;
            }
        }
    }

    void VSetCtx::Save()
    {
        std::string Contents;

        auto WriteSection = [this, &Contents](const std::string& SectionName)
        {
            auto& Section = SettingValues[SectionName];

            std::string Lines;

            for (auto& SettingName : Section.Order)
            {
                VSetting& Setting = Section.Values[SettingName];
                if (Setting.IsCustomDefault()) continue;

                Lines += std::format("{}={}\n", SettingName, Setting.GetAsString());
            }

            // Keep settings that were loaded but not registered this session
            for (auto& [SettingName, Setting] : Section.Values)
            {
                if (std::find(Section.Order.begin(), Section.Order.end(), SettingName) != Section.Order.end()) continue;
                if (Setting.IsCustomDefault()) continue;

                Lines += std::format("{}={}\n", SettingName, Setting.GetAsString());
            }

            if (Lines.empty()) return;

            Contents += std::format("[{}]\n", SectionName);
            Contents += Lines;
            Contents += "\n";
        };

        for (auto& SectionName : SettingOrder)
        {
            WriteSection(SectionName);
        }
        for (auto& [SectionName, Section] : SettingValues)
        {
            if (std::find(SettingOrder.begin(), SettingOrder.end(), SectionName) != SettingOrder.end()) continue;

            WriteSection(SectionName);
        }

        if (Contents == LastSaved) return;

        // Write to a temporary file first, so that being killed mid-write doesn't corrupt the settings
        auto TempFilepath = Filepath;
        TempFilepath += ".tmp";

        std::ofstream FileStream(TempFilepath.wstring(), std::ios::out | std::ios::trunc);

        if (not FileStream.is_open()) return;

        FileStream << Contents;
        FileStream.close();

        if (FileStream.fail()) return;

        std::error_code Error;
        std::filesystem::rename(TempFilepath, Filepath, Error);

        if (not Error) LastSaved = Contents;
    }

    VSetting& VSetCtx::Register(std::string Section, std::string Name, std::string FacingName, VSettingType Type, void* Value, bool Hidden, VSettingRenderFnStruct ExtraRenderFn, VCustomFns Custom)
    {
        if (this->SettingValues.contains(Section))
        {
            if (this->SettingValues[Section].Values.contains(Name))
            {
                VSetting& ExistingSetting = this->SettingValues[Section].Values[Name];

                if (Type == VSettingType::VCustom)
                {
                    if (Custom.FromString) Custom.FromString(ExistingSetting.SettingStr);
                }
                else
                {
                    ExistingSetting.Type = Type;
                    ExistingSetting.SetFromString();

                    if (Value and ExistingSetting.Value)
                    {
                        if (Type == VSettingType::VString)
                            *reinterpret_cast<std::string*>(Value) = ExistingSetting.SettingStr;
                        else
                            memcpy_s(Value, TypeToSize[Type], ExistingSetting.Value, TypeToSize[Type]);
                    }

                    if (Type != VSettingType::VString) delete ExistingSetting.Value;
                    ExistingSetting.Value = nullptr;
                }
            }
        }

        if (std::find(SettingOrder.begin(), SettingOrder.end(), Section) == SettingOrder.end()) SettingOrder.push_back(Section);
        if (std::find(SettingValues[Section].Order.begin(), SettingValues[Section].Order.end(), Name) == SettingValues[Section].Order.end())
            SettingValues[Section].Order.push_back(Name);

        VSetting& NewSettingRef = this->SettingValues[Section].Values[Name];

        NewSettingRef.Name = Name;
        NewSettingRef.FacingName = FacingName;
        NewSettingRef.Section = Section;
        NewSettingRef.Type = Type;
        NewSettingRef.Registered = true;
        NewSettingRef.Hidden = Hidden;
        NewSettingRef.ExtraRenderFn = ExtraRenderFn;
        NewSettingRef.Custom = Custom;
        NewSettingRef.Value = reinterpret_cast<VValue*>(Value);

        return this->SettingValues[Section].Values[Name];
    }

    void VSetCtx::ReleaseSetting(Inner& Section, std::string Name)
    {
        auto Found = Section.Values.find(Name);
        if (Found == Section.Values.end() or not Found->second.Registered) return;

        VSetting& Setting = Found->second;
        if (Setting.IsCustomDefault())
        {
            Section.Values.erase(Found);
            std::erase(Section.Order, Name);
            return;
        }

        if (Setting.Value) Setting.SettingStr = Setting.GetAsString();
        Setting.Value = nullptr;
        Setting.Registered = false;
        Setting.ExtraRenderFn = {};
        Setting.Custom = {};
    }

    void VSetCtx::Unregister(const std::string& Section, const std::string& Name)
    {
        auto Found = SettingValues.find(Section);
        if (Found != SettingValues.end()) ReleaseSetting(Found->second, Name);
    }

    void VSetCtx::UnregisterSection(const std::string& Section)
    {
        auto Found = SettingValues.find(Section);
        if (Found == SettingValues.end()) return;

        std::vector<std::string> Names;
        for (auto& [SettingName, Setting] : Found->second.Values)
        {
            if (Setting.Registered) Names.push_back(SettingName);
        }
        for (auto& SettingName : Names)
        {
            ReleaseSetting(Found->second, SettingName);
        }

        Found->second.FacingName.clear();
        Found->second.Render = nullptr;
        Found->second.Visible = nullptr;
    }

    void VSetCtx::RegisterSection(const std::string& Section, std::string FacingName, std::function<void()> Render, std::function<bool()> Visible)
    {
        if (std::find(SettingOrder.begin(), SettingOrder.end(), Section) == SettingOrder.end()) SettingOrder.push_back(Section);

        Inner& Target = SettingValues[Section];
        Target.FacingName = std::move(FacingName);
        Target.Render = std::move(Render);
        Target.Visible = std::move(Visible);
    }

    bool VSetCtx::Read(const std::string& Section, const std::string& Name, std::string& Out)
    {
        auto FoundSection = SettingValues.find(Section);
        if (FoundSection == SettingValues.end()) return false;

        auto Found = FoundSection->second.Values.find(Name);
        if (Found == FoundSection->second.Values.end() or Found->second.IsCustomDefault()) return false;

        Out = Found->second.GetAsString();
        return true;
    }

    void VSetCtx::Write(const std::string& Section, const std::string& Name, const std::string& Value)
    {
        Inner& Target = SettingValues[Section];
        VSetting& Setting = Target.Values[Name];

        if (Setting.Registered)
        {
            if (Setting.Type == VSettingType::VCustom and Setting.Custom.FromString) Setting.Custom.FromString(Value);
            return;
        }

        Setting.Name = Name;
        Setting.SettingStr = Value;
        if (std::find(Target.Order.begin(), Target.Order.end(), Name) == Target.Order.end()) Target.Order.push_back(Name);
    }

    void VSetCtx::Remove(const std::string& Section, const std::string& Name)
    {
        auto FoundSection = SettingValues.find(Section);
        if (FoundSection == SettingValues.end()) return;

        auto Found = FoundSection->second.Values.find(Name);
        if (Found == FoundSection->second.Values.end() or Found->second.Registered) return;

        FoundSection->second.Values.erase(Found);
        std::erase(FoundSection->second.Order, Name);
    }

    void VSetCtx::ClearUnregistered(const std::string& Section)
    {
        for (auto& Name : Keys(Section))
        {
            Remove(Section, Name);
        }
    }

    std::vector<std::string> VSetCtx::Keys(const std::string& Section)
    {
        std::vector<std::string> Result;

        auto FoundSection = SettingValues.find(Section);
        if (FoundSection == SettingValues.end()) return Result;

        for (auto& Name : FoundSection->second.Order)
        {
            auto Found = FoundSection->second.Values.find(Name);
            if (Found != FoundSection->second.Values.end() and not Found->second.IsCustomDefault()) Result.push_back(Name);
        }
        for (auto& [Name, Setting] : FoundSection->second.Values)
        {
            if (Setting.IsCustomDefault()) continue;
            if (std::find(Result.begin(), Result.end(), Name) == Result.end()) Result.push_back(Name);
        }

        return Result;
    }

    void VSetCtx::PrepareForDestruction()
    {
        for (auto& SectionName : this->SettingOrder)
        {
            auto& Section = this->SettingValues[SectionName];
            for (auto& SettingName : Section.Order)
            {
                auto& Setting = Section.Values[SettingName];

                if (Setting.Value)
                {
                    Setting.SettingStr = Setting.GetAsString();
                    Setting.Value = nullptr;
                }
            }
        }
    }

    void InitContext(std::filesystem::path Filepath)
    {
        VastVeridian = new VSetCtx(Filepath);
    }

    void RenderSetting(VSetting& Setting)
    {
        if (Setting.Hidden) return;

        if (Setting.ExtraRenderFn.BeforeRender)
        {
            Setting.ExtraRenderFn.BeforeRender(&Setting);
        }

        static std::map<VSettingType, ImGuiDataType> VeridianToImGui =
        {
            {VSettingType::VInt, ImGuiDataType_S64},
            {VSettingType::VUInt, ImGuiDataType_U64},
            {VSettingType::VFloat, ImGuiDataType_Float}
        };

        char TempBuffer[513] = {};

        if (Setting.Value != nullptr and not Setting.ExtraRenderFn.Render)
        {
            if (Setting.Type == VSettingType::VString)
            {
                strcpy_s(TempBuffer, 513, Setting.Value->String.c_str());
            }

            switch (Setting.Type)
            {
            case VSettingType::VUInt:
            case VSettingType::VInt:
            case VSettingType::VFloat:
                ImGui::InputScalar(Setting.FacingName.c_str(), VeridianToImGui[Setting.Type], Setting.Value);
                break;
            case VSettingType::VBool:
                ImGui::Checkbox(Setting.FacingName.c_str(), &Setting.Value->Bool);
                break;
            case VSettingType::VVec2:
                ImGui::InputFloat2(Setting.FacingName.c_str(), &Setting.Value->Float);
                break;
            case VSettingType::VVec3:
                ImGui::ColorEdit3(Setting.FacingName.c_str(), &Setting.Value->Float);
                break;
            case VSettingType::VVec4:
                ImGui::ColorEdit4(Setting.FacingName.c_str(), &Setting.Value->Float);
                break;
            case VSettingType::VString:
                ImGui::InputText(Setting.FacingName.c_str(), TempBuffer, 513);
                break;
            default:
                ImGui::Text("Error rending setting %s (in %s): unknown type", Setting.Name.c_str(), Setting.Section.c_str());
                break;
            }
        }
        else if (Setting.Value == nullptr)
        {
            static int64_t Int = 0;
            static uint64_t UInt = 0;
            static float Float = 0;
            static bool Bool = false;
            static VVec2 Vec2 = VVec2();
            static VVec3 Vec3 = VVec3();
            static VVec4 Vec4 = VVec4();
            static std::string String;

            strcpy_s(TempBuffer, 513, String.c_str());

            ImGui::BeginDisabled();

            switch (Setting.Type)
            {
            case VSettingType::VUInt:
            case VSettingType::VInt:
            case VSettingType::VFloat:
                ImGui::InputScalar(Setting.FacingName.c_str(), VeridianToImGui[Setting.Type], &Float);
                break;
            case VSettingType::VBool:
                ImGui::Checkbox(Setting.FacingName.c_str(), &Bool);
                break;
            case VSettingType::VVec2:
                ImGui::InputFloat2(Setting.FacingName.c_str(), &Vec2.x);
                break;
            case VSettingType::VVec3:
                ImGui::ColorEdit3(Setting.FacingName.c_str(), &Vec3.x);
                break;
            case VSettingType::VVec4:
                ImGui::ColorEdit4(Setting.FacingName.c_str(), &Vec4.x);
                break;
            case VSettingType::VString:
                ImGui::InputText(Setting.FacingName.c_str(), TempBuffer, 513);
                break;
            default:
                ImGui::Text("Error rending setting %s (in %s): unknown type", Setting.Name.c_str(), Setting.Section.c_str());
                break;
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("Item has been disabled because it has not been registered properly.");
            }

            ImGui::EndDisabled();
        }
        else if (Setting.ExtraRenderFn.Render)
        {
            Setting.ExtraRenderFn.Render(&Setting);
        }

        if (Setting.ExtraRenderFn.AfterRender)
        {
            Setting.ExtraRenderFn.AfterRender(&Setting);
        }
    }

    void RenderSetting(std::string Section, std::string Name)
    {
        if (not VastVeridian->SettingValues.contains(Section)) return;
        if (not VastVeridian->SettingValues[Section].Values.contains(Name)) return;

        RenderSetting(VastVeridian->SettingValues[Section].Values[Name]);
    }

    void RenderSection(std::string Section)
    {
        if (not VastVeridian->SettingValues.contains(Section)) return;

        if (VastVeridian->SettingValues.at(Section).Render)
        {
            VastVeridian->SettingValues.at(Section).Render();
            return;
        }

        for (auto& SetPair : VastVeridian->SettingValues.at(Section).Order)
        {
            if (VastVeridian->SettingValues[Section].Values[SetPair].Registered) RenderSetting(VastVeridian->SettingValues[Section].Values[SetPair]);
        }
    }

    static std::string ActiveSection;

    void SetActiveSection(std::string Section)
    {
        ActiveSection = std::move(Section);
    }

    static bool IsSectionVisible(const std::string& SectionName, size_t RegisteredSettings)
    {
        auto& Section = VastVeridian->SettingValues[SectionName];
        if (Section.Render) return not Section.Visible or Section.Visible();

        return RegisteredSettings != 0;
    }

    void RenderAll(std::string DefaultSectionName)
    {
        if (ActiveSection.empty()) ActiveSection = DefaultSectionName;

        std::map<std::string, size_t> RegisteredSettingsPerSection = {};

        for (auto& Section : VastVeridian->SettingOrder)
        {
            for (auto& Setting : VastVeridian->SettingValues[Section].Order)
            {
                if (VastVeridian->SettingValues[Section].Values[Setting].Registered and not VastVeridian->SettingValues[Section].Values[Setting].Hidden)
                {
                    RegisteredSettingsPerSection[Section]++;
                }
            }
        }

        if (ActiveSection != DefaultSectionName and not IsSectionVisible(ActiveSection, RegisteredSettingsPerSection[ActiveSection]))
        {
            ActiveSection = DefaultSectionName;
        }

        if (ImGui::BeginChild("##SettingsSidebar", { 150.f, 0.f }, ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX))
        {
            if (ImGui::Selectable(DefaultSectionName.c_str(), DefaultSectionName == ActiveSection))
            {
                ActiveSection = DefaultSectionName;
            }

            for (auto& Section : VastVeridian->SettingOrder)
            {
                if (Section == DefaultSectionName) continue;

                if (not IsSectionVisible(Section, RegisteredSettingsPerSection[Section])) continue;

                const std::string& FacingName = VastVeridian->SettingValues[Section].FacingName;
                std::string Label = (FacingName.empty() ? Section : FacingName) + "##" + Section;

                if (ImGui::Selectable(Label.c_str(), Section == ActiveSection))
                {
                    ActiveSection = Section;
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginGroup();

        RenderSection(ActiveSection);

        ImGui::EndGroup();
    }

    VSetting& Register(std::string Section, std::string Name, std::string FacingName, VSettingType Type, void* Value, bool Hidden, VSettingRenderFnStruct ExtraRenderFn, VCustomFns Custom)
    {
        return VastVeridian->Register(Section, Name, FacingName, Type, Value, Hidden, ExtraRenderFn, Custom);
    }

    void Unregister(const std::string& Section, const std::string& Name)
    {
        if (VastVeridian) VastVeridian->Unregister(Section, Name);
    }

    void UnregisterSection(const std::string& Section)
    {
        if (VastVeridian) VastVeridian->UnregisterSection(Section);
    }

    void RegisterSection(const std::string& Section, std::string FacingName, std::function<void()> Render, std::function<bool()> Visible)
    {
        if (VastVeridian) VastVeridian->RegisterSection(Section, std::move(FacingName), std::move(Render), std::move(Visible));
    }
}
