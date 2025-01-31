/*
 * Copyright (c) Wasted Audio 2023 - GPL-3.0-or-later
 */

#include "DistrhoUI.hpp"
#include "ResizeHandle.hpp"
#include "veramobd.hpp"
#include "wstdcolors.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class ImGuiPluginUI : public UI
{
    float ffeedback = 0.0f;
    float fintensity = 20.0f;
    float fmix = 50.0f;
    float fspeed = 2.0f;
    bool frange = false;

    ResizeHandle fResizeHandle;

    // ----------------------------------------------------------------------------------------------------------------

public:
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    ImGuiPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true),
          fResizeHandle(this)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        ImGuiIO& io(ImGui::GetIO());

        ImFontConfig fc;
        fc.FontDataOwnedByAtlas = true;
        fc.OversampleH = 1;
        fc.OversampleV = 1;
        fc.PixelSnapH = true;

        io.Fonts->AddFontFromMemoryCompressedTTF((void*)veramobd_compressed_data, veramobd_compressed_size, 16.0f * getScaleFactor(), &fc);
        io.Fonts->AddFontFromMemoryCompressedTTF((void*)veramobd_compressed_data, veramobd_compressed_size, 21.0f * getScaleFactor(), &fc);
        io.Fonts->AddFontFromMemoryCompressedTTF((void*)veramobd_compressed_data, veramobd_compressed_size, 11.0f * getScaleFactor(), &fc);
        io.Fonts->Build();
        io.FontDefault = io.Fonts->Fonts[1];

        fResizeHandle.hide();
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // DSP/Plugin Callbacks

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
            case 0: {
                ffeedback = value;
                break;
            }
            case 1: {
                fintensity = value;
                break;
            }
            case 2: {
                fmix = value;
                break;
            }
            case 3: {
                fspeed = value;
                break;
            }

            default: return;
        }

        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

   /**
      ImGui specific onDisplay function.
    */
    void onImGuiDisplay() override
    {

        const float width = getWidth();
        const float height = getHeight();
        const float margin = 0.0f;

        ImGui::SetNextWindowPos(ImVec2(margin, margin));
        ImGui::SetNextWindowSize(ImVec2(width - 2 * margin, height - 2 * margin));

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

        style.Colors[ImGuiCol_TitleBgActive] = (ImVec4)WstdTitleBgActive;
        style.Colors[ImGuiCol_WindowBg] = (ImVec4)WstdWindowBg;

        ImGuiIO& io(ImGui::GetIO());
        ImFont* defaultFont = ImGui::GetFont();
        ImFont* titleBarFont = io.Fonts->Fonts[2];
        ImFont* smallFont = io.Fonts->Fonts[3];

        auto intense = (fintensity - 20.0f) / 5.0f;

        auto IntensityActive  = ColorBright(Red, intense);
        auto IntensityHovered = ColorBright(RedBr, intense);
        auto SpeedActive      = ColorBright(Green, intense);
        auto SpeedHovered     = ColorBright(GreenBr, intense);
        auto RangeSw          = ColorBright(WhiteDr, intense);
        auto RangeAct         = ColorBright(GreenDr, intense);
        auto RangeActHovered  = ColorBright(Green, intense);
        auto FeedbackActive   = ColorBright(Blue, intense);
        auto FeedbackHovered  = ColorBright(BlueBr, intense);
        auto MixActive        = ColorMix(SpeedActive, Yellow, intense, fmix);
        auto MixHovered       = ColorMix(SpeedHovered, YellowBr, intense, fmix);

        auto scaleFactor = getScaleFactor();
        const float hundred = 100 * scaleFactor;
        const float toggleWidth  = 20 * scaleFactor;

        auto speedstep = 1.0f;
        auto percstep = 1.0f;

        if (io.KeyShift)
        {
            speedstep = (frange) ? 0.01f : 0.001f;
            percstep = 0.1f;
        }
        else
        {
            speedstep = (frange) ? 0.1f : 0.01f;
        }

        if(fspeed > 2.0f)
            frange = true;

        ImGui::PushFont(titleBarFont);
        if (ImGui::Begin("WSTD FLANGR", nullptr, ImGuiWindowFlags_NoResize + ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Dummy(ImVec2(0.0f, 8.0f * scaleFactor));
            ImGui::PushFont(defaultFont);
            auto ImGuiKnob_Flags = ImGuiKnobFlags_DoubleClickReset + ImGuiKnobFlags_ValueTooltip + ImGuiKnobFlags_NoInput + ImGuiKnobFlags_ValueTooltipHideOnClick;
            auto ImGuiKnob_FlagsDB = ImGuiKnob_Flags + ImGuiKnobFlags_dB;

            ImGui::PushStyleColor(ImGuiCol_ButtonActive,    (ImVec4)IntensityActive);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   (ImVec4)IntensityHovered);
            if (ImGuiKnobs::Knob("Intensity", &fintensity, 0.0f, 100.0f, percstep, "%.1f%%", ImGuiKnobVariant_SteppedTick, hundred, ImGuiKnob_Flags, 11))
            {
                if (ImGui::IsItemActivated())
                {
                    editParameter(1, true);
                    if (ImGui::IsMouseDoubleClicked(0))
                        fintensity = 20.0f;
                }
                setParameterValue(1, fintensity);
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();

            ImGui::BeginGroup();
            {
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,    (ImVec4)SpeedActive);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   (ImVec4)SpeedHovered);
                auto flow_max = 20.0f;
                auto flow_steps = 21;
                if (not frange)
                {
                    flow_max = 2.0f;
                    flow_steps = 11;
                }

                if (ImGuiKnobs::Knob("Speed", &fspeed, 0.0f, flow_max, speedstep, "%.3fHz", ImGuiKnobVariant_SteppedTick, hundred, ImGuiKnob_Flags, flow_steps))
                {
                    if (ImGui::IsItemActivated())
                    {
                        editParameter(3, true);
                        if (ImGui::IsMouseDoubleClicked(0))
                            fspeed = 2.0f;
                    }
                    setParameterValue(3, fspeed);
                }
                ImGui::PopStyleColor(2);
                ImGui::SameLine();

                ImGui::BeginGroup();
                {
                    // Title text
                    ImGui::PushStyleColor(ImGuiCol_Text, TextClr);
                    CenterTextX("Range", toggleWidth);
                    ImGui::Dummy(ImVec2(0.0f, 20.0f) * scaleFactor);

                    // Range text
                    ImGui::PushFont(smallFont);
                    auto rangedef = (frange) ? "fast": "slow";
                    CenterTextX(rangedef, toggleWidth);
                    ImGui::PopFont();

                    ImGui::PushFont(defaultFont);
                    ImGui::PopStyleColor();

                    // knob
                    ImGui::PushStyleColor(ImGuiCol_Text,            (ImVec4)RangeSw);

                    // inactive colors
                    ImGui::PushStyleColor(ImGuiCol_FrameBg,         (ImVec4)RangeAct);
                    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  (ImVec4)RangeActHovered);

                    // active colors
                    ImGui::PushStyleColor(ImGuiCol_Button,          (ImVec4)RangeAct);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   (ImVec4)RangeActHovered);

                    if (ImGui::Toggle("##Range", &frange, ImGuiToggleFlags_Animated))
                    {
                        if (ImGui::IsItemActivated() && !frange)
                        {
                            editParameter(3, true);
                            fspeed = std::min(fspeed, 2.0f);
                            setParameterValue(3, fspeed);
                        }
                    }
                    ImGui::PopStyleColor(5);

                    ImGui::PopFont();
                }
                ImGui::EndGroup();
            }
            ImGui::EndGroup();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_ButtonActive,    (ImVec4)FeedbackActive);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   (ImVec4)FeedbackHovered);
            if (ImGuiKnobs::Knob("Feedback", &ffeedback, -100.0f, 100.0f, percstep, "%.1f%%", ImGuiKnobVariant_SpaceBipolar, hundred, ImGuiKnob_Flags))
            {
                if (ImGui::IsItemActivated())
                {
                    editParameter(0, true);
                    if (ImGui::IsMouseDoubleClicked(0))
                        ffeedback = 0.0f;
                }
                setParameterValue(0, ffeedback);
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_ButtonActive,    (ImVec4)MixActive);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,   (ImVec4)MixHovered);
            if (ImGuiKnobs::Knob("Mix", &fmix, 0.0f, 100.0f, percstep, "%.1f%%", ImGuiKnobVariant_SteppedTick, hundred, ImGuiKnob_Flags, 11))
            {
                if (ImGui::IsItemActivated())
                {
                    editParameter(2, true);
                    if (ImGui::IsMouseDoubleClicked(0))
                        fmix = 50.0f;
                }
                setParameterValue(2, fmix);
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();

            if (ImGui::IsItemDeactivated())
            {
                editParameter(0, false);
                editParameter(1, false);
                editParameter(2, false);
                editParameter(3, false);
            }

            ImGui::PopFont();
        }
        ImGui::PopFont();
        ImGui::End();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiPluginUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new ImGuiPluginUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
