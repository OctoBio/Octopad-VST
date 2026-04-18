#include "LiquidView.h"

namespace
{
// ---- Shaders ---------------------------------------------------
static const char* kVertexShader = R"GLSL(
#version 150 core
out vec2 vUv;
void main()
{
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

// Rich reactive ink-blob shader.
//
// Conceptually: a bowl of ink viewed from above. A central textured ink
// mass drifts and splits into orbiting droplets; delay creates echoed
// ghost blobs; reverb produces diffusion halo; cutoff adds high-freq
// surface ripple; drive warps contours; resonance produces chromatic
// fringe; chorus adds wavy distortion; detune stretches; shimmer shifts
// the tint; movement adds teal; character stylises; noise adds grain;
// sub pulses the core; tone tilts warm/cool.
static const char* kFragmentShader = R"GLSL(
#version 150 core
in vec2 vUv;
out vec4 fragColor;

uniform vec2  uRes;
uniform float uTime;
uniform float uLevel;
uniform float uLfo;

uniform float uCharacter, uDetune, uSub, uNoise;
uniform float uCutoff, uReso, uDrive, uTone;
uniform float uAttack, uRelease, uGlide, uSpread, uMovement;
uniform float uChorus, uShimmer;
uniform float uReverbSize, uReverbMix;
uniform float uDelayTime, uDelayFb, uDelayMix;
uniform float uGain;

// ---- Hash / noise -----------------------------------------------
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
    return mix(mix(a,b,u.x), mix(c,d,u.x), u.y);
}

float fbm(vec2 p, int oct){
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 6; ++i) {
        if (i >= oct) break;
        v += a * vnoise(p);
        p = p * 2.03 + 17.1;
        a *= 0.5;
    }
    return v;
}

// Domain warp helper
vec2 warpField(vec2 uv, float t){
    vec2 q = vec2(fbm(uv + vec2(0.0, t*0.17), 3),
                  fbm(uv + vec2(5.2, t*0.22), 3));
    return uv + q * 0.35;
}

void main()
{
    vec2 uv = vUv * 2.0 - 1.0;
    float ar = uRes.x / max(uRes.y, 1.0);
    uv.x *= ar;

    float t   = uTime * (0.45 + uMovement * 1.4);   // speed scales with Movement
    float lfo = sin(uLfo * 6.2831853);

    // ============================================================
    // DISTORTION : each effect injects its own deformation style
    // ============================================================
    vec2 warp = vec2(0.0);

    // Motion : base lazy curl
    warp += (vec2(fbm(uv*1.4 + t*0.18, 4),
                  fbm(uv*1.4 - t*0.15 + 7.1, 4)) - 0.5) * (0.25 + uMovement * 1.1);

    // Drive : sharper, finer warp
    warp += (vec2(fbm(uv*4.0 + t*0.9, 2),
                  fbm(uv*4.0 - t*1.0 + 3.3, 2)) - 0.5) * uDrive * 1.1;

    // Resonance : radial swirl
    float ang = atan(uv.y, uv.x);
    float rad = length(uv);
    float swirl = sin(rad * 7.0 - t * 2.0) * uReso * 0.55;
    warp += vec2(-sin(ang), cos(ang)) * swirl * 0.25;

    // Chorus : smooth sinusoidal wobble
    warp.x += sin(uv.y * 6.0 + t * 1.7) * uChorus * 0.22;
    warp.y += cos(uv.x * 6.0 + t * 1.3) * uChorus * 0.22;

    // Detune : horizontal stretch/split
    warp.x += sin(uv.y * 3.0 + t * 0.5) * uDetune * 0.30;

    // Glide : slow horizontal drift
    warp.x += sin(t * 0.3) * uGlide * 0.25;

    // Reverb Size : soft large-scale push
    warp += (vec2(fbm(uv*0.5 + t*0.05, 3),
                  fbm(uv*0.5 - t*0.04 + 9.1, 3)) - 0.5) * uReverbSize * 0.45;

    // Cutoff : high-freq surface tremble
    warp += (vec2(vnoise(uv*(8.0 + uCutoff*30.0) + t*1.5),
                  vnoise(uv*(8.0 + uCutoff*30.0) - t*1.6 + 4.0)) - 0.5)
            * uCutoff * 0.18;

    vec2 wuv = uv + warp;

    // ============================================================
    // METABALL FIELD
    // ============================================================
    float field = 0.0;

    // Central ink mass - Sub param inflates it, audio level pulses it
    float rCentre = length(wuv);
    float coreSize = 0.32 + 0.18 * uSub + 0.22 * uLevel + 0.05 * lfo;
    field += (coreSize * coreSize) / (rCentre * rCentre + 0.05);

    // Orbiting droplets - Spread pushes them out, Character weights them
    const int N = 6;
    for (int i = 0; i < N; ++i) {
        float fi = float(i);
        float aa = t * 0.42 + fi * 1.047 + lfo * uMovement * 1.8;
        float rr = 0.28 + 0.22 * sin(t * 0.38 + fi * 1.7)
                 + 0.18 * uLevel + uSpread * 0.25;
        vec2  pp = vec2(cos(aa), sin(aa * 1.07 + fi)) * rr;
        float r  = length(wuv - pp);
        float w  = 0.11 + 0.07 * uCharacter + 0.05 * uDetune + 0.03 * uReverbSize;
        field += w / (r * r + 0.02);
    }

    // Delay echoes : ghost blobs trailing main mass
    float echoAmt = uDelayMix * (0.5 + uDelayFb);
    for (int i = 1; i < 4; ++i) {
        float fi = float(i);
        float ea = t * 0.3 - fi * (0.5 + uDelayTime * 1.3);
        vec2  ep = vec2(cos(ea), sin(ea * 0.9)) * (0.42 + uDelayTime * 0.2);
        float r  = length(wuv - ep);
        float w  = echoAmt * 0.08 / fi;
        field += w / (r * r + 0.035);
    }

    // Noise grain : granular turbulence added to the field
    float grain = hash21(floor(uv * 180.0) + floor(t * 24.0));
    field += (grain - 0.5) * 0.22 * uNoise;

    // ============================================================
    // INK TEXTURE (inside the blob, like the reference image)
    // ============================================================
    // Low-freq density swirls
    vec2 tuv = wuv * 2.2 + vec2(t * 0.08, -t * 0.06);
    float inkLo = fbm(tuv, 4);

    // Mid-freq : warped by the low-freq noise (looks "inky")
    float inkMi = fbm(tuv * 2.6 + inkLo * 1.2 + vec2(-t*0.05, t*0.04), 3);

    // Hi-freq : Noise param adds grain
    float inkHi = fbm(tuv * 9.0 + vec2(t*0.3, -t*0.25), 2);

    // Combined internal density : 0 (light) .. 1 (dark ink)
    float density = clamp(0.25 + inkLo * 1.1 + inkMi * 0.6 - inkHi * 0.25, 0.0, 1.0);

    // Release / Attack breath : slow internal pulsation of density
    float breath = sin(t * (0.25 + uAttack * 0.7)) * 0.5 + 0.5;
    density *= mix(1.0, 0.75 + 0.35 * breath, uRelease * 0.6);

    // ============================================================
    // EDGE / RIM
    // ============================================================
    float edge    = smoothstep(0.85, 1.10, field);
    float rim     = exp(-pow(field - 0.97, 2.0) * 26.0);
    float haloOut = smoothstep(0.95, 0.35, field);   // outside diffusion

    // ============================================================
    // COLOURS (palette with per-param tint direction)
    // ============================================================
    vec3 ink     = vec3(0.010, 0.013, 0.020);
    vec3 inkMid  = vec3(0.090, 0.095, 0.110);        // lighter grey ink
    vec3 gold    = vec3(0.80, 0.72, 0.46);
    vec3 violet  = vec3(0.55, 0.42, 0.88);
    vec3 teal    = vec3(0.30, 0.72, 0.82);
    vec3 amber   = vec3(0.95, 0.55, 0.25);
    vec3 cyan    = vec3(0.25, 0.85, 0.95);
    vec3 magenta = vec3(0.88, 0.28, 0.58);

    // Accumulate tint based on different effects
    vec3 tint = gold;
    tint = mix(tint, violet,  clamp(uShimmer,              0.0, 1.0));
    tint = mix(tint, teal,    clamp(uMovement   * 0.55,    0.0, 1.0));
    tint = mix(tint, amber,   clamp(uDrive      * 0.65,    0.0, 1.0));
    tint = mix(tint, cyan,    clamp(uChorus     * 0.45,    0.0, 1.0));
    tint = mix(tint, magenta, clamp(uReso       * 0.40,    0.0, 1.0));

    // Tone : warm/cool global cast
    vec3 warmCool = mix(vec3(0.85, 0.90, 1.05), vec3(1.08, 0.96, 0.82), uTone);

    // ============================================================
    // BACKGROUND (bowl)
    // ============================================================
    float bowl = smoothstep(1.35, 0.0, length(uv));
    vec3 bg = mix(vec3(0.045, 0.050, 0.062),
                  vec3(0.085, 0.085, 0.096), bowl);
    // Reverb Mix bleeds a faint halo wash into the bg
    bg += haloOut * tint * uReverbMix * 0.14;

    // ============================================================
    // INK FILL (the textured interior)
    // ============================================================
    // Base ink colour modulated by density (black <-> mid grey-ink)
    vec3 inside = mix(inkMid, ink, density);

    // Inject subtle coloured veins from the mid-freq FBM
    inside += tint * inkMi * inkMi * 0.18;

    // Character gives a pronounced ink "body" (mix toward pure ink)
    inside = mix(inside, ink, uCharacter * 0.45);

    // Sub : a deeper dark pulsing core
    float subPulse = 0.5 + 0.5 * sin(t * 1.2);
    inside = mix(inside, ink * 0.6,
                 uSub * smoothstep(0.8, 0.0, length(wuv)) * (0.6 + 0.4*subPulse));

    // Noise param scatters bright micro-speckles inside
    float speck = step(0.985, hash21(floor(uv * 420.0) + floor(t * 18.0)));
    inside += vec3(speck) * uNoise * 0.35;

    // ============================================================
    // FINAL COMPOSITE
    // ============================================================
    vec3 col = bg;

    // Ink interior with texture
    col = mix(col, inside, edge);

    // Rim glow : audio level + Gain amplify it
    float rimIntensity = 0.18 + uLevel * 1.8 + uGain * 0.15;
    col += tint * rim * rimIntensity;

    // Resonance chromatic fringe on the rim
    col.r += rim * uReso * 0.35;
    col.b += rim * uReso * 0.20;

    // Diffusion bloom around blob (Reverb Mix)
    col += tint * haloOut * uReverbMix * 0.35;

    // Tone cast
    col *= warmCool;

    // Overall gain influences brightness floor subtly
    col *= (0.82 + uGain * 0.3);

    // Vignette
    float v = smoothstep(1.8, 0.15, length(uv));
    col *= mix(0.40, 1.0, v);

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
    juce::gl::glGenVertexArrays (1, &vao);

    uRes.create       (*shader, "uRes");
    uTime.create      (*shader, "uTime");
    uLevel.create     (*shader, "uLevel");
    uLfo.create       (*shader, "uLfo");

    uCharacter.create (*shader, "uCharacter");
    uDetune.create    (*shader, "uDetune");
    uSub.create       (*shader, "uSub");
    uNoise.create     (*shader, "uNoise");

    uCutoff.create    (*shader, "uCutoff");
    uReso.create      (*shader, "uReso");
    uDrive.create     (*shader, "uDrive");
    uTone.create      (*shader, "uTone");

    uAttack.create    (*shader, "uAttack");
    uRelease.create   (*shader, "uRelease");
    uGlide.create     (*shader, "uGlide");
    uSpread.create    (*shader, "uSpread");
    uMovement.create  (*shader, "uMovement");

    uChorus.create    (*shader, "uChorus");
    uShimmer.create   (*shader, "uShimmer");

    uReverbSize.create(*shader, "uReverbSize");
    uReverbMix.create (*shader, "uReverbMix");

    uDelayTime.create (*shader, "uDelayTime");
    uDelayFb.create   (*shader, "uDelayFb");
    uDelayMix.create  (*shader, "uDelayMix");

    uGain.create      (*shader, "uGain");

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
    auto set = [] (U& u, float a) { if (u.ptr) u.ptr->set (a); };

    if (uRes.ptr)  uRes.ptr->set ((float) w, (float) h);
    if (uTime.ptr) uTime.ptr->set ((float) now);

    set (uLevel,     juce::jlimit (0.0f, 1.5f, proc.uiAudioLevel.load()));
    set (uLfo,       proc.uiLfoPhase.load());

    set (uCharacter, readParam ("character"));
    set (uDetune,    readParam ("detune"));
    set (uSub,       readParam ("sub"));
    set (uNoise,     readParam ("noise"));

    set (uCutoff,    readParam ("cutoff"));
    set (uReso,      readParam ("resonance"));
    set (uDrive,     readParam ("drive"));
    set (uTone,      readParam ("tone"));

    set (uAttack,    readParam ("attack"));
    set (uRelease,   readParam ("release"));
    set (uGlide,     readParam ("glide"));
    set (uSpread,    readParam ("spread"));
    set (uMovement,  readParam ("movement"));

    set (uChorus,    readParam ("chorus"));
    set (uShimmer,   readParam ("shimmer"));

    set (uReverbSize,readParam ("reverbSize"));
    set (uReverbMix, readParam ("reverbMix"));

    set (uDelayTime, readParam ("delayTime"));
    set (uDelayFb,   readParam ("delayFb"));
    set (uDelayMix,  readParam ("delayMix"));

    set (uGain,      readParam ("gain"));

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
    uRes.ptr.reset(); uTime.ptr.reset(); uLevel.ptr.reset(); uLfo.ptr.reset();
    uCharacter.ptr.reset(); uDetune.ptr.reset(); uSub.ptr.reset(); uNoise.ptr.reset();
    uCutoff.ptr.reset(); uReso.ptr.reset(); uDrive.ptr.reset(); uTone.ptr.reset();
    uAttack.ptr.reset(); uRelease.ptr.reset(); uGlide.ptr.reset();
    uSpread.ptr.reset(); uMovement.ptr.reset();
    uChorus.ptr.reset(); uShimmer.ptr.reset();
    uReverbSize.ptr.reset(); uReverbMix.ptr.reset();
    uDelayTime.ptr.reset(); uDelayFb.ptr.reset(); uDelayMix.ptr.reset();
    uGain.ptr.reset();
    shaderReady = false;
}
