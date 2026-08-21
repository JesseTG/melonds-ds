#version 140
layout(std140) uniform uConfig
{
    vec2 uScreenSize;
    uint u3DScale;
    uint uFilterMode;
    vec4 cursorPos;
    bool cursorVisible;
};
uniform sampler2D ScreenTex;
smooth in vec2 fTexcoord;
out vec4 oColor;
void main()
{
    vec4 pixel = texture(ScreenTex, fTexcoord);
    // virtual cursor so you can see where you touch
    vec2 fragPos = vec2(gl_FragCoord.x, uScreenSize.y - gl_FragCoord.y);
    if (cursorVisible &&
        cursorPos.x <= fragPos.x && cursorPos.y <= fragPos.y &&
        cursorPos.z >= fragPos.x && cursorPos.w >= fragPos.y) {
        pixel = vec4(1.0 - pixel.r, 1.0 - pixel.g, 1.0 - pixel.b, pixel.a);
    }
    oColor = vec4(pixel.bgr, 1.0);
}