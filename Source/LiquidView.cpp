#include "LiquidView.h"

namespace
{
// ---- Shaders ---------------------------------------------------
// GLSL 150 core: widely supported on Windows/macOS/Linux. Uses gl_VertexID
// trick to generate a fullscreen triangle without any VBO data.
static const char* kVertexShader = R"GLSL(
#version 150 core
out vec2 vUv;
void main()
{
    // Fullscreen triangle (covers -1..1 in NDC with a single triangle).
    vec2 verts[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 p = verts[gl_VertexID];
    vUv = (p * 0.5) + 0.5;
    gl_Position = vec4(p, 0.0, 1.0);
}
)GLSL";

static const char* kFragmentShader = R"GLSL(
#version 150 core
in vec2 vUv;
out vec4 fragColor;

uniform vec2  uRes;
uniform float uTime;
uniform float uLevel;      // 0..1 audio peak (smoothed)
uniform float uLfo;        // 0..1 phase
uniform float uCutoff;
uniform float uReso;
uniform float uShimmer;
uniform float uDrive;
uniform float uCharacter;
uniform float uMovement;
uniform float uDetune;

// ---- Noise helpers ----
float hash21(vec2 p){
    p = fract(p * vec2(234.34, 435.345));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

float vnoise(vec2 p){
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    vec2 u = f*f*(3.0 - 2.0*f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main()
{
    // Aspect-correct centred coords
    vec2 uv = vUv * 2.0 - 1.0;
    float ar = uRes.x / max(uRes.y, 1.0);
    uv.x *= ar;

    // Time modulated by Motion
    float t  = uTime * (0.6 + uMovement * 0.8);
    float lfo = sin(uLfo * 6.2831853);

    // Curl-ish domain distortion for liquid look
    vec2 warp = vec2(
        vnoise(uv * 1.8 + t * 0.25) - 0.5,
        vnoise(uv * 1.8 - t * 0.22 + 7.1) - 0.5
    ) * (0.35 + 0.25 * uDrive + 0.15 * uLevel);
    vec2 wuv = uv + warp;

    // --- Metaball field ---
    float field = 0.0;

    // Central mass - always present
    float rCentre = length(wuv);
    float sizeC = 0.38 + 0.22 * uLevel + 0.04 * lfo;
    field += (sizeC * sizeC) / (rCentre * rCentre + 0.05);

    // 5 orbiting blobs
    const int N = 5;
    for (int i = 0; i < N; ++i) {
        float fi = float(i);
        float ang = t * 0.35 + fi * 1.2566 + lfo * uMovement * 1.2;
        float rad = 0.28 + 0.22 * sin(t * 0.41 + fi * 1.7) + 0.18 * uLevel;
        vec2  pos = vec2(cos(ang), sin(ang * 1.07 + fi)) * rad;
        float r = length(wuv - pos);
        float w = 0.11 + 0.06 * uCharacter + 0.03 * uDetune;
        field += w / (r * r + 0.02);
    }

    // High-frequency surface ripple when Cutoff is open
    float ripple = vnoise(uv * (18.0 + uCutoff * 30.0) + t * 1.6);
    field += (ripple - 0.5) * 0.18 * uCutoff;

    // --- Liquid boundary (soft edge) ---
    float edge   = smoothstep(0.85, 1.05, field);     // inside the ink
    float rim    = exp(-pow(field - 0.98, 2.0) * 24.0);// glowing border band

    // --- Colour palette ---
    vec3 ink       = vec3(0.015, 0.020, 0.030);
    vec3 gold      = vec3(0.79, 0.72, 0.48);
    vec3 violet    = vec3(0.56, 0.48, 0.85);
    vec3 teal      = vec3(0.35, 0.70, 0.78);
    vec3 tint      = mix(gold, violet, clamp(uShimmer, 0.0, 1.0));
    tint           = mix(tint, teal, clamp(uMovement * 0.55, 0.0, 1.0));

    // Bowl / background : very subtle gradient
    float bowl = smoothstep(1.35, 0.0, length(uv));
    vec3 bg = mix(vec3(0.055, 0.058, 0.068),
                  vec3(0.085, 0.083, 0.090), bowl);

    // Composite
    vec3 col = bg;
    col = mix(col, ink, edge);                                   // black fill
    col += tint * rim * (0.25 + uLevel * 1.6);                   // glow rim
    col += tint * 0.03 * edge * (1.0 + uLevel * 2.0);            // inner sheen

    // Specular highlight at top-left of each blob (liquid gloss)
    float gloss = pow(max(0.0, dot(normalize(wuv - vec2(-0.35, 0.35)), vec2(0.0, 1.0))), 6.0);
    col += vec3(0.9, 0.87, 0.75) * gloss * edge * 0.20;

    // Resonance adds a little chromatic fringe on the rim
    float fringe = rim * uReso * 0.35;
    col.r += fringe * 0.15;
    col.b += fringe * 0.10;

    // Vignette
    float v = smoothstep(1.7, 0.25, length(uv));
    col *= mix(0.55, 1.0, v);

    fragColor = vec4(col, 1.0);
}
)GLSL";
} // namespace

LiquidView::LiquidView (OctoPadAudioProcessor& p) : proc (p)
{
    setOpaque (true);
    openGLContext.setRenderer (this);
    openGLContext.setContinuousRepainting (true);
    openGLContext.setComponentPaintingEnabled (false);
    openGLContext.attachTo (*this);
    startTimeSec = juce::Time::getMillisecondCounterHiRes() * 0.001;
}

LiquidView::~LiquidView()
{
    openGLContext.detach();
}

void LiquidView::paint (juce::Graphics& g)
{
    // Only used before the GL context has rendered its first frame.
    g.fillAll (juce::Colour (0xff0b0d11));
    g.setColour (juce::Colour (0xffc9b87a).withAlpha (0.4f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f, 1.0f);
}

void LiquidView::resized() {}

float LiquidView::readParam (const char* id) const
{
    if (auto* v = proc.apvts.getRawParameterValue (id)) return v->load();
    return 0.0f;
}

void LiquidView::newOpenGLContextCreated()
{
    auto newShader = std::make_unique<juce::OpenGLShaderProgram> (openGLContext);
    bool ok = newShader->addVertexShader   (kVertexShader)
           && newShader->addFragmentShader (kFragmentShader)
           && newShader->link();
    if (! ok)
    {
        DBG ("LiquidView shader compile failed: " << newShader->getLastError());
        shaderReady = false;
        return;
    }
    shader = std::move (newShader);

    // Empty VAO (GL core profile requires one bound for glDrawArrays)
    juce::gl::glGenVertexArrays (1, &vao);

    auto U = [&](const char* n)
    {
        return std::make_unique<juce::OpenGLShaderProgram::Uniform> (*shader, n);
    };
    uRes       = U ("uRes");
    uTime      = U ("uTime");
    uLevel     = U ("uLevel");
    uLfo       = U ("uLfo");
    uCutoff    = U ("uCutoff");
    uReso      = U ("uReso");
    uShimmer   = U ("uShimmer");
    uDrive     = U ("uDrive");
    uCharacter = U ("uCharacter");
    uMovement  = U ("uMovement");
    uDetune    = U ("uDetune");
    shaderReady = true;
}

void LiquidView::renderOpenGL()
{
    const float scale = (float) openGLContext.getRenderingScale();
    const int w = juce::roundToInt (getWidth()  * scale);
    const int h = juce::roundToInt (getHeight() * scale);
    juce::gl::glViewport (0, 0, w, h);

    juce::OpenGLHelpers::clear (juce::Colour (0xff0b0d11));

    if (! shaderReady || shader == nullptr) return;

    shader->use();

    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001 - startTimeSec;

    if (uRes)       uRes      ->set ((float) w, (float) h);
    if (uTime)      uTime     ->set ((float) now);
    if (uLevel)     uLevel    ->set (juce::jlimit (0.0f, 1.5f, proc.uiAudioLevel.load()));
    if (uLfo)       uLfo      ->set (proc.uiLfoPhase.load());
    if (uCutoff)    uCutoff   ->set (readParam ("cutoff"));
    if (uReso)      uReso     ->set (readParam ("resonance"));
    if (uShimmer)   uShimmer  ->set (readParam ("shimmer"));
    if (uDrive)     uDrive    ->set (readParam ("drive"));
    if (uCharacter) uCharacter->set (readParam ("character"));
    if (uMovement)  uMovement ->set (readParam ("movement"));
    if (uDetune)    uDetune   ->set (readParam ("detune"));

    juce::gl::glBindVertexArray (vao);
    juce::gl::glDrawArrays (juce::gl::GL_TRIANGLES, 0, 3);
    juce::gl::glBindVertexArray (0);
}

void LiquidView::openGLContextClosing()
{
    if (vao != 0)
    {
        juce::gl::glDeleteVertexArrays (1, &vao);
        vao = 0;
    }
    shader.reset();
    uRes.reset(); uTime.reset(); uLevel.reset(); uLfo.reset();
    uCutoff.reset(); uReso.reset(); uShimmer.reset(); uDrive.reset();
    uCharacter.reset(); uMovement.reset(); uDetune.reset();
    shaderReady = false;
}
