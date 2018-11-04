#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>

namespace VulkanDemo
{
    class Font;

    class FontManager
    {
    public:
        FontManager();
        ~FontManager();

        void LoadDefaultFonts();
        void UnloadDefaultFonts();

        FT_Library GetLibrary() { return m_Library; }

        // Stock fonts:
        inline Font* GetVerdana() const { return m_Verdana; }

    private:
        FontManager& operator=(const FontManager& orig) = delete;

        bool m_DefaultFontsLoaded = false;
        FT_Library m_Library = nullptr;

        Font* m_Verdana = nullptr;
    };
}
