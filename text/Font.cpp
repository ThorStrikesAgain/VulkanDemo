#include "Font.h"

#include "FontManager.h"

#include "Application.h"
#include "Shared.h"

#include <string>

namespace VulkanDemo
{
    FT_F26Dot6 FloatToFixed26Dot6(float value)
    {
        return (FT_F26Dot6)(value * (int)(1 << 6));
    }

    Font::Font(const std::string & fontResource) :
        m_FontResource(fontResource)
    {
        FontManager * fontManager = Application::GetInstance().GetFontManager();
        
        auto error = FT_New_Face(fontManager->GetLibrary(), m_FontResource.c_str(), 0, &m_FontFace);
        if (error)
            Fail("Failed to create new font.", -1);

        // TODO: Remove this:
        GetCharData('L', 12);
    }

    Font::~Font()
    {
        FT_Done_Face(m_FontFace);
        m_FontFace = nullptr;
    }

    void Font::GetCharData(char c, float sizeInPoints, int monitorDpi)
    {
        // Get cached data.

        // If the character isn't already generated:
        FT_Error error;

        if (m_LastSizeInPoints != sizeInPoints || m_LastMonitorDpi != monitorDpi)
        {
            FT_F26Dot6 fixedSizeInPoints = FloatToFixed26Dot6(sizeInPoints);
            error = FT_Set_Char_Size(m_FontFace, 0, fixedSizeInPoints, monitorDpi, monitorDpi);
            if (error)
                Fail("Failed to set the character size.", -1);

            m_LastSizeInPoints = sizeInPoints;
            m_LastMonitorDpi = monitorDpi;
        }

        // We assume that font has a Unicode character map.
        FT_UInt glyphIndex = FT_Get_Char_Index(m_FontFace, c);
        if (glyphIndex == 0)
            Fail("The provided character could not be found.", -1);

        error = FT_Load_Glyph(m_FontFace, glyphIndex, FT_LOAD_DEFAULT);
        if (error)
            Fail("Failed to load a glyph.", -1);

        FT_GlyphSlot slot = m_FontFace->glyph;

        error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
        if (error)
            Fail("Failed to render a glyph.", -1);

        FT_Bitmap bitmap = slot->bitmap;
    }
}
