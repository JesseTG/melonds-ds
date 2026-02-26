#version 140
layout(std140) uniform uConfig
{
    vec2 uScreenSize;
    uint u3DScale;
    uint uFilterMode;
    vec4 cursorPos;
    bool cursorVisible;
};
in vec2 vPosition;
in vec2 vTexcoord;
smooth out vec2 fTexcoord;
void main()
{
    vec4 fpos;
    fpos.xy = ((vPosition * 2.0) / uScreenSize) - 1.0;
    fpos.y *= -1;
    fpos.z = 0.0;
    fpos.w = 1.0;
    gl_Position = fpos;
    fTexcoord = vTexcoord;
}