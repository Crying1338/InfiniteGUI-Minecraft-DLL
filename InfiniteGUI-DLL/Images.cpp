#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Images.h"



GLuint LoadTextureFromMemory(const unsigned char* buffer, size_t size, int* outWidth, int* outHeight)
{
    int w, h, channels;
    unsigned char* data = stbi_load_from_memory(buffer, size, &w, &h, &channels, 4);
    if (!data)
        return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA,
        w, h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, data
    );

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    if (outWidth) *outWidth = w;
    if (outHeight) *outHeight = h;

    return tex;
}
