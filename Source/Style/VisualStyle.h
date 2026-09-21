/*
  ==============================================================================

    VisualStyle.h
    Created: 6 Dec 2025 10:27:34am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

#pragma once
#include <juce_graphics/juce_graphics.h>

namespace VisualStyle
{
    // ============================================================
    //  CORE BACKGROUND & PANEL COLORS  (Retro Futuristic Base)
    // ============================================================

    static const juce::Colour background        = juce::Colour::fromRGB(10, 10, 15);     // Nearly black, slight blue tint
    static const juce::Colour panelBackground   = juce::Colour::fromRGB(18, 18, 26);     // Subtle difference to create component separation
    static const juce::Colour panelBorder       = juce::Colour::fromRGB(60, 60, 80);     // Non-vibrant perimetric border

    // Optional glow color for outlines or hover effects
    static const juce::Colour neonGlow          = juce::Colour::fromRGB(120, 150, 255)
                                                        .withAlpha(0.15f);


    // ============================================================
    //  RETRO FUTURISTIC PALETTE (Highlight / Dim / Disabled)
    // ============================================================

    struct ColorSet
    {
        juce::Colour highlight;    // active / selected state
        juce::Colour dim;          // inactive / normal
        juce::Colour disabled;     // fully inactive
    };

    namespace Palette
    {
        // MAGENTA / NEON PINK ------------------------------------
        static const ColorSet neon_pink =
        {
            juce::Colour::fromRGB(255, 0, 120),        // highlight
            juce::Colour::fromRGB(150, 0, 80),         // dim
            juce::Colour::fromRGB(70, 0, 40)           // disabled
        };

        // ELECTRIC BLUE ------------------------------------------
        static const ColorSet electric_blue =
        {
            juce::Colour::fromRGB(0, 170, 255),
            juce::Colour::fromRGB(0, 100, 150),
            juce::Colour::fromRGB(0, 60, 90)
        };

        // CYBER GREEN --------------------------------------------
        static const ColorSet cyber_green =
        {
            juce::Colour::fromRGB(0, 255, 140),
            juce::Colour::fromRGB(0, 160, 90),
            juce::Colour::fromRGB(0, 90, 50)
        };

        // LASER YELLOW --------------------------------------------
        static const ColorSet yellow =
        {
            juce::Colour::fromRGB(255, 240, 0),
            juce::Colour::fromRGB(150, 140, 0),
            juce::Colour::fromRGB(90, 80, 0)
        };
    
        // RED / pure red ------------------------------------
        static const ColorSet red =
        {
            juce::Colour::fromRGB(255, 0, 0),        // highlight
            juce::Colour::fromRGB(150, 0, 0),         // dim
            juce::Colour::fromRGB(70, 0, 0)           // disabled
        };
        
        // pure_blue / pure blue ------------------------------------
        static const ColorSet blue =
        {
            juce::Colour::fromRGB(0, 0, 255),        // highlight
            juce::Colour::fromRGB(0, 0, 150),         // dim
            juce::Colour::fromRGB(0, 0, 70)           // disabled
        };
    
        // pure_green / pure green ------------------------------------
        static const ColorSet green =
        {
            juce::Colour::fromRGB(0, 255, 0),        // highlight
            juce::Colour::fromRGB(0, 150, 0),         // dim
            juce::Colour::fromRGB(0, 70, 0)           // disabled
        };
        
        // cyan / pure cyan ------------------------------------
        static const ColorSet cyan =
        {
            juce::Colour::fromRGB(0, 255, 255),        // highlight
            juce::Colour::fromRGB(0, 150, 150),         // dim
            juce::Colour::fromRGB(0, 70, 70)           // disabled
        };
        
        // MAGENTA / pure_magenta ------------------------------------
        static const ColorSet magenta =
        {
            juce::Colour::fromRGB(255, 0, 255),        // highlight
            juce::Colour::fromRGB(150, 0, 150),         // dim
            juce::Colour::fromRGB(70, 0, 70)           // disabled
        };
    
        // DARK BLUE / pure dark bleu ------------------------------------
        static const ColorSet dark_blue =
        {
            juce::Colour::fromRGB(0, 0, 120),        // highlight
            juce::Colour::fromRGB(0, 0, 80),         // dim
            juce::Colour::fromRGB(0, 0, 40)           // disabled
        };
    }


    // ============================================================
    //  TEXT COLORS / TYPOGRAPHY
    // ============================================================

    // Common text colors (semi-neon)
    static const juce::Colour textPrimary      = Palette::blue.highlight;  // Default text glow
    static const juce::Colour textSecondary    = Palette::blue.dim;
    static const juce::Colour textDisabled     = Palette::blue.disabled;

    // Font sizes (you can tweak globally)
    namespace FontSize
    {
        static constexpr float small      = 12.0f;
        static constexpr float normal     = 14.0f;
        static constexpr float medium     = 16.0f;
        static constexpr float large      = 20.0f;
        static constexpr float title      = 26.0f;
    }

    // Optional: use a digital-style or monospaced fallback
    inline juce::Font getDefaultFont(float size)
    {
        return juce::Font("Menlo", size, juce::Font::plain);
    }


    // ============================================================
    //  COMPONENT GEOMETRY / CONSTANTS
    // ============================================================

    namespace Geometry
    {
        // Components
        static constexpr float componentBorderThickness = 2.0f;
        static constexpr float componentCornerRadius    = 6.0f;
        
        // Buttons / Toggles
        static constexpr float buttonBorderThickness = 2.0f;
        static constexpr float buttonCornerRadius    = 6.0f;

        // Toggles (circle)
        static constexpr float toggleDiameter        = 18.0f;

        // Sliders
        static constexpr float sliderTrackThickness  = 4.0f;
        static constexpr float sliderThumbSize       = 12.0f;   // Only if thumb is used

        // Margins / spacing
        static constexpr int componentMargin         = 8;
        static constexpr int componentPadding        = 6;
    }


    // ============================================================
    //  HELPERS
    // ============================================================

    // Returns the correct color for a given state.
    inline juce::Colour getStateColor(const ColorSet& set, bool isActive, bool isEnabled = true)
    {
        if (!isEnabled) return set.disabled;
        return isActive ? set.highlight : set.dim;
    }
}
