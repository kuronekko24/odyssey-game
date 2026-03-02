using UnityEngine;

namespace Odyssey.Audio
{
    // =========================================================================
    // ProceduralAudio -- runtime AudioClip generation using basic waveforms
    // =========================================================================

    /// <summary>
    /// Generates chiptune-style AudioClips at runtime.
    /// Uses AudioClip.Create + SetData with float[] sample buffers.
    /// All waveform math is self-contained -- no external dependencies.
    /// </summary>
    public static class ProceduralAudio
    {
        public const int SampleRate = 44100;

        // --- Waveform generators (return -1..1) ---

        public static float Square(float phase)
        {
            return phase % 1f < 0.5f ? 0.8f : -0.8f;
        }

        public static float Triangle(float phase)
        {
            float p = phase % 1f;
            return p < 0.5f
                ? 4f * p - 1f
                : 3f - 4f * p;
        }

        public static float Sawtooth(float phase)
        {
            return 2f * (phase % 1f) - 1f;
        }

        public static float Noise()
        {
            return UnityEngine.Random.Range(-1f, 1f);
        }

        public static float Sine(float phase)
        {
            return Mathf.Sin(phase * 2f * Mathf.PI);
        }

        // --- ADSR Envelope ---

        /// <summary>
        /// Returns envelope amplitude (0-1) for a given time within an ADSR envelope.
        /// </summary>
        public static float Envelope(float time, float attack, float decay, float sustain, float release, float totalDuration)
        {
            if (time < 0f) return 0f;
            if (time < attack)
                return time / attack;
            if (time < attack + decay)
                return 1f - (1f - sustain) * ((time - attack) / decay);

            float releaseStart = totalDuration - release;
            if (time >= releaseStart)
            {
                float t = (time - releaseStart) / release;
                return sustain * (1f - Mathf.Clamp01(t));
            }
            return sustain;
        }

        // --- Clip builders ---

        /// <summary>
        /// Create a tone with frequency sweep and ADSR envelope.
        /// waveType: 0=square, 1=triangle, 2=sawtooth, 3=noise, 4=sine
        /// </summary>
        public static AudioClip CreateTone(
            string name, float durationSec, float startFreq, float endFreq,
            int waveType = 0,
            float attack = 0.005f, float decay = 0.05f, float sustain = 0.6f, float release = 0.05f,
            float volume = 0.5f)
        {
            int sampleCount = Mathf.CeilToInt(durationSec * SampleRate);
            float[] data = new float[sampleCount];
            float phase = 0f;

            for (int i = 0; i < sampleCount; i++)
            {
                float t = (float)i / SampleRate;
                float progress = (float)i / sampleCount;
                float freq = Mathf.Lerp(startFreq, endFreq, progress);

                float sample = waveType switch
                {
                    0 => Square(phase),
                    1 => Triangle(phase),
                    2 => Sawtooth(phase),
                    3 => Noise(),
                    4 => Sine(phase),
                    _ => Square(phase),
                };

                float env = Envelope(t, attack, decay, sustain, release, durationSec);
                data[i] = sample * env * volume;

                phase += freq / SampleRate;
            }

            return CreateClipFromData(name, data);
        }

        /// <summary>
        /// Create a noise burst with envelope (explosions, impacts).
        /// </summary>
        public static AudioClip CreateNoiseBurst(
            string name, float durationSec,
            float startFreqFilter = 8000f, float endFreqFilter = 200f,
            float attack = 0.001f, float decay = 0.1f, float sustain = 0.3f, float release = 0.1f,
            float volume = 0.5f)
        {
            int sampleCount = Mathf.CeilToInt(durationSec * SampleRate);
            float[] data = new float[sampleCount];
            float prevSample = 0f;

            for (int i = 0; i < sampleCount; i++)
            {
                float t = (float)i / SampleRate;
                float progress = (float)i / sampleCount;

                // Simple low-pass filter: mix noise with previous sample
                float filterFreq = Mathf.Lerp(startFreqFilter, endFreqFilter, progress);
                float rc = 1f / (2f * Mathf.PI * filterFreq);
                float dt = 1f / SampleRate;
                float alpha = dt / (rc + dt);

                float raw = Noise();
                float filtered = prevSample + alpha * (raw - prevSample);
                prevSample = filtered;

                float env = Envelope(t, attack, decay, sustain, release, durationSec);
                data[i] = filtered * env * volume;
            }

            return CreateClipFromData(name, data);
        }

        /// <summary>
        /// Create a multi-note arpeggio from an array of frequencies.
        /// Each note plays for noteDuration seconds with the given waveType.
        /// </summary>
        public static AudioClip CreateArpeggio(
            string name, float[] frequencies, float noteDuration,
            int waveType = 0, float volume = 0.4f)
        {
            float totalDuration = frequencies.Length * noteDuration;
            int sampleCount = Mathf.CeilToInt(totalDuration * SampleRate);
            float[] data = new float[sampleCount];
            float phase = 0f;

            for (int i = 0; i < sampleCount; i++)
            {
                float t = (float)i / SampleRate;
                int noteIndex = Mathf.Min((int)(t / noteDuration), frequencies.Length - 1);
                float freq = frequencies[noteIndex];

                float noteTime = t - noteIndex * noteDuration;
                float env = Envelope(noteTime, 0.005f, 0.02f, 0.7f, 0.02f, noteDuration);

                float sample = waveType switch
                {
                    0 => Square(phase),
                    1 => Triangle(phase),
                    2 => Sawtooth(phase),
                    4 => Sine(phase),
                    _ => Square(phase),
                };

                data[i] = sample * env * volume;
                phase += freq / SampleRate;
            }

            return CreateClipFromData(name, data);
        }

        /// <summary>
        /// Create a two-tone blip (for notification pops, etc.).
        /// </summary>
        public static AudioClip CreateTwoTone(
            string name, float freq1, float freq2,
            float toneDuration = 0.05f, int waveType = 0, float volume = 0.4f)
        {
            float totalDuration = toneDuration * 2f;
            int sampleCount = Mathf.CeilToInt(totalDuration * SampleRate);
            float[] data = new float[sampleCount];
            float phase = 0f;

            for (int i = 0; i < sampleCount; i++)
            {
                float t = (float)i / SampleRate;
                float freq = t < toneDuration ? freq1 : freq2;

                float localTime = t < toneDuration ? t : t - toneDuration;
                float env = Envelope(localTime, 0.003f, 0.01f, 0.7f, 0.01f, toneDuration);

                float sample = waveType switch
                {
                    0 => Square(phase),
                    1 => Triangle(phase),
                    _ => Square(phase),
                };

                data[i] = sample * env * volume;
                phase += freq / SampleRate;
            }

            return CreateClipFromData(name, data);
        }

        /// <summary>
        /// Create a looping ambient texture (filtered noise + optional tone).
        /// </summary>
        public static AudioClip CreateAmbientLoop(
            string name, float durationSec, float filterFreq,
            float toneFreq = 0f, int toneWave = 1, float volume = 0.15f)
        {
            int sampleCount = Mathf.CeilToInt(durationSec * SampleRate);
            float[] data = new float[sampleCount];
            float prevSample = 0f;
            float tonePhase = 0f;

            float rc = 1f / (2f * Mathf.PI * filterFreq);
            float dt = 1f / SampleRate;
            float alpha = dt / (rc + dt);

            for (int i = 0; i < sampleCount; i++)
            {
                float raw = Noise();
                float filtered = prevSample + alpha * (raw - prevSample);
                prevSample = filtered;

                float sample = filtered * volume;

                if (toneFreq > 0f)
                {
                    float tone = toneWave switch
                    {
                        0 => Square(tonePhase),
                        1 => Triangle(tonePhase),
                        4 => Sine(tonePhase),
                        _ => Triangle(tonePhase),
                    };
                    sample += tone * volume * 0.3f;
                    tonePhase += toneFreq / SampleRate;
                }

                // Smooth loop: fade in first 1000 samples, fade out last 1000 samples
                int fadeSamples = Mathf.Min(1000, sampleCount / 4);
                if (i < fadeSamples)
                    sample *= (float)i / fadeSamples;
                else if (i > sampleCount - fadeSamples)
                    sample *= (float)(sampleCount - i) / fadeSamples;

                data[i] = sample;
            }

            return CreateClipFromData(name, data);
        }

        /// <summary>
        /// Generate a chiptune music loop with melody, bass, and drums layers.
        /// </summary>
        public static AudioClip CreateMusicLoop(
            string name,
            float bpm,
            float[] melodyNotes,     // frequencies, 0 = rest
            float[] bassNotes,       // frequencies, 0 = rest
            bool[] drumHits,         // true = hit on this step
            int stepsPerBeat,
            int melodyWave = 0,
            int bassWave = 1,
            float melodyVol = 0.25f,
            float bassVol = 0.20f,
            float drumVol = 0.15f)
        {
            float stepDuration = 60f / bpm / stepsPerBeat;
            int totalSteps = Mathf.Max(melodyNotes.Length, Mathf.Max(bassNotes.Length, drumHits.Length));
            float totalDuration = totalSteps * stepDuration;
            int sampleCount = Mathf.CeilToInt(totalDuration * SampleRate);
            float[] data = new float[sampleCount];

            float melodyPhase = 0f;
            float bassPhase = 0f;

            // Pre-generate a short drum hit (noise burst)
            int drumLength = Mathf.CeilToInt(0.04f * SampleRate);
            float[] drumSamples = new float[drumLength];
            float drumPrev = 0f;
            for (int d = 0; d < drumLength; d++)
            {
                float raw = Noise();
                float filt = drumPrev + 0.3f * (raw - drumPrev);
                drumPrev = filt;
                float env = 1f - (float)d / drumLength;
                drumSamples[d] = filt * env;
            }

            for (int i = 0; i < sampleCount; i++)
            {
                float t = (float)i / SampleRate;
                int step = Mathf.Min((int)(t / stepDuration), totalSteps - 1);
                float stepTime = t - step * stepDuration;

                float sample = 0f;

                // Melody
                if (step < melodyNotes.Length && melodyNotes[step] > 0f)
                {
                    float mEnv = Envelope(stepTime, 0.005f, 0.03f, 0.6f, 0.02f, stepDuration);
                    float mSample = melodyWave switch
                    {
                        0 => Square(melodyPhase),
                        1 => Triangle(melodyPhase),
                        2 => Sawtooth(melodyPhase),
                        4 => Sine(melodyPhase),
                        _ => Square(melodyPhase),
                    };
                    sample += mSample * mEnv * melodyVol;
                    melodyPhase += melodyNotes[step] / SampleRate;
                }

                // Bass
                if (step < bassNotes.Length && bassNotes[step] > 0f)
                {
                    float bEnv = Envelope(stepTime, 0.005f, 0.05f, 0.5f, 0.03f, stepDuration);
                    float bSample = bassWave switch
                    {
                        0 => Square(bassPhase),
                        1 => Triangle(bassPhase),
                        2 => Sawtooth(bassPhase),
                        4 => Sine(bassPhase),
                        _ => Triangle(bassPhase),
                    };
                    sample += bSample * bEnv * bassVol;
                    bassPhase += bassNotes[step] / SampleRate;
                }

                // Drums
                if (step < drumHits.Length && drumHits[step])
                {
                    int drumIndex = (int)(stepTime * SampleRate);
                    if (drumIndex >= 0 && drumIndex < drumLength)
                        sample += drumSamples[drumIndex] * drumVol;
                }

                // Soft clip to prevent distortion
                data[i] = Mathf.Clamp(sample, -0.9f, 0.9f);
            }

            // Smooth loop boundaries
            int fade = Mathf.Min(2000, sampleCount / 8);
            for (int i = 0; i < fade; i++)
            {
                float f = (float)i / fade;
                data[i] *= f;
                data[sampleCount - 1 - i] *= f;
            }

            return CreateClipFromData(name, data);
        }

        // --- Internal helpers ---

        private static AudioClip CreateClipFromData(string name, float[] data)
        {
            var clip = AudioClip.Create(name, data.Length, 1, SampleRate, false);
            clip.SetData(data, 0);
            return clip;
        }

        // --- Musical note frequencies (for convenience) ---

        public static float NoteFreq(int midiNote)
        {
            // A4 = MIDI 69 = 440 Hz
            return 440f * Mathf.Pow(2f, (midiNote - 69) / 12f);
        }

        // Common note names as MIDI numbers
        public const int C3 = 48, D3 = 50, E3 = 52, F3 = 53, G3 = 55, A3 = 57, B3 = 59;
        public const int C4 = 60, D4 = 62, E4 = 64, F4 = 65, G4 = 67, A4 = 69, B4 = 71;
        public const int C5 = 72, D5 = 74, E5 = 76, F5 = 77, G5 = 79, A5 = 81, B5 = 83;
        public const int C6 = 84;

        // Sharps / flats
        public const int Cs3 = 49, Eb3 = 51, Fs3 = 54, Ab3 = 56, Bb3 = 58;
        public const int Cs4 = 61, Eb4 = 63, Fs4 = 66, Ab4 = 68, Bb4 = 70;
        public const int Cs5 = 73, Eb5 = 75, Fs5 = 78, Ab5 = 80, Bb5 = 82;
    }

    // =========================================================================
    // SFXLibrary -- all game sound effects defined with procedural parameters
    // =========================================================================

    /// <summary>
    /// Static library of all game sound effects.
    /// Clips are lazily generated on first access and cached.
    /// Organised by category matching the GDD audio design document.
    /// </summary>
    public static class SFXLibrary
    {
        // =====================================================================
        // UI Sounds
        // =====================================================================

        private static AudioClip _buttonClick;
        /// <summary>Short square blip, 800Hz, 50ms -- chunky mechanical keyboard feel.</summary>
        public static AudioClip ButtonClick => _buttonClick ??= ProceduralAudio.CreateTone(
            "UI_ButtonClick", 0.05f, 800f, 780f,
            waveType: 0, attack: 0.002f, decay: 0.01f, sustain: 0.5f, release: 0.01f, volume: 0.4f);

        private static AudioClip _panelOpen;
        /// <summary>Sweep up 200->600Hz, 100ms -- ascending 3-note blip matching slide-in.</summary>
        public static AudioClip PanelOpen => _panelOpen ??= ProceduralAudio.CreateTone(
            "UI_PanelOpen", 0.1f, 200f, 600f,
            waveType: 0, attack: 0.003f, decay: 0.02f, sustain: 0.6f, release: 0.02f, volume: 0.35f);

        private static AudioClip _panelClose;
        /// <summary>Sweep down 600->200Hz, 100ms -- descending 2-note blip matching slide-out.</summary>
        public static AudioClip PanelClose => _panelClose ??= ProceduralAudio.CreateTone(
            "UI_PanelClose", 0.1f, 600f, 200f,
            waveType: 0, attack: 0.003f, decay: 0.02f, sustain: 0.6f, release: 0.02f, volume: 0.35f);

        private static AudioClip _notificationPop;
        /// <summary>Two-tone 400->800Hz -- bright pop for toast notifications.</summary>
        public static AudioClip NotificationPop => _notificationPop ??= ProceduralAudio.CreateTwoTone(
            "UI_NotificationPop", 400f, 800f,
            toneDuration: 0.04f, waveType: 0, volume: 0.35f);

        private static AudioClip _tabSwitch;
        /// <summary>Light tick -- subtle tab switch sound.</summary>
        public static AudioClip TabSwitch => _tabSwitch ??= ProceduralAudio.CreateTone(
            "UI_TabSwitch", 0.03f, 1000f, 980f,
            waveType: 0, attack: 0.001f, decay: 0.005f, sustain: 0.3f, release: 0.005f, volume: 0.25f);

        private static AudioClip _errorReject;
        /// <summary>Low buzz + dull tone -- "nope" without frustration.</summary>
        public static AudioClip ErrorReject => _errorReject ??= ProceduralAudio.CreateTone(
            "UI_ErrorReject", 0.15f, 180f, 120f,
            waveType: 0, attack: 0.003f, decay: 0.03f, sustain: 0.4f, release: 0.04f, volume: 0.35f);

        private static AudioClip _omenReceived;
        /// <summary>Coin-like clink with warm resonance -- satisfying money sound.</summary>
        public static AudioClip OmenReceived => _omenReceived ??= ProceduralAudio.CreateTone(
            "UI_OmenReceived", 0.2f, 1200f, 1400f,
            waveType: 1, attack: 0.002f, decay: 0.05f, sustain: 0.3f, release: 0.08f, volume: 0.35f);

        private static AudioClip _levelUp;
        /// <summary>4-bar fanfare arpeggio -- the most rewarding sound in the game.</summary>
        public static AudioClip LevelUp => _levelUp ??= ProceduralAudio.CreateArpeggio(
            "UI_LevelUp",
            new float[]
            {
                ProceduralAudio.NoteFreq(ProceduralAudio.C5),
                ProceduralAudio.NoteFreq(ProceduralAudio.E5),
                ProceduralAudio.NoteFreq(ProceduralAudio.G5),
                ProceduralAudio.NoteFreq(ProceduralAudio.C6),
                ProceduralAudio.NoteFreq(ProceduralAudio.E5),
                ProceduralAudio.NoteFreq(ProceduralAudio.G5),
                ProceduralAudio.NoteFreq(ProceduralAudio.C6),
                ProceduralAudio.NoteFreq(ProceduralAudio.E5),
            },
            noteDuration: 0.12f, waveType: 0, volume: 0.35f);

        private static AudioClip _questComplete;
        /// <summary>Triumphant stinger -- brass-style chiptune.</summary>
        public static AudioClip QuestComplete => _questComplete ??= ProceduralAudio.CreateArpeggio(
            "UI_QuestComplete",
            new float[]
            {
                ProceduralAudio.NoteFreq(ProceduralAudio.G4),
                ProceduralAudio.NoteFreq(ProceduralAudio.B4),
                ProceduralAudio.NoteFreq(ProceduralAudio.D5),
                ProceduralAudio.NoteFreq(ProceduralAudio.G5),
                0f,
                ProceduralAudio.NoteFreq(ProceduralAudio.G5),
            },
            noteDuration: 0.15f, waveType: 2, volume: 0.3f);

        // =====================================================================
        // Combat Sounds
        // =====================================================================

        private static AudioClip _laserFire;
        /// <summary>Square 1200Hz sweep down to 400Hz, 150ms -- bright zap.</summary>
        public static AudioClip LaserFire => _laserFire ??= ProceduralAudio.CreateTone(
            "Combat_LaserFire", 0.15f, 1200f, 400f,
            waveType: 0, attack: 0.002f, decay: 0.03f, sustain: 0.5f, release: 0.03f, volume: 0.45f);

        private static AudioClip _missileLaunch;
        /// <summary>Noise burst + low freq sweep, 300ms -- whoosh launch.</summary>
        public static AudioClip MissileLaunch => _missileLaunch ??= ProceduralAudio.CreateNoiseBurst(
            "Combat_MissileLaunch", 0.3f, 6000f, 300f,
            attack: 0.002f, decay: 0.05f, sustain: 0.4f, release: 0.1f, volume: 0.45f);

        private static AudioClip _railgunFire;
        /// <summary>Noise + square 2000Hz, 100ms + echo feel -- the most satisfying crack.</summary>
        public static AudioClip RailgunFire => _railgunFire ??= ProceduralAudio.CreateTone(
            "Combat_RailgunFire", 0.12f, 2000f, 1600f,
            waveType: 0, attack: 0.001f, decay: 0.02f, sustain: 0.7f, release: 0.02f, volume: 0.5f);

        private static AudioClip _cannonFire;
        /// <summary>Deep punchy thud with metallic ring -- low frequency heavy hit.</summary>
        public static AudioClip CannonFire => _cannonFire ??= ProceduralAudio.CreateTone(
            "Combat_CannonFire", 0.2f, 120f, 60f,
            waveType: 2, attack: 0.001f, decay: 0.04f, sustain: 0.3f, release: 0.08f, volume: 0.5f);

        private static AudioClip _hitImpact;
        /// <summary>Noise burst 50ms -- short punchy impact.</summary>
        public static AudioClip HitImpact => _hitImpact ??= ProceduralAudio.CreateNoiseBurst(
            "Combat_HitImpact", 0.05f, 5000f, 800f,
            attack: 0.001f, decay: 0.01f, sustain: 0.5f, release: 0.01f, volume: 0.5f);

        private static AudioClip _shieldHit;
        /// <summary>Filtered noise + high tone, 100ms -- electric sizzle.</summary>
        public static AudioClip ShieldHit => _shieldHit ??= ProceduralAudio.CreateTone(
            "Combat_ShieldHit", 0.1f, 2400f, 1800f,
            waveType: 1, attack: 0.001f, decay: 0.02f, sustain: 0.4f, release: 0.03f, volume: 0.4f);

        private static AudioClip _armorHit;
        /// <summary>Metallic clang + crunch -- heavier than shield hits.</summary>
        public static AudioClip ArmorHit => _armorHit ??= ProceduralAudio.CreateTone(
            "Combat_ArmorHit", 0.08f, 600f, 200f,
            waveType: 2, attack: 0.001f, decay: 0.02f, sustain: 0.5f, release: 0.02f, volume: 0.45f);

        private static AudioClip _explosion;
        /// <summary>Noise sweep down 1000->50Hz, 500ms -- bass-heavy boom.</summary>
        public static AudioClip Explosion => _explosion ??= ProceduralAudio.CreateNoiseBurst(
            "Combat_Explosion", 0.5f, 1000f, 50f,
            attack: 0.002f, decay: 0.1f, sustain: 0.4f, release: 0.2f, volume: 0.55f);

        private static AudioClip _deathExplosion;
        /// <summary>Long noise + frequency drop, 1000ms -- ship destruction.</summary>
        public static AudioClip DeathExplosion => _deathExplosion ??= ProceduralAudio.CreateNoiseBurst(
            "Combat_DeathExplosion", 1.0f, 2000f, 30f,
            attack: 0.003f, decay: 0.15f, sustain: 0.35f, release: 0.4f, volume: 0.55f);

        private static AudioClip _shieldBreak;
        /// <summary>Descending electric whine + pop -- shields gone.</summary>
        public static AudioClip ShieldBreak => _shieldBreak ??= ProceduralAudio.CreateTone(
            "Combat_ShieldBreak", 0.25f, 3000f, 200f,
            waveType: 0, attack: 0.002f, decay: 0.05f, sustain: 0.4f, release: 0.1f, volume: 0.45f);

        private static AudioClip _empDisruptor;
        /// <summary>Electrical crackle + bass drop -- systems failing.</summary>
        public static AudioClip EmpDisruptor => _empDisruptor ??= ProceduralAudio.CreateNoiseBurst(
            "Combat_EMP", 0.3f, 8000f, 100f,
            attack: 0.001f, decay: 0.05f, sustain: 0.3f, release: 0.15f, volume: 0.4f);

        // =====================================================================
        // Mining Sounds
        // =====================================================================

        private static AudioClip _miningTick;
        /// <summary>Low square pulse, 200Hz, 30ms -- repeating extraction tick.</summary>
        public static AudioClip MiningTick => _miningTick ??= ProceduralAudio.CreateTone(
            "Mining_Tick", 0.03f, 200f, 190f,
            waveType: 0, attack: 0.002f, decay: 0.005f, sustain: 0.6f, release: 0.005f, volume: 0.35f);

        private static AudioClip _miningComplete;
        /// <summary>Ascending arpeggio 400->600->800Hz -- extraction complete.</summary>
        public static AudioClip MiningComplete => _miningComplete ??= ProceduralAudio.CreateArpeggio(
            "Mining_Complete",
            new float[] { 400f, 600f, 800f },
            noteDuration: 0.1f, waveType: 0, volume: 0.4f);

        private static AudioClip _nodeDepleted;
        /// <summary>Descending tone 600->200Hz, 200ms -- node exhausted.</summary>
        public static AudioClip NodeDepleted => _nodeDepleted ??= ProceduralAudio.CreateTone(
            "Mining_NodeDepleted", 0.2f, 600f, 200f,
            waveType: 1, attack: 0.005f, decay: 0.03f, sustain: 0.5f, release: 0.05f, volume: 0.35f);

        private static AudioClip _craftStart;
        /// <summary>Mechanical whir + click -- materials loading in.</summary>
        public static AudioClip CraftStart => _craftStart ??= ProceduralAudio.CreateTone(
            "Mining_CraftStart", 0.15f, 300f, 500f,
            waveType: 2, attack: 0.005f, decay: 0.03f, sustain: 0.4f, release: 0.03f, volume: 0.3f);

        private static AudioClip _craftComplete;
        /// <summary>Anvil ring + sparkle chime + bass thump -- triumphant crafting.</summary>
        public static AudioClip CraftComplete => _craftComplete ??= ProceduralAudio.CreateArpeggio(
            "Mining_CraftComplete",
            new float[]
            {
                ProceduralAudio.NoteFreq(ProceduralAudio.E4),
                ProceduralAudio.NoteFreq(ProceduralAudio.G4),
                ProceduralAudio.NoteFreq(ProceduralAudio.B4),
                ProceduralAudio.NoteFreq(ProceduralAudio.E5),
            },
            noteDuration: 0.1f, waveType: 1, volume: 0.4f);

        // =====================================================================
        // Ship Sounds
        // =====================================================================

        private static AudioClip _thrusterHum;
        /// <summary>Low freq triangle loop, 80Hz -- continuous engine hum.</summary>
        public static AudioClip ThrusterHum => _thrusterHum ??= ProceduralAudio.CreateTone(
            "Ship_ThrusterHum", 1.0f, 80f, 82f,
            waveType: 1, attack: 0.1f, decay: 0.1f, sustain: 0.5f, release: 0.2f, volume: 0.2f);

        private static AudioClip _boostActivate;
        /// <summary>Sweep up 100->400Hz, 200ms -- afterburner engage.</summary>
        public static AudioClip BoostActivate => _boostActivate ??= ProceduralAudio.CreateTone(
            "Ship_BoostActivate", 0.2f, 100f, 400f,
            waveType: 1, attack: 0.005f, decay: 0.03f, sustain: 0.6f, release: 0.05f, volume: 0.4f);

        private static AudioClip _dockingClamp;
        /// <summary>Metallic click, noise+square 1000Hz, 50ms -- clamp lock.</summary>
        public static AudioClip DockingClamp => _dockingClamp ??= ProceduralAudio.CreateTone(
            "Ship_DockingClamp", 0.05f, 1000f, 800f,
            waveType: 0, attack: 0.001f, decay: 0.01f, sustain: 0.6f, release: 0.01f, volume: 0.45f);

        private static AudioClip _warpJump;
        /// <summary>3-second swoosh with rising pitch -- warp/jump transition.</summary>
        public static AudioClip WarpJump => _warpJump ??= ProceduralAudio.CreateTone(
            "Ship_WarpJump", 0.8f, 100f, 2000f,
            waveType: 2, attack: 0.01f, decay: 0.1f, sustain: 0.5f, release: 0.2f, volume: 0.4f);

        private static AudioClip _warningAlarm;
        /// <summary>Rapid 3-pulse beep, ascending pitch -- catches attention.</summary>
        public static AudioClip WarningAlarm => _warningAlarm ??= ProceduralAudio.CreateArpeggio(
            "Ship_WarningAlarm",
            new float[] { 600f, 800f, 1000f },
            noteDuration: 0.08f, waveType: 0, volume: 0.45f);

        // =====================================================================
        // Ambient Loops
        // =====================================================================

        private static AudioClip _spaceHum;
        /// <summary>Very low filtered noise + sub-bass tone, looping -- the void.</summary>
        public static AudioClip SpaceHum => _spaceHum ??= ProceduralAudio.CreateAmbientLoop(
            "Ambient_SpaceHum", 4.0f, 80f, toneFreq: 40f, toneWave: 1, volume: 0.1f);

        private static AudioClip _stationBustle;
        /// <summary>Filtered noise + random blips -- busy station atmosphere.</summary>
        public static AudioClip StationBustle => _stationBustle ??= ProceduralAudio.CreateAmbientLoop(
            "Ambient_StationBustle", 4.0f, 400f, toneFreq: 120f, toneWave: 1, volume: 0.12f);

        private static AudioClip _planetWind;
        /// <summary>Filtered noise sweep -- planetary wind loop.</summary>
        public static AudioClip PlanetWind => _planetWind ??= ProceduralAudio.CreateAmbientLoop(
            "Ambient_PlanetWind", 4.0f, 250f, toneFreq: 0f, volume: 0.1f);

        private static AudioClip _asteroidRumble;
        /// <summary>Low rumble -- asteroid belt ambient.</summary>
        public static AudioClip AsteroidRumble => _asteroidRumble ??= ProceduralAudio.CreateAmbientLoop(
            "Ambient_AsteroidRumble", 4.0f, 120f, toneFreq: 55f, toneWave: 1, volume: 0.1f);

        private static AudioClip _volcanicRumble;
        /// <summary>Deep rumble with bubbling feel -- volcanic planet.</summary>
        public static AudioClip VolcanicRumble => _volcanicRumble ??= ProceduralAudio.CreateAmbientLoop(
            "Ambient_VolcanicRumble", 4.0f, 150f, toneFreq: 45f, toneWave: 2, volume: 0.12f);

        private static AudioClip _iceWind;
        /// <summary>Whistling wind with crystalline shimmer -- ice planet.</summary>
        public static AudioClip IceWind => _iceWind ??= ProceduralAudio.CreateAmbientLoop(
            "Ambient_IceWind", 4.0f, 600f, toneFreq: 0f, volume: 0.08f);

        // =====================================================================
        // Music Loops
        // =====================================================================

        // Note helper
        private static float N(int midi) => ProceduralAudio.NoteFreq(midi);

        private static AudioClip _mainTheme;
        /// <summary>
        /// 16-bar main theme: square wave melody + triangle bass + noise drums at 120 BPM.
        /// Adventurous, slightly melancholic, unmistakable Odyssey identity.
        /// Key: C major with modal inflections.
        /// </summary>
        public static AudioClip MainTheme
        {
            get
            {
                if (_mainTheme != null) return _mainTheme;

                // 120 BPM, 2 steps per beat = 8th notes
                // 16 bars x 4 beats x 2 steps = 128 steps
                float bpm = 120f;
                int stepsPerBeat = 2;

                // Melody: iconic ascending motif (C-E-G then descending variation)
                float[] melody = new float[]
                {
                    // Bar 1-2: Opening motif (C major ascending)
                    N(ProceduralAudio.C5), 0, N(ProceduralAudio.E5), 0,
                    N(ProceduralAudio.G5), 0, N(ProceduralAudio.E5), 0,
                    N(ProceduralAudio.C5), N(ProceduralAudio.D5), N(ProceduralAudio.E5), 0,
                    N(ProceduralAudio.G5), 0, 0, 0,
                    // Bar 3-4: Response phrase (descending with character)
                    N(ProceduralAudio.A5), 0, N(ProceduralAudio.G5), 0,
                    N(ProceduralAudio.E5), 0, N(ProceduralAudio.D5), 0,
                    N(ProceduralAudio.C5), 0, N(ProceduralAudio.E5), 0,
                    N(ProceduralAudio.D5), 0, 0, 0,
                    // Bar 5-6: Second phrase (higher register, builds)
                    N(ProceduralAudio.E5), 0, N(ProceduralAudio.G5), 0,
                    N(ProceduralAudio.A5), 0, N(ProceduralAudio.G5), 0,
                    N(ProceduralAudio.C6), 0, N(ProceduralAudio.B5), 0,
                    N(ProceduralAudio.A5), 0, N(ProceduralAudio.G5), 0,
                    // Bar 7-8: Resolution
                    N(ProceduralAudio.A5), 0, N(ProceduralAudio.G5), N(ProceduralAudio.E5),
                    N(ProceduralAudio.D5), 0, N(ProceduralAudio.C5), 0,
                    N(ProceduralAudio.E5), 0, N(ProceduralAudio.D5), 0,
                    N(ProceduralAudio.C5), 0, 0, 0,
                    // Bar 9-10: Variation of opening (octave lower for contrast)
                    N(ProceduralAudio.C4), 0, N(ProceduralAudio.E4), 0,
                    N(ProceduralAudio.G4), 0, N(ProceduralAudio.E4), 0,
                    N(ProceduralAudio.C4), N(ProceduralAudio.D4), N(ProceduralAudio.E4), 0,
                    N(ProceduralAudio.G4), 0, N(ProceduralAudio.A4), 0,
                    // Bar 11-12: Building back up
                    N(ProceduralAudio.B4), 0, N(ProceduralAudio.C5), 0,
                    N(ProceduralAudio.D5), 0, N(ProceduralAudio.E5), 0,
                    N(ProceduralAudio.G5), 0, N(ProceduralAudio.A5), 0,
                    N(ProceduralAudio.G5), 0, 0, 0,
                    // Bar 13-14: Climax phrase
                    N(ProceduralAudio.C6), 0, N(ProceduralAudio.B5), 0,
                    N(ProceduralAudio.A5), 0, N(ProceduralAudio.G5), 0,
                    N(ProceduralAudio.A5), N(ProceduralAudio.G5), N(ProceduralAudio.E5), 0,
                    N(ProceduralAudio.G5), 0, 0, 0,
                    // Bar 15-16: Final resolution to root
                    N(ProceduralAudio.E5), 0, N(ProceduralAudio.D5), 0,
                    N(ProceduralAudio.C5), 0, N(ProceduralAudio.E5), 0,
                    N(ProceduralAudio.D5), 0, N(ProceduralAudio.C5), 0,
                    N(ProceduralAudio.C5), 0, 0, 0,
                };

                // Bass: arpeggiated root notes following chord progression
                float[] bass = new float[]
                {
                    // C - Am - F - G chord progression, repeated
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.G3), 0,
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.G3), 0,
                    N(ProceduralAudio.A3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.A3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.F3), 0, N(ProceduralAudio.C3), 0,
                    N(ProceduralAudio.F3), 0, N(ProceduralAudio.C3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.D3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.D3), 0,
                    // Repeat with slight variation
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.C3), 0,
                    N(ProceduralAudio.A3), 0, N(ProceduralAudio.C3), 0,
                    N(ProceduralAudio.E3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.F3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.F3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.B3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.D3), 0,
                    // Second half -- same progression
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.G3), 0,
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.G3), 0,
                    N(ProceduralAudio.A3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.A3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.F3), 0, N(ProceduralAudio.C3), 0,
                    N(ProceduralAudio.F3), 0, N(ProceduralAudio.C3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.D3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.D3), 0,
                    // Final 4 bars
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.F3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.B3), 0,
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.G3), 0, N(ProceduralAudio.C3), 0,
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.G3), 0,
                    N(ProceduralAudio.C3), 0, 0, 0,
                };

                // Drums: kick on beats 1&3, hi-hat on every 8th note
                bool[] drums = new bool[128];
                for (int i = 0; i < 128; i++)
                {
                    // Every 8th note gets a hi-hat-like hit
                    drums[i] = (i % 2 == 0);
                    // Emphasize beats 1 and 3 (every 4 steps)
                    if (i % 4 == 0) drums[i] = true;
                }

                _mainTheme = ProceduralAudio.CreateMusicLoop(
                    "Music_MainTheme", bpm, melody, bass, drums, stepsPerBeat,
                    melodyWave: 0, bassWave: 1,
                    melodyVol: 0.22f, bassVol: 0.16f, drumVol: 0.1f);

                return _mainTheme;
            }
        }

        private static AudioClip _spaceExplore;
        /// <summary>
        /// Atmospheric slow arpeggio at 80 BPM -- triangle wave, reverb-like delay.
        /// Open Space exploration: vast, serene, contemplative.
        /// </summary>
        public static AudioClip SpaceExplore
        {
            get
            {
                if (_spaceExplore != null) return _spaceExplore;

                float bpm = 80f;
                int stepsPerBeat = 2;

                // Slow arpeggiated chords: Am - F - C - G
                float[] melody = new float[]
                {
                    // Bar 1-2
                    N(ProceduralAudio.A4), 0, 0, N(ProceduralAudio.C5),
                    0, 0, N(ProceduralAudio.E5), 0,
                    0, N(ProceduralAudio.A4), 0, 0,
                    N(ProceduralAudio.C5), 0, 0, 0,
                    // Bar 3-4
                    N(ProceduralAudio.F4), 0, 0, N(ProceduralAudio.A4),
                    0, 0, N(ProceduralAudio.C5), 0,
                    0, N(ProceduralAudio.F4), 0, 0,
                    N(ProceduralAudio.A4), 0, 0, 0,
                    // Bar 5-6
                    N(ProceduralAudio.C5), 0, 0, N(ProceduralAudio.E5),
                    0, 0, N(ProceduralAudio.G5), 0,
                    0, N(ProceduralAudio.C5), 0, 0,
                    N(ProceduralAudio.E5), 0, 0, 0,
                    // Bar 7-8
                    N(ProceduralAudio.G4), 0, 0, N(ProceduralAudio.B4),
                    0, 0, N(ProceduralAudio.D5), 0,
                    0, N(ProceduralAudio.G4), 0, 0,
                    N(ProceduralAudio.B4), 0, 0, 0,
                };

                float[] bass = new float[]
                {
                    N(ProceduralAudio.A3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.A3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.F3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.F3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.C3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.C3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.G3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.G3), 0, 0, 0, 0, 0, 0, 0,
                };

                // Sparse percussion -- gentle ticks on every other beat
                bool[] drums = new bool[64];
                for (int i = 0; i < 64; i++)
                    drums[i] = (i % 8 == 0);

                _spaceExplore = ProceduralAudio.CreateMusicLoop(
                    "Music_SpaceExplore", bpm, melody, bass, drums, stepsPerBeat,
                    melodyWave: 1, bassWave: 4,
                    melodyVol: 0.18f, bassVol: 0.12f, drumVol: 0.06f);

                return _spaceExplore;
            }
        }

        private static AudioClip _stationCalm;
        /// <summary>
        /// Warm pad chords (triangle) + gentle melody (square, low volume).
        /// Station/city ambiance: social, warm, safe.
        /// </summary>
        public static AudioClip StationCalm
        {
            get
            {
                if (_stationCalm != null) return _stationCalm;

                float bpm = 95f;
                int stepsPerBeat = 2;

                // Gentle melody in C major with jazzy feel
                float[] melody = new float[]
                {
                    // Bar 1-2
                    N(ProceduralAudio.E4), 0, N(ProceduralAudio.G4), 0,
                    N(ProceduralAudio.A4), 0, 0, N(ProceduralAudio.G4),
                    N(ProceduralAudio.E4), 0, N(ProceduralAudio.D4), 0,
                    N(ProceduralAudio.C4), 0, 0, 0,
                    // Bar 3-4
                    N(ProceduralAudio.D4), 0, N(ProceduralAudio.F4), 0,
                    N(ProceduralAudio.A4), 0, 0, N(ProceduralAudio.G4),
                    N(ProceduralAudio.F4), 0, N(ProceduralAudio.E4), 0,
                    N(ProceduralAudio.D4), 0, 0, 0,
                    // Bar 5-6
                    N(ProceduralAudio.C4), 0, N(ProceduralAudio.E4), 0,
                    N(ProceduralAudio.G4), 0, 0, N(ProceduralAudio.A4),
                    N(ProceduralAudio.G4), 0, N(ProceduralAudio.E4), 0,
                    N(ProceduralAudio.C4), 0, 0, 0,
                    // Bar 7-8
                    N(ProceduralAudio.G4), 0, N(ProceduralAudio.A4), 0,
                    N(ProceduralAudio.B4), 0, 0, N(ProceduralAudio.A4),
                    N(ProceduralAudio.G4), 0, N(ProceduralAudio.E4), 0,
                    N(ProceduralAudio.C4), 0, 0, 0,
                };

                // Warm bass pad: whole notes on root
                float[] bass = new float[]
                {
                    N(ProceduralAudio.C3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.A3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.F3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.G3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.C3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.A3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.F3), 0, 0, 0, 0, 0, 0, 0,
                    N(ProceduralAudio.G3), 0, 0, 0, 0, 0, 0, 0,
                };

                // Very light percussion
                bool[] drums = new bool[64];
                for (int i = 0; i < 64; i++)
                    drums[i] = (i % 4 == 0);

                _stationCalm = ProceduralAudio.CreateMusicLoop(
                    "Music_StationCalm", bpm, melody, bass, drums, stepsPerBeat,
                    melodyWave: 0, bassWave: 1,
                    melodyVol: 0.15f, bassVol: 0.12f, drumVol: 0.05f);

                return _stationCalm;
            }
        }

        private static AudioClip _combatIntense;
        /// <summary>
        /// Fast drums (noise) + urgent square wave melody at 140 BPM.
        /// Combat: intense, high-stakes, driving percussion.
        /// </summary>
        public static AudioClip CombatIntense
        {
            get
            {
                if (_combatIntense != null) return _combatIntense;

                float bpm = 140f;
                int stepsPerBeat = 2;

                // Aggressive staccato melody in D minor
                float[] melody = new float[]
                {
                    // Bar 1-2: Urgent repeating motif
                    N(ProceduralAudio.D5), N(ProceduralAudio.D5), 0, N(ProceduralAudio.F5),
                    N(ProceduralAudio.E5), 0, N(ProceduralAudio.D5), 0,
                    N(ProceduralAudio.A5), 0, N(ProceduralAudio.G5), N(ProceduralAudio.F5),
                    N(ProceduralAudio.E5), 0, N(ProceduralAudio.D5), 0,
                    // Bar 3-4: Rising tension
                    N(ProceduralAudio.F5), N(ProceduralAudio.F5), 0, N(ProceduralAudio.G5),
                    N(ProceduralAudio.A5), 0, N(ProceduralAudio.G5), 0,
                    N(ProceduralAudio.F5), 0, N(ProceduralAudio.E5), N(ProceduralAudio.D5),
                    N(ProceduralAudio.E5), 0, 0, 0,
                    // Bar 5-6: Repeat with variation
                    N(ProceduralAudio.D5), N(ProceduralAudio.D5), 0, N(ProceduralAudio.F5),
                    N(ProceduralAudio.G5), 0, N(ProceduralAudio.A5), 0,
                    N(ProceduralAudio.Bb5), 0, N(ProceduralAudio.A5), N(ProceduralAudio.G5),
                    N(ProceduralAudio.F5), 0, N(ProceduralAudio.E5), 0,
                    // Bar 7-8: Resolution to root
                    N(ProceduralAudio.A5), N(ProceduralAudio.G5), N(ProceduralAudio.F5), 0,
                    N(ProceduralAudio.E5), N(ProceduralAudio.D5), 0, 0,
                    N(ProceduralAudio.D5), 0, N(ProceduralAudio.F5), 0,
                    N(ProceduralAudio.D5), 0, 0, 0,
                };

                // Driving bass: fast arpeggiated pattern
                float[] bass = new float[]
                {
                    N(ProceduralAudio.D3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.D3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.D3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.D3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.Bb3), 0, N(ProceduralAudio.F3), 0,
                    N(ProceduralAudio.Bb3), 0, N(ProceduralAudio.F3), 0,
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.G3), 0,
                    N(ProceduralAudio.C3), 0, N(ProceduralAudio.G3), 0,
                    N(ProceduralAudio.D3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.D3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.Bb3), 0, N(ProceduralAudio.F3), 0,
                    N(ProceduralAudio.Bb3), 0, N(ProceduralAudio.F3), 0,
                    N(ProceduralAudio.A3), 0, N(ProceduralAudio.E3), 0,
                    N(ProceduralAudio.A3), 0, N(ProceduralAudio.D3), 0,
                    N(ProceduralAudio.D3), 0, N(ProceduralAudio.A3), 0,
                    N(ProceduralAudio.D3), 0, 0, 0,
                };

                // Heavy drums: kick on every beat, hi-hat on every 8th note
                bool[] drums = new bool[64];
                for (int i = 0; i < 64; i++)
                    drums[i] = true; // every 8th note gets a hit for that urgent feel

                _combatIntense = ProceduralAudio.CreateMusicLoop(
                    "Music_CombatIntense", bpm, melody, bass, drums, stepsPerBeat,
                    melodyWave: 0, bassWave: 2,
                    melodyVol: 0.24f, bassVol: 0.18f, drumVol: 0.14f);

                return _combatIntense;
            }
        }

        private static AudioClip _planetSerene;
        /// <summary>
        /// Gentle triangle wave melody with nature-like filtered noise at 90 BPM.
        /// Planet exploration: warm, pastoral, hopeful.
        /// </summary>
        public static AudioClip PlanetSerene
        {
            get
            {
                if (_planetSerene != null) return _planetSerene;

                float bpm = 90f;
                int stepsPerBeat = 2;

                // Pastoral melody in G major
                float[] melody = new float[]
                {
                    // Bar 1-2
                    N(ProceduralAudio.G4), 0, N(ProceduralAudio.A4), 0,
                    N(ProceduralAudio.B4), 0, 0, N(ProceduralAudio.A4),
                    N(ProceduralAudio.G4), 0, N(ProceduralAudio.E4), 0,
                    N(ProceduralAudio.D4), 0, 0, 0,
                    // Bar 3-4
                    N(ProceduralAudio.E4), 0, N(ProceduralAudio.G4), 0,
                    N(ProceduralAudio.A4), 0, 0, N(ProceduralAudio.B4),
                    N(ProceduralAudio.A4), 0, N(ProceduralAudio.G4), 0,
                    N(ProceduralAudio.E4), 0, 0, 0,
                    // Bar 5-6
                    N(ProceduralAudio.B4), 0, N(ProceduralAudio.D5), 0,
                    N(ProceduralAudio.E5), 0, 0, N(ProceduralAudio.D5),
                    N(ProceduralAudio.B4), 0, N(ProceduralAudio.A4), 0,
                    N(ProceduralAudio.G4), 0, 0, 0,
                    // Bar 7-8: Gentle resolution
                    N(ProceduralAudio.A4), 0, N(ProceduralAudio.B4), 0,
                    N(ProceduralAudio.A4), 0, 0, N(ProceduralAudio.G4),
                    N(ProceduralAudio.E4), 0, N(ProceduralAudio.D4), 0,
                    N(ProceduralAudio.G4), 0, 0, 0,
                };

                // Gentle bass
                float[] bass = new float[]
                {
                    N(ProceduralAudio.G3), 0, 0, 0, N(ProceduralAudio.D3), 0, 0, 0,
                    N(ProceduralAudio.C3), 0, 0, 0, N(ProceduralAudio.G3), 0, 0, 0,
                    N(ProceduralAudio.C3), 0, 0, 0, N(ProceduralAudio.E3), 0, 0, 0,
                    N(ProceduralAudio.D3), 0, 0, 0, N(ProceduralAudio.G3), 0, 0, 0,
                    N(ProceduralAudio.G3), 0, 0, 0, N(ProceduralAudio.B3), 0, 0, 0,
                    N(ProceduralAudio.C3), 0, 0, 0, N(ProceduralAudio.E3), 0, 0, 0,
                    N(ProceduralAudio.D3), 0, 0, 0, N(ProceduralAudio.G3), 0, 0, 0,
                    N(ProceduralAudio.G3), 0, 0, 0, 0, 0, 0, 0,
                };

                // Light percussion -- sparse
                bool[] drums = new bool[64];
                for (int i = 0; i < 64; i++)
                    drums[i] = (i % 4 == 0);

                _planetSerene = ProceduralAudio.CreateMusicLoop(
                    "Music_PlanetSerene", bpm, melody, bass, drums, stepsPerBeat,
                    melodyWave: 1, bassWave: 1,
                    melodyVol: 0.18f, bassVol: 0.13f, drumVol: 0.06f);

                return _planetSerene;
            }
        }

        private static AudioClip _deathSting;
        /// <summary>2-second descending minor chord sting for death screen.</summary>
        public static AudioClip DeathSting
        {
            get
            {
                if (_deathSting != null) return _deathSting;

                _deathSting = ProceduralAudio.CreateArpeggio(
                    "Music_DeathSting",
                    new float[]
                    {
                        N(ProceduralAudio.D5),
                        N(ProceduralAudio.Bb4),
                        N(ProceduralAudio.A4),
                        N(ProceduralAudio.F4),
                        N(ProceduralAudio.D4),
                        0f, 0f, 0f,
                    },
                    noteDuration: 0.25f, waveType: 0, volume: 0.3f);

                return _deathSting;
            }
        }
    }
}
