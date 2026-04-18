# OctoPad

Ambient & mélodique pad synthesizer — VST3 / Standalone (JUCE 8).
Membre de la famille **Octopus Audio** (sibling d'Octopus, Larcene, OctoDrums).

## Philosophie

Un pad modulaire, pensé pour évoluer du drone profond jusqu'au shimmer céleste.
Chaque potar a été choisi pour vriller / texturer / sculpter le son.

## Architecture DSP

```
[3 oscs détunés + sub + noise]
          ↓
  [SVF lowpass + drive]
          ↓
    [ADSR long (pad)]
          ↓
  [chorus stéréo large]
          ↓
  [shimmer +1 octave]
          ↓
  [ping-pong delay]
          ↓
     [reverb hall]
          ↓
     [tilt EQ (tone)]
          ↓
     [limiter + out]
```

## 22 paramètres (potars)

| Section | Paramètre | Effet |
|---------|-----------|-------|
| Source  | Character | Saw → Square → Sine blend |
|         | Detune    | Supersaw spread |
|         | Sub       | Oscillateur -1 octave |
|         | Texture   | Noise / air |
|         | Spread    | Largeur stéréo des voix |
| Filter  | Cutoff    | 60 Hz → 12 kHz (log) |
|         | Resonance | 0.3 → 6.0 Q |
|         | Drive     | Pre-filter soft clip |
|         | Tone      | Tilt EQ (dark → bright) |
| Shape   | Attack    | 10 ms → 4 s |
|         | Release   | 100 ms → 8 s |
|         | Glide     | Portamento |
|         | Movement  | LFO → cutoff + pitch |
|         | Motion    | 0.05 → 4 Hz |
| Space   | Chorus    | Ensemble stéréo |
|         | Shimmer   | Octave-up granulaire |
|         | Dly Time  | 40 → 900 ms |
|         | Dly Fb    | Feedback ping-pong |
|         | Dly Mix   | Dry/wet |
|         | Rvb Size  | Roomsize |
|         | Rvb Mix   | Dry/wet |
|         | Gain      | Sortie |

## 20 presets factory

1. **Aurora** — voile brillant, shimmer léger
2. **Deep Space** — drone profond, reverb énorme
3. **Glass Tower** — cristallin, shimmer +12
4. **Warm Hug** — analogique chaleureux
5. **Ice Field** — cristal aigu, chorus large
6. **Ocean Drone** — sub massif, mouvement lent
7. **Celestial** — shimmer infini
8. **Nebula** — granulaire / texturé
9. **Whisper Choir** — formant vocal
10. **Velvet** — doux, velouté
11. **Frozen Lake** — statique, LFO très lent
12. **Phantom** — sombre, filtre modulé
13. **Silk Road** — mélodique, détune large
14. **Ghost Strings** — attack rapide, string-like
15. **Morning Mist** — éthéré léger
16. **Echo Valley** — delay ping-pong prononcé
17. **Lunar** — mystérieux, mineur
18. **Solar Winds** — brillant, motion
19. **Temple** — cathédrale sacrée
20. **Dreamweaver** — multicouche complexe

## Build

```bash
cmake -B build
cmake --build build --parallel
```

Artéfacts :
- `build/OctoPad_artefacts/VST3/OctoPad.vst3`
- `build/OctoPad_artefacts/Standalone/OctoPad`

## Esthétique UI

- Fond anthracite `#12141A`
- Panel `#191C24`
- Texte ivoire `#F2F0E6`
- Accent champagne `#C9B87A`
- Typo : Inter (fallback système)

4 sections verticales (Source / Filter / Shape / Space), knobs minimalistes avec arc doré, texte dimmed sous chaque potar.
