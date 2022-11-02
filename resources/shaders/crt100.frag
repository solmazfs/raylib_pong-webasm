// https://www.shadertoy.com/view/Ms23DR by Mattias
// https://github.com/henriquelalves/SimpleGodotCRTShader by henriquelalves

#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 resolution;
uniform float time;

//out vec4 finalColor;

vec2 curve(vec2 uv)
{
	uv = (uv - 0.5) * 2.0;
	uv *= 1.1;	
	uv.x *= 1.0 + pow((abs(uv.y) / 8.8), 2.0);
	uv.y *= 1.0 + pow((abs(uv.x) / 8.2), 2.0);
	uv  = (uv / 2.0) + 0.5;
	uv =  uv *0.92 + 0.04;
	return uv;
}

void get_color_bleeding(inout vec4 color_left){
    color_left = color_left*vec4(0.13,0.03,0.0,0.8);
}

void main() {
    vec2 q = fragTexCoord;
    vec2 uv = q;
    uv = curve( uv );
    vec3 color = texture2D( texture0, vec2(q.x,q.y) ).xyz;
    // second layer
    float pixel_size_x = 1.0/resolution.x*4.0;
    float pixel_size_y = 1.0/resolution.y*3.2;
    pixel_size_x*=sin(time);
    pixel_size_y*=cos(time);
    vec4 color_left = texture2D(texture0,fragTexCoord - vec2(pixel_size_x, pixel_size_y));
    get_color_bleeding(color_left);
    color+=color_left.xyz;

    float vig = (0.0 + 1.0*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y));
    color *= vec3(pow(vig,0.3)); // fall in
    color *= vec3(2.3,2.8,1.3);
    color *= 0.6;

    float scans = clamp( 0.1+0.10*sin(3.5*time+uv.y*resolution.y*1.3), 0.1, 0.6);
    float s = pow(scans,0.6);
    color = color*vec3( 0.8+0.5*s);
    color *= 1.0+0.01*sin(60.0*time); // frame blink

    if (uv.x < 0.0 || uv.x > 1.0) color *= 0.0;
    if (uv.y < 0.0 || uv.y > 1.0) color *= 0.0;

    gl_FragColor = vec4(color,1.0);
}
