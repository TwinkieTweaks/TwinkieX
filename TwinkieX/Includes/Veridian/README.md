# Veridian
A setting manager using INI files and dear ImGui.

## Usage
```cpp
#include "Veridian.h"
#include "imgui.h"

void MyRenderLoop()
{
    static float MyFloat = 0.f;
    static ImVec4 MyColor = ImVec4;

    static bool Registered = false;
    if (!Registered)
    {
        Veridian::InitContext("MySettings.ini");

        //               Section   Internal setting name  User-facing name              Type                 Value
        Veridian::Export("Floats",     "MyFloat",         "Example float", Veridian::VSettingType::VFloat, &MyFloat);
        Veridian::Export("Colors",     "MyColor",         "Example color", Veridian::VSettingType::VVec4,  &MyColor);
    }

    ImGui::Begin("Veridian settings");

    Veridian::RenderAll();

    ImGui::End();
}
```
<img width="458" height="200" alt="image" src="https://github.com/user-attachments/assets/6334be39-011f-4e5a-8fac-5dc451999321" />

## Invalid setting handling
If (for whatever reason), the setting is invalid (nullptr was registered as setting, or invalid setting type), then Veridian will handle both cases properly: 

<img width="668" height="145" alt="image" src="https://github.com/user-attachments/assets/6bd816e6-c9fc-415d-85a6-ef73a9b8de6e" />
