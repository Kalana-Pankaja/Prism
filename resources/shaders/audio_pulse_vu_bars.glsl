// Audio-Pulse VU Bar Array
// Connect a clip's AudioOut to this shader's ShaderAudioIn for real FFT data.
//
// Optional audio uniforms (fallback to time-based motion when u_hasAudio is false):
//   uniform sampler2D u_spectrum;   // 1xN, .r = magnitude 0..1, x = low→high freq
//   uniform float     u_audioLevel; // overall level 0..1
//   uniform vec3      u_bands;      // x=low y=mid z=high (0..1)
//   uniform float     u_beatPulse;  // transient pulse envelope (0..1)
//   uniform bool      u_hasAudio;   // false when no audio connected

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 u_resolution;
uniform float u_time;
uniform sampler2D u_spectrum;
uniform float u_audioLevel;
uniform vec3 u_bands;
uniform float u_beatPulse;
uniform bool u_hasAudio;

const float TEMPO = 4.0;
const vec3 BAR_COLOR = vec3(0.2, 1.0, 0.5);

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    float t = u_time * TEMPO;

    float columns = 16.0;
    float barIdx = floor(uv.x * columns);
    float localX = fract(uv.x * columns) - 0.5;

    float peakHeight;
    float accent = 0.0;
    if (u_hasAudio) {
        float specX = (barIdx + 0.5) / columns;
        float s = texture2D(u_spectrum, vec2(specX, 0.5)).r;

        float bandBias;
        if (specX < 0.25) {
            bandBias = u_bands.x;
        } else if (specX < 0.70) {
            bandBias = u_bands.y;
        } else {
            bandBias = u_bands.z;
        }

        peakHeight = 0.07 + s * (0.65 + u_audioLevel * 0.25) + bandBias * 0.28;
        accent = 0.3 * bandBias + 0.7 * u_beatPulse;
    } else {
        peakHeight = 0.2 + 0.6 * sin(barIdx * 45.12 + t) * cos(barIdx * 12.75 + t * 0.3);
        accent = 0.0;
    }
    peakHeight = max(0.1, peakHeight);

    float distX = abs(localX) - 0.35;
    float distY = uv.y - peakHeight;

    float clampX = smoothstep(0.0, -0.05, distX);
    float bloomY = 0.015 / (abs(distY) + 0.005);

    float barCore = uv.y < peakHeight ? (1.8 + accent * 0.6) : bloomY;
    float renderEngine = clampX * barCore;

    vec3 color = BAR_COLOR;
    color += vec3(0.25, 0.12, 0.02) * u_bands.x;
    color += vec3(0.08, 0.20, 0.26) * u_bands.z;
    color *= 1.0 + u_beatPulse * 0.35;

    gl_FragColor = vec4(color * renderEngine * (0.6 + uv.y), 1.0);
}
