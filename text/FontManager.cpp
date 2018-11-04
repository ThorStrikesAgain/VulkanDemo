#include "FontManager.h"

#include "Font.h"

#include "Resources.h"
#include "Shared.h"

namespace VulkanDemo
{
    FontManager::FontManager()
    {
        auto error = FT_Init_FreeType(&m_Library);
        if (error)
            Fail("Failed to initialize freetype", -1);
    }

    FontManager::~FontManager()
    {
        UnloadDefaultFonts();

        auto error = FT_Done_FreeType(m_Library);
        if (error)
            Fail("Failed to destroy freetype", -1);
    }

    void FontManager::LoadDefaultFonts()
    {
        if (m_DefaultFontsLoaded)
            return;

        m_Verdana = new Font(Resources::FontVerdana);
    }

    void FontManager::UnloadDefaultFonts()
    {
        if (!m_DefaultFontsLoaded)
            return;

        delete m_Verdana;
        m_Verdana = nullptr;
    }
}
