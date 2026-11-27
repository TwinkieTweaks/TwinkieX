#include "pch.h"
#include "PluginSettings.h"
#include "ScriptManager.h"
#include <Gecko/Exports/Math/Math.h>

#include <algorithm>
#include <charconv>
#include <limits>

namespace Gecko
{
	static std::u8string ToU8(std::string_view Str)
	{
		return std::u8string((const char8_t*)Str.data(), Str.size());
	}

	static std::string_view Trim(std::string_view Text)
	{
		size_t Start = Text.find_first_not_of(" \t\r\n");
		if (Start == std::string_view::npos) return {};
		size_t End = Text.find_last_not_of(" \t\r\n");
		return Text.substr(Start, End - Start + 1);
	}

	static bool EqualsNoCase(std::string_view Left, std::string_view Right)
	{
		return Left.size() == Right.size() && std::equal(Left.begin(), Left.end(), Right.begin(), [](char LeftChar, char RightChar)
		{
			return std::tolower((uint8_t)LeftChar) == std::tolower((uint8_t)RightChar);
		});
	}

	template <typename T>
	static bool ParseNumber(std::string_view Text, T& Out)
	{
		Text = Trim(Text);
		if (!Text.empty() && Text.front() == '+') Text.remove_prefix(1);

		T Value{};
		auto [End, Error] = std::from_chars(Text.data(), Text.data() + Text.size(), Value);
		if (Text.empty() || Error != std::errc() || End != Text.data() + Text.size()) return false;
		Out = Value;
		return true;
	}

	bool ParseSettingBool(std::string_view Text, bool& Out)
	{
		Text = Trim(Text);
		if (EqualsNoCase(Text, "true") || Text == "1" || EqualsNoCase(Text, "yes"))
		{
			Out = true;
			return true;
		}
		if (EqualsNoCase(Text, "false") || Text == "0" || EqualsNoCase(Text, "no"))
		{
			Out = false;
			return true;
		}
		return false;
	}

	std::string SettingEnumValueName(asITypeInfo* EnumType, int32_t Value)
	{
		for (asUINT Idx = 0; EnumType && Idx < EnumType->GetEnumValueCount(); Idx++)
		{
			int Option = 0;
			const char* OptionName = EnumType->GetEnumValueByIndex(Idx, &Option);
			if (OptionName && Option == Value) return OptionName;
		}
		return std::to_string(Value);
	}

	bool SettingsSection::Get(const std::string& Key, std::string& Out) const
	{
		return Veridian::VastVeridian && Veridian::VastVeridian->Read(Name, Key, Out);
	}

	void SettingsSection::Set(const std::string& Key, const std::string& Value) const
	{
		if (Veridian::VastVeridian) Veridian::VastVeridian->Write(Name, Key, Value);
	}

	void SettingsSection::Remove(const std::string& Key) const
	{
		if (Veridian::VastVeridian) Veridian::VastVeridian->Remove(Name, Key);
	}

	void SettingsSection::Clear() const
	{
		if (Veridian::VastVeridian) Veridian::VastVeridian->ClearUnregistered(Name);
	}

	std::vector<std::string> SettingsSection::Keys() const
	{
		return Veridian::VastVeridian ? Veridian::VastVeridian->Keys(Name) : std::vector<std::string>();
	}

	std::string EscapeSettingString(std::string_view Text)
	{
		std::string Out;
		Out.reserve(Text.size());
		for (char Char : Text)
		{
			if (Char == '\\') Out += "\\\\";
			else if (Char == '\n') Out += "\\n";
			else if (Char == '\r') Out += "\\r";
			else if (Char == '=') Out += "\\=";
			else Out += Char;
		}
		return Out;
	}

	std::string UnescapeSettingString(std::string_view Text)
	{
		std::string Out;
		Out.reserve(Text.size());
		for (size_t Idx = 0; Idx < Text.size(); Idx++)
		{
			char Char = Text[Idx];
			if (Char == '\\' && Idx + 1 < Text.size())
			{
				char Next = Text[Idx + 1];
				if (Next == '\\' || Next == 'n' || Next == 'r' || Next == '=')
				{
					Out += Next == 'n' ? '\n' : Next == 'r' ? '\r' : Next;
					Idx++;
					continue;
				}
			}
			Out += Char;
		}
		return Out;
	}

	static size_t ComponentCount(SettingType Type)
	{
		switch (Type)
		{
		case SettingType::Vec2:
		case SettingType::Int2:
		case SettingType::Nat2:
			return 2;
		case SettingType::Vec3:
		case SettingType::Int3:
		case SettingType::Nat3:
			return 3;
		case SettingType::Vec4:
		case SettingType::Quat:
			return 4;
		default:
			return 1;
		}
	}

	static ImGuiDataType ComponentDataType(SettingType Type)
	{
		switch (Type)
		{
		case SettingType::Int8: return ImGuiDataType_S8;
		case SettingType::Uint8: return ImGuiDataType_U8;
		case SettingType::Int16: return ImGuiDataType_S16;
		case SettingType::Uint16: return ImGuiDataType_U16;
		case SettingType::Enum:
		case SettingType::Int32:
		case SettingType::Int2:
		case SettingType::Int3:
			return ImGuiDataType_S32;
		case SettingType::Uint32:
		case SettingType::Nat2:
		case SettingType::Nat3:
			return ImGuiDataType_U32;
		case SettingType::Double: return ImGuiDataType_Double;
		default: return ImGuiDataType_Float;
		}
	}

	static size_t ComponentSize(ImGuiDataType DataType)
	{
		switch (DataType)
		{
		case ImGuiDataType_S8:
		case ImGuiDataType_U8:
			return 1;
		case ImGuiDataType_S16:
		case ImGuiDataType_U16:
			return 2;
		case ImGuiDataType_Double:
			return 8;
		default:
			return 4;
		}
	}

	static size_t ValueSize(SettingType Type)
	{
		if (Type == SettingType::Bool) return sizeof(bool);
		if (Type == SettingType::String || Type == SettingType::Unknown) return 0;
		return ComponentCount(Type) * ComponentSize(ComponentDataType(Type));
	}

	template <typename T>
	static T LoadAs(const void* Data)
	{
		T Value{};
		std::memcpy(&Value, Data, sizeof(T));
		return Value;
	}

	template <typename T>
	static void StoreAs(double Value, void* Data)
	{
		if (Value != Value) Value = 0.0;
		T Typed = (T)std::clamp(Value, (double)(std::numeric_limits<T>::lowest)(), (double)(std::numeric_limits<T>::max)());
		std::memcpy(Data, &Typed, sizeof(T));
	}

	static double LoadComponent(ImGuiDataType DataType, const void* Data)
	{
		switch (DataType)
		{
		case ImGuiDataType_S8: return (double)LoadAs<int8_t>(Data);
		case ImGuiDataType_U8: return (double)LoadAs<uint8_t>(Data);
		case ImGuiDataType_S16: return (double)LoadAs<int16_t>(Data);
		case ImGuiDataType_U16: return (double)LoadAs<uint16_t>(Data);
		case ImGuiDataType_S32: return (double)LoadAs<int32_t>(Data);
		case ImGuiDataType_U32: return (double)LoadAs<uint32_t>(Data);
		case ImGuiDataType_Float: return (double)LoadAs<float>(Data);
		case ImGuiDataType_Double: return LoadAs<double>(Data);
		default: return 0.0;
		}
	}

	static void StoreComponent(ImGuiDataType DataType, double Value, void* Data)
	{
		switch (DataType)
		{
		case ImGuiDataType_S8: StoreAs<int8_t>(Value, Data); break;
		case ImGuiDataType_U8: StoreAs<uint8_t>(Value, Data); break;
		case ImGuiDataType_S16: StoreAs<int16_t>(Value, Data); break;
		case ImGuiDataType_U16: StoreAs<uint16_t>(Value, Data); break;
		case ImGuiDataType_S32: StoreAs<int32_t>(Value, Data); break;
		case ImGuiDataType_U32: StoreAs<uint32_t>(Value, Data); break;
		case ImGuiDataType_Float: StoreAs<float>(Value, Data); break;
		case ImGuiDataType_Double: StoreAs<double>(Value, Data); break;
		default: break;
		}
	}

	static std::string FormatComponent(ImGuiDataType DataType, const void* Data)
	{
		switch (DataType)
		{
		case ImGuiDataType_S8: return std::to_string((int)LoadAs<int8_t>(Data));
		case ImGuiDataType_U8: return std::to_string((unsigned)LoadAs<uint8_t>(Data));
		case ImGuiDataType_S16: return std::to_string((int)LoadAs<int16_t>(Data));
		case ImGuiDataType_U16: return std::to_string((unsigned)LoadAs<uint16_t>(Data));
		case ImGuiDataType_S32: return std::to_string(LoadAs<int32_t>(Data));
		case ImGuiDataType_U32: return std::to_string(LoadAs<uint32_t>(Data));
		case ImGuiDataType_Float: return std::format("{}", LoadAs<float>(Data));
		case ImGuiDataType_Double: return std::format("{}", LoadAs<double>(Data));
		default: return {};
		}
	}

	template <typename T>
	static bool ParseInto(std::string_view Text, void* Data)
	{
		T Value{};
		if (!ParseNumber(Text, Value)) return false;
		std::memcpy(Data, &Value, sizeof(T));
		return true;
	}

	static bool ParseComponent(ImGuiDataType DataType, std::string_view Text, void* Data)
	{
		switch (DataType)
		{
		case ImGuiDataType_S8: return ParseInto<int8_t>(Text, Data);
		case ImGuiDataType_U8: return ParseInto<uint8_t>(Text, Data);
		case ImGuiDataType_S16: return ParseInto<int16_t>(Text, Data);
		case ImGuiDataType_U16: return ParseInto<uint16_t>(Text, Data);
		case ImGuiDataType_S32: return ParseInto<int32_t>(Text, Data);
		case ImGuiDataType_U32: return ParseInto<uint32_t>(Text, Data);
		case ImGuiDataType_Float: return ParseInto<float>(Text, Data);
		case ImGuiDataType_Double: return ParseInto<double>(Text, Data);
		default: return false;
		}
	}

	std::string PluginSetting::Key() const
	{
		return VarName;
	}

	std::string PluginSetting::ToString() const
	{
		switch (Type)
		{
		case SettingType::Bool: return *(const bool*)Address ? "true" : "false";
		case SettingType::String: return EscapeSettingString(*(const std::string*)Address);
		case SettingType::Unknown: return {};
		default: break;
		}

		ImGuiDataType DataType = ComponentDataType(Type);
		size_t Size = ComponentSize(DataType);

		std::string Text;
		for (size_t Idx = 0; Idx < ComponentCount(Type); Idx++)
		{
			if (Idx) Text += ',';
			Text += FormatComponent(DataType, (const uint8_t*)Address + Idx * Size);
		}
		return Text;
	}

	bool PluginSetting::FromString(std::string_view Text)
	{
		switch (Type)
		{
		case SettingType::Bool:
		{
			bool Value = false;
			if (!ParseSettingBool(Text, Value)) return false;
			*(bool*)Address = Value;
			return true;
		}
		case SettingType::String:
			*(std::string*)Address = UnescapeSettingString(Text);
			return true;
		case SettingType::Enum:
		{
			int32_t Value = 0;
			if (ParseNumber(Text, Value))
			{
				*(int32_t*)Address = Value;
				return true;
			}

			std::string_view ValueName = Trim(Text);
			for (asUINT Idx = 0; EnumType && Idx < EnumType->GetEnumValueCount(); Idx++)
			{
				int Option = 0;
				const char* OptionName = EnumType->GetEnumValueByIndex(Idx, &Option);
				if (!OptionName || ValueName != OptionName) continue;

				*(int32_t*)Address = Option;
				return true;
			}
			return false;
		}
		case SettingType::Unknown:
			return false;
		default:
			break;
		}

		ImGuiDataType DataType = ComponentDataType(Type);
		size_t Size = ComponentSize(DataType);
		size_t Count = ComponentCount(Type);

		uint8_t Buffer[32]{};
		size_t Start = 0;
		for (size_t Idx = 0; Idx < Count; Idx++)
		{
			size_t Comma = Text.find(',', Start);
			bool Last = Idx + 1 == Count;
			if (Last != (Comma == std::string_view::npos)) return false;

			std::string_view Component = Text.substr(Start, Last ? std::string_view::npos : Comma - Start);
			if (!ParseComponent(DataType, Component, Buffer + Idx * Size)) return false;
			Start = Comma + 1;
		}

		std::memcpy(Address, Buffer, Count * Size);
		return true;
	}

	bool PluginSetting::IsDefault() const
	{
		if (Type == SettingType::String) return *(const std::string*)Address == DefaultString;
		return Default.empty() || std::memcmp(Address, Default.data(), Default.size()) == 0;
	}

	void PluginSetting::Reset()
	{
		if (Type == SettingType::String) *(std::string*)Address = DefaultString;
		else if (!Default.empty()) std::memcpy(Address, Default.data(), Default.size());
	}

	struct ParsedMetadata
	{
		std::string Kind;
		std::vector<std::pair<std::string, std::optional<std::string>>> Attributes;

		const std::optional<std::string>* Find(std::string_view Key) const
		{
			for (const auto& Attribute : Attributes)
			{
				if (EqualsNoCase(Attribute.first, Key)) return &Attribute.second;
			}
			return nullptr;
		}
	};

	static ParsedMetadata ParseMetadata(std::string_view Text)
	{
		ParsedMetadata Result;
		size_t Pos = 0;

		auto SkipSpace = [&]
		{
			while (Pos < Text.size() && std::isspace((uint8_t)Text[Pos])) Pos++;
		};
		auto ReadWord = [&]
		{
			size_t Start = Pos;
			while (Pos < Text.size() && !std::isspace((uint8_t)Text[Pos]) && Text[Pos] != '=') Pos++;
			return std::string(Text.substr(Start, Pos - Start));
		};

		SkipSpace();
		Result.Kind = ReadWord();

		while (true)
		{
			SkipSpace();
			if (Pos >= Text.size()) break;

			std::string Key = ReadWord();
			if (Key.empty())
			{
				Pos++;
				continue;
			}

			SkipSpace();
			std::optional<std::string> Value;
			if (Pos < Text.size() && Text[Pos] == '=')
			{
				Pos++;
				SkipSpace();

				std::string Parsed;
				if (Pos < Text.size() && Text[Pos] == '"')
				{
					Pos++;
					while (Pos < Text.size() && Text[Pos] != '"')
					{
						if (Text[Pos] == '\\' && Pos + 1 < Text.size())
						{
							Pos++;
							Parsed += Text[Pos] == 'n' ? '\n' : Text[Pos];
						}
						else
						{
							Parsed += Text[Pos];
						}
						Pos++;
					}
					Pos++;
				}
				else
				{
					size_t Start = Pos;
					while (Pos < Text.size() && !std::isspace((uint8_t)Text[Pos])) Pos++;
					Parsed = Text.substr(Start, Pos - Start);
				}
				Value = std::move(Parsed);
			}

			Result.Attributes.emplace_back(std::move(Key), std::move(Value));
		}

		return Result;
	}

	static SettingType TypeFromId(asIScriptEngine* Engine, int TypeId, asITypeInfo*& EnumType)
	{
		switch (TypeId)
		{
		case asTYPEID_BOOL: return SettingType::Bool;
		case asTYPEID_INT8: return SettingType::Int8;
		case asTYPEID_INT16: return SettingType::Int16;
		case asTYPEID_INT32: return SettingType::Int32;
		case asTYPEID_UINT8: return SettingType::Uint8;
		case asTYPEID_UINT16: return SettingType::Uint16;
		case asTYPEID_UINT32: return SettingType::Uint32;
		case asTYPEID_FLOAT: return SettingType::Float;
		case asTYPEID_DOUBLE: return SettingType::Double;
		default: break;
		}

		if (TypeId & (asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) return SettingType::Unknown;

		asITypeInfo* Info = Engine->GetTypeInfoById(TypeId);
		if (!Info) return SettingType::Unknown;

		if (Info->GetFlags() & asOBJ_ENUM)
		{
			EnumType = Info;
			return SettingType::Enum;
		}

		const char* InfoNamespace = Info->GetNamespace();
		if (InfoNamespace && *InfoNamespace) return SettingType::Unknown;

		std::string_view Name = Info->GetName();
		if (Name == "string") return SettingType::String;
		if (Name == "vec2") return SettingType::Vec2;
		if (Name == "vec3") return SettingType::Vec3;
		if (Name == "vec4") return SettingType::Vec4;
		if (Name == "int2") return SettingType::Int2;
		if (Name == "int3") return SettingType::Int3;
		if (Name == "nat2") return SettingType::Nat2;
		if (Name == "nat3") return SettingType::Nat3;
		if (Name == "quat") return SettingType::Quat;
		return SettingType::Unknown;
	}

	static asIScriptFunction* FindFunction(asIScriptModule* Module, const std::string& Namespace, const std::string& Name)
	{
		if (!Namespace.empty())
		{
			if (asIScriptFunction* Function = Module->GetFunctionByName((Namespace + "::" + Name).c_str())) return Function;
		}
		return Module->GetFunctionByName(Name.c_str());
	}

	static asIScriptFunction* FindCallback(asIScriptModule* Module, const std::string& Namespace, std::string_view Text, std::string& Error)
	{
		std::string Name(Trim(Text));
		asIScriptFunction* Function = FindFunction(Module, Namespace, Name);
		if (!Function)
		{
			Error = std::format("there's no function named {}", Name);
			return nullptr;
		}
		if (Function->GetReturnTypeId() != asTYPEID_VOID || Function->GetParamCount() != 0)
		{
			Error = std::format("{} must be declared as void {}()", Name, Name);
			return nullptr;
		}
		return Function;
	}

	static std::optional<SettingCondition> ResolveCondition(asIScriptModule* Module, const std::string& Namespace, std::string_view Text, std::string& Error)
	{
		SettingCondition Condition;

		Text = Trim(Text);
		if (!Text.empty() && Text.front() == '!')
		{
			Condition.Negate = true;
			Text = Trim(Text.substr(1));
		}

		size_t Space = Text.find_first_of(" \t");
		std::string Name(Text.substr(0, Space));
		std::string ValueName = Space == std::string_view::npos ? std::string() : std::string(Trim(Text.substr(Space)));
		if (Name.empty())
		{
			Error = "the condition is empty";
			return std::nullopt;
		}

		if (ValueName.empty())
		{
			if (asIScriptFunction* Function = FindFunction(Module, Namespace, Name))
			{
				if (Function->GetReturnTypeId() != asTYPEID_BOOL || Function->GetParamCount() != 0)
				{
					Error = std::format("{} must be declared as bool {}()", Name, Name);
					return std::nullopt;
				}
				Condition.Function = Function;
				return Condition;
			}
		}

		int Index = Module->GetGlobalVarIndexByName(Namespace.empty() ? Name.c_str() : (Namespace + "::" + Name).c_str());
		if (Index < 0 && !Namespace.empty()) Index = Module->GetGlobalVarIndexByName(Name.c_str());
		if (Index < 0)
		{
			Error = std::format("{} isn't a function or a global variable", Name);
			return std::nullopt;
		}

		int TypeId = 0;
		Module->GetGlobalVar((asUINT)Index, nullptr, nullptr, &TypeId);
		Condition.Variable = Module->GetAddressOfGlobalVar((asUINT)Index);

		if (ValueName.empty())
		{
			if (TypeId == asTYPEID_BOOL) return Condition;
			Error = std::format("{} isn't a bool", Name);
			return std::nullopt;
		}

		asITypeInfo* EnumType = Module->GetEngine()->GetTypeInfoById(TypeId);
		if (!EnumType || !(EnumType->GetFlags() & asOBJ_ENUM))
		{
			Error = std::format("{} isn't an enum", Name);
			return std::nullopt;
		}

		for (asUINT Idx = 0; Idx < EnumType->GetEnumValueCount(); Idx++)
		{
			int Option = 0;
			const char* OptionName = EnumType->GetEnumValueByIndex(Idx, &Option);
			if (!OptionName || ValueName != OptionName) continue;

			Condition.EnumValue = Option;
			return Condition;
		}

		Error = std::format("{} has no value named {}", EnumType->GetName(), ValueName);
		return std::nullopt;
	}

	static bool AttributeFlag(const std::optional<std::string>& Value)
	{
		bool Flag = true;
		if (Value && !ParseSettingBool(*Value, Flag)) Flag = true;
		return Flag;
	}

	void CollectSettings(Script& Owner, asIScriptModule* Module, CScriptBuilder& Builder)
	{
		Owner.Settings.clear();
		Owner.SettingsTabs.clear();
		if (!Module) return;

		asIScriptEngine* Engine = Module->GetEngine();
		auto Warn = [&Owner](std::string_view Subject, std::string_view Message)
		{
			LoggertAngelscript.warn(ToU8(std::format("[{}] {}: {}", (const char*)Owner.Id.c_str(), Subject, Message)));
		};

		for (asUINT VarIdx = 0; VarIdx < Module->GetGlobalVarCount(); VarIdx++)
		{
			std::vector<std::string> Metadata = Builder.GetMetadataForVar((int)VarIdx);
			if (Metadata.empty()) continue;

			std::optional<ParsedMetadata> SettingMetadata;
			std::vector<std::string> Aliases;
			for (const std::string& Entry : Metadata)
			{
				ParsedMetadata Parsed = ParseMetadata(Entry);
				if (EqualsNoCase(Parsed.Kind, "Setting"))
				{
					SettingMetadata = std::move(Parsed);
				}
				else if (EqualsNoCase(Parsed.Kind, "SettingAlias"))
				{
					const std::optional<std::string>* AliasKey = Parsed.Find("key");
					if (AliasKey && *AliasKey && !(*AliasKey)->empty()) Aliases.push_back(**AliasKey);
				}
			}
			if (!SettingMetadata) continue;

			const char* VarName = nullptr;
			const char* VarNamespace = nullptr;
			int TypeId = 0;
			bool IsConst = false;
			Module->GetGlobalVar(VarIdx, &VarName, &VarNamespace, &TypeId, &IsConst);

			auto Setting = std::make_shared<PluginSetting>();
			Setting->Owner = &Owner;
			Setting->Namespace = VarNamespace ? VarNamespace : "";
			Setting->VarName = VarName ? VarName : "";
			Setting->Name = Setting->VarName;
			Setting->Aliases = std::move(Aliases);
			Setting->TypeId = TypeId;
			Setting->Type = TypeFromId(Engine, TypeId, Setting->EnumType);
			const char* TypeDeclaration = Engine->GetTypeDeclaration(TypeId, true);
			Setting->TypeName = TypeDeclaration ? TypeDeclaration : "";
			if (Setting->Type == SettingType::Int8 || Setting->Type == SettingType::Int16 || Setting->Type == SettingType::Int32) Setting->TypeName = "int";
			if (Setting->Type == SettingType::Uint8 || Setting->Type == SettingType::Uint16 || Setting->Type == SettingType::Uint32) Setting->TypeName = "uint";
			Setting->Address = Module->GetAddressOfGlobalVar(VarIdx);

			if (IsConst || !Setting->Address)
			{
				Warn(Setting->Key(), "a constant can't be a setting");
				continue;
			}
			if (Setting->Type == SettingType::Unknown)
			{
				Warn(Setting->Key(), std::format("settings of type {} aren't supported", Setting->TypeName));
				continue;
			}

			for (const auto& [AttributeKey, Value] : SettingMetadata->Attributes)
			{
				std::string_view Text = Value ? std::string_view(*Value) : std::string_view();
				std::string Error;

				auto ReadNumber = [&](std::optional<double>& Out)
				{
					double Number = 0.0;
					if (ParseNumber(Text, Number)) Out = Number;
					else Error = std::format("{} isn't a number", Text);
				};
				auto ReadCondition = [&](std::optional<SettingCondition>& Out)
				{
					Out = ResolveCondition(Module, Setting->Namespace, Text, Error);
				};
				auto ReadCallback = [&](asIScriptFunction*& Out)
				{
					Out = FindCallback(Module, Setting->Namespace, Text, Error);
				};

				if (EqualsNoCase(AttributeKey, "name")) Setting->Name = Text;
				else if (EqualsNoCase(AttributeKey, "description")) Setting->Description = Text;
				else if (EqualsNoCase(AttributeKey, "category")) Setting->Category = Text;
				else if (EqualsNoCase(AttributeKey, "hidden")) Setting->Hidden = AttributeFlag(Value);
				else if (EqualsNoCase(AttributeKey, "drag")) Setting->Drag = AttributeFlag(Value);
				else if (EqualsNoCase(AttributeKey, "color")) Setting->Color = AttributeFlag(Value);
				else if (EqualsNoCase(AttributeKey, "multiline")) Setting->Multiline = AttributeFlag(Value);
				else if (EqualsNoCase(AttributeKey, "password")) Setting->Password = AttributeFlag(Value);
				else if (EqualsNoCase(AttributeKey, "min")) ReadNumber(Setting->Min);
				else if (EqualsNoCase(AttributeKey, "max")) ReadNumber(Setting->Max);
				else if (EqualsNoCase(AttributeKey, "step")) ReadNumber(Setting->Step);
				else if (EqualsNoCase(AttributeKey, "if")) ReadCondition(Setting->If);
				else if (EqualsNoCase(AttributeKey, "enableif")) ReadCondition(Setting->EnableIf);
				else if (EqualsNoCase(AttributeKey, "onchange")) ReadCallback(Setting->OnChange);
				else if (EqualsNoCase(AttributeKey, "beforerender")) ReadCallback(Setting->BeforeRender);
				else if (EqualsNoCase(AttributeKey, "afterrender")) ReadCallback(Setting->AfterRender);
				else Error = "unknown attribute";

				if (!Error.empty()) Warn(Setting->Key(), std::format("{}: {}", AttributeKey, Error));
			}

			if (Setting->Type == SettingType::String)
			{
				Setting->DefaultString = *(const std::string*)Setting->Address;
			}
			else
			{
				const uint8_t* Bytes = (const uint8_t*)Setting->Address;
				Setting->Default.assign(Bytes, Bytes + ValueSize(Setting->Type));
			}

			Owner.Settings.push_back(std::move(Setting));
		}

		for (asUINT FunctionIdx = 0; FunctionIdx < Module->GetFunctionCount(); FunctionIdx++)
		{
			asIScriptFunction* Function = Module->GetFunctionByIndex(FunctionIdx);
			if (!Function) continue;

			for (const std::string& Entry : Builder.GetMetadataForFunc(Function))
			{
				ParsedMetadata Parsed = ParseMetadata(Entry);
				if (!EqualsNoCase(Parsed.Kind, "SettingsTab")) continue;

				if (Function->GetReturnTypeId() != asTYPEID_VOID || Function->GetParamCount() != 0)
				{
					Warn(Function->GetName(), "a settings tab must be declared as void F()");
					continue;
				}

				SettingsTab Tab;
				Tab.Function = Function;
				for (const auto& [AttributeKey, Value] : Parsed.Attributes)
				{
					std::string_view Text = Value ? std::string_view(*Value) : std::string_view();
					if (EqualsNoCase(AttributeKey, "name")) Tab.Name = Text;
					else if (EqualsNoCase(AttributeKey, "icon")) Tab.Icon = Text;
					else if (EqualsNoCase(AttributeKey, "order"))
					{
						if (!ParseNumber(Text, Tab.Order)) Warn(Function->GetName(), std::format("order: {} isn't a number", Text));
					}
					else Warn(Function->GetName(), std::format("{}: unknown attribute", AttributeKey));
				}
				Owner.SettingsTabs.push_back(std::move(Tab));
			}
		}

		std::stable_sort(Owner.SettingsTabs.begin(), Owner.SettingsTabs.end(), [](const SettingsTab& Left, const SettingsTab& Right)
		{
			return Left.Order < Right.Order;
		});
	}

	void RegisterSettings(Script& Owner, const SettingsSection& Section)
	{
		if (!Veridian::VastVeridian) return;

		for (auto& Setting : Owner.Settings)
		{
			std::string Key = Setting->Key();
			std::string Stored;
			for (size_t Idx = 0; !Section.Get(Key, Stored) && Idx < Setting->Aliases.size(); Idx++)
			{
				if (Section.Get(Setting->Aliases[Idx], Stored)) Section.Set(Key, Stored);
			}
			for (const std::string& Alias : Setting->Aliases)
			{
				if (Alias != Key) Section.Remove(Alias);
			}

			PluginSetting* Raw = Setting.get();
			Veridian::VCustomFns Custom;
			Custom.ToString = [Raw] { return Raw->ToString(); };
			Custom.IsDefault = [Raw] { return Raw->IsDefault(); };
			Custom.FromString = [Raw](const std::string& Text)
			{
				if (Raw->FromString(Text)) return true;
				LoggertAngelscript.warn(ToU8(std::format("[{}] {}: couldn't read the saved value {}", (const char*)Raw->Owner->Id.c_str(), Raw->Key(), Text)));
				return false;
			};

			Veridian::Register(Section.Name, Key, Setting->Name, Veridian::VSettingType::VCustom, Setting->Address, true, {}, Custom);
		}
	}

	void UnregisterSettings(const SettingsSection& Section)
	{
		Veridian::UnregisterSection(Section.Name);
	}

	bool EvaluateCondition(PluginSetting& Setting, const SettingCondition& Condition)
	{
		bool Result = true;
		if (Condition.Function)
		{
			if (!TheScriptManager || !TheScriptManager->CallFunction(*Setting.Owner, Condition.Function, CallbackType::RenderSettings, {}, &Result)) return true;
		}
		else if (Condition.Variable)
		{
			Result = Condition.EnumValue ? *(const int32_t*)Condition.Variable == *Condition.EnumValue : *(const bool*)Condition.Variable;
		}
		return Condition.Negate ? !Result : Result;
	}

	void NotifySettingChanged(PluginSetting& Setting)
	{
		if (!TheScriptManager || !Setting.Owner) return;

		if (Setting.OnChange) TheScriptManager->CallFunction(*Setting.Owner, Setting.OnChange, CallbackType::OnSettingsChanged);

		asIScriptModule* Module = TheScriptManager->Engine->GetModule((const char*)Setting.Owner->Name.c_str());
		asIScriptFunction* Changed = Module ? Module->GetFunctionByDecl(CallbackTypeSignatures[(size_t)CallbackType::OnSettingsChanged]) : nullptr;
		if (Changed) TheScriptManager->CallFunction(*Setting.Owner, Changed, CallbackType::OnSettingsChanged);
	}

	static int ResizeStringCallback(ImGuiInputTextCallbackData* Data)
	{
		if (Data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			std::string* String = (std::string*)Data->UserData;
			String->resize((size_t)Data->BufTextLen);
			Data->Buf = String->data();
		}
		return 0;
	}

	static bool RenderString(PluginSetting& Setting, const char* Label)
	{
		std::string& Value = *(std::string*)Setting.Address;

		ImGuiInputTextFlags Flags = ImGuiInputTextFlags_CallbackResize;
		if (Setting.Password && !Setting.Multiline) Flags |= ImGuiInputTextFlags_Password;

		bool Changed = Setting.Multiline
			? ImGui::InputTextMultiline(Label, Value.data(), Value.capacity() + 1, ImVec2(0, 0), Flags, ResizeStringCallback, &Value)
			: ImGui::InputText(Label, Value.data(), Value.capacity() + 1, Flags, ResizeStringCallback, &Value);

		if (Changed && Setting.Max && *Setting.Max >= 0.0 && (double)Value.size() > *Setting.Max) Value.resize((size_t)*Setting.Max);
		return Changed;
	}

	static bool RenderEnum(PluginSetting& Setting, const char* Label)
	{
		int32_t& Value = *(int32_t*)Setting.Address;
		std::string Preview = SettingEnumValueName(Setting.EnumType, Value);

		bool Changed = false;
		if (ImGui::BeginCombo(Label, Preview.c_str()))
		{
			for (asUINT Idx = 0; Setting.EnumType && Idx < Setting.EnumType->GetEnumValueCount(); Idx++)
			{
				int Option = 0;
				const char* OptionName = Setting.EnumType->GetEnumValueByIndex(Idx, &Option);
				bool Selected = Option == Value;

				ImGui::PushID((int)Idx);
				if (ImGui::Selectable(OptionName ? OptionName : "?", Selected) && !Selected)
				{
					Value = Option;
					Changed = true;
				}
				if (Selected) ImGui::SetItemDefaultFocus();
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		return Changed;
	}

	static bool RenderScalars(PluginSetting& Setting, const char* Label)
	{
		ImGuiDataType DataType = ComponentDataType(Setting.Type);
		int Components = (int)ComponentCount(Setting.Type);
		size_t Size = ComponentSize(DataType);
		bool Floating = DataType == ImGuiDataType_Float || DataType == ImGuiDataType_Double;

		uint64_t Min = 0;
		uint64_t Max = 0;
		uint64_t Step = 0;
		if (Setting.Min) StoreComponent(DataType, *Setting.Min, &Min);
		if (Setting.Max) StoreComponent(DataType, *Setting.Max, &Max);

		bool Changed = false;
		if (Setting.Drag)
		{
			float Speed = Setting.Step ? (float)*Setting.Step : Floating ? 0.1f : 1.0f;
			Changed = ImGui::DragScalarN(Label, DataType, Setting.Address, Components, Speed, Setting.Min ? &Min : nullptr, Setting.Max ? &Max : nullptr, nullptr, ImGuiSliderFlags_AlwaysClamp);
		}
		else if (Setting.Min && Setting.Max)
		{
			Changed = ImGui::SliderScalarN(Label, DataType, Setting.Address, Components, &Min, &Max, nullptr, ImGuiSliderFlags_AlwaysClamp);
		}
		else
		{
			bool HasStep = Components == 1 && (Setting.Step || !Floating);
			StoreComponent(DataType, Setting.Step ? *Setting.Step : 1.0, &Step);
			Changed = ImGui::InputScalarN(Label, DataType, Setting.Address, Components, HasStep ? &Step : nullptr, nullptr, nullptr);
		}

		if (Changed && (Setting.Min || Setting.Max))
		{
			for (size_t Idx = 0; Idx < (size_t)Components; Idx++)
			{
				uint8_t* Component = (uint8_t*)Setting.Address + Idx * Size;
				double Value = LoadComponent(DataType, Component);
				if (Setting.Min) Value = (std::max)(Value, *Setting.Min);
				if (Setting.Max) Value = (std::min)(Value, *Setting.Max);
				StoreComponent(DataType, Value, Component);
			}
		}
		return Changed;
	}

	static bool RenderValue(PluginSetting& Setting, const char* Label)
	{
		switch (Setting.Type)
		{
		case SettingType::Bool:
			return ImGui::Checkbox(Label, (bool*)Setting.Address);
		case SettingType::String:
			return RenderString(Setting, Label);
		case SettingType::Enum:
			return RenderEnum(Setting, Label);
		case SettingType::Vec3:
			if (Setting.Color) return ImGui::ColorEdit3(Label, (float*)Setting.Address);
			break;
		case SettingType::Vec4:
			if (Setting.Color) return ImGui::ColorEdit4(Label, (float*)Setting.Address);
			break;
		case SettingType::Unknown:
			return false;
		default:
			break;
		}
		return RenderScalars(Setting, Label);
	}

	bool RenderSetting(PluginSetting& Setting)
	{
		if (!Setting.Owner || !Setting.Address) return false;
		if (Setting.If && !EvaluateCondition(Setting, *Setting.If)) return false;
		bool Enabled = !Setting.EnableIf || EvaluateCondition(Setting, *Setting.EnableIf);

		ImGui::PushID(Setting.Address);
		if (Setting.BeforeRender && TheScriptManager) TheScriptManager->CallFunction(*Setting.Owner, Setting.BeforeRender, CallbackType::RenderSettings);

		ImGui::BeginDisabled(!Enabled);
		bool Changed = RenderValue(Setting, Setting.Name.c_str());
		ImGui::EndDisabled();

		if (!Setting.Description.empty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled(ICON_FK_QUESTION_CIRCLE);
			ImGui::SetItemTooltip("%s", Setting.Description.c_str());
		}

		if (Setting.AfterRender && TheScriptManager) TheScriptManager->CallFunction(*Setting.Owner, Setting.AfterRender, CallbackType::RenderSettings);
		ImGui::PopID();

		if (Changed) NotifySettingChanged(Setting);
		return Changed;
	}
}
