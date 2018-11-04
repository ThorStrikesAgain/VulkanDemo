#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>

namespace VulkanDemo
{
    class Font
    {
    public:
        Font(const std::string & fontFace);
        ~Font();

        void GetCharData(char c, float sizeInPoints, int monitorDpi = 96);

    private:
        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        FT_Face m_FontFace = nullptr;

        const std::string m_FontResource;

        float m_LastSizeInPoints = -1;
        int m_LastMonitorDpi = -1;
    };
}
