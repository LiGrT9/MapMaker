#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textures[8];
uniform int textureCount;

void main()
{
    vec4 color = vec4(0.0);
    if (textureCount > 0) color += texture(textures[0], TexCoord);
    if (textureCount > 1) color += texture(textures[1], TexCoord);
    if (textureCount > 2) color += texture(textures[2], TexCoord);
    if (textureCount > 3) color += texture(textures[3], TexCoord);
    if (textureCount > 4) color += texture(textures[4], TexCoord);
    if (textureCount > 5) color += texture(textures[5], TexCoord);
    if (textureCount > 6) color += texture(textures[6], TexCoord);
    if (textureCount > 7) color += texture(textures[7], TexCoord);
    FragColor = textureCount > 0 ? color / float(textureCount) : vec4(1.0);
}

