#version 130 core

attribute vec2 aPos;
attribute vec2 inTex;

out vec2 vUV;

uniform vec2 uCamCenter;
uniform float uZoom;

void main()
{
   
    vec2 zoomedUV = (inTex - uCamCenter) * uZoom + uCamCenter;

    zoomedUV = clamp(zoomedUV, 0.0, 1.0);

    vUV = zoomedUV;

    gl_Position = vec4(aPos, 0.0, 1.0);
}



