#!/usr/bin/env python3
"""Genera 32 sonidos electronicos originales (2 kits de 16 pads) en WAV 16 bit mono.

Uso: python3 gen_sounds.py <carpeta_salida>
Solo usa la biblioteca estandar (sin numpy).
"""
import math
import os
import random
import struct
import sys
import wave

SR = 44100
TWO_PI = 2 * math.pi


def n(sec):
    return int(SR * sec)


def shape(ph, kind):
    if kind == "sine":
        return math.sin(ph)
    if kind == "saw":
        return 2.0 * ((ph / TWO_PI) % 1.0) - 1.0
    if kind == "square":
        return 1.0 if math.sin(ph) >= 0 else -1.0
    raise ValueError(kind)


def sweep(f0, f1, dur, decay, kind="sine", k=8.0):
    """Oscilador con caida exponencial de frecuencia f0 -> f1 y amplitud."""
    N = n(dur)
    out, ph = [], 0.0
    for i in range(N):
        t = i / N
        f = f1 + (f0 - f1) * math.exp(-t * k)
        ph += TWO_PI * f / SR
        out.append(shape(ph, kind) * math.exp(-i / (SR * decay)))
    return out


def tone(freq, dur, decay, kind="saw", attack=0.002):
    N = n(dur)
    out, ph = [], 0.0
    for i in range(N):
        ph += TWO_PI * freq / SR
        a = min(1.0, (i / SR) / attack) if attack > 0 else 1.0
        out.append(shape(ph, kind) * a * math.exp(-i / (SR * decay)))
    return out


def noise(dur, decay, seed):
    rng = random.Random(seed)
    return [rng.uniform(-1, 1) * math.exp(-i / (SR * decay)) for i in range(n(dur))]


def hp(x, a=0.95):
    y, py, px = [], 0.0, 0.0
    for v in x:
        py = a * (py + v - px)
        px = v
        y.append(py)
    return y


def lp(x, a=0.1):
    y, py = [], 0.0
    for v in x:
        py += a * (v - py)
        y.append(py)
    return y


def mix(*parts):
    """parts: (senal, ganancia, desplazamiento_en_segundos)"""
    length = max(n(off) + len(sig) for sig, _, off in parts)
    out = [0.0] * length
    for sig, g, off in parts:
        o = n(off)
        for i, v in enumerate(sig):
            out[o + i] += v * g
    return out


def scale(x, g):
    return [v * g for v in x]


def distort(x, drive):
    return [math.tanh(drive * v) for v in x]


def chord(freqs, dur, decay, kind="saw", attack=0.002):
    return mix(*[(tone(f, dur, decay, kind, attack), 1.0 / len(freqs), 0.0) for f in freqs])


# ---------------- Kit 1: electro ----------------

def kick_808():
    return sweep(180, 45, 0.7, 0.25, k=10)


def kick_punch():
    return mix((sweep(250, 55, 0.35, 0.09, k=14), 1.0, 0),
               (hp(noise(0.012, 0.003, 1), 0.9), 0.35, 0))


def snare():
    return mix((sweep(220, 170, 0.25, 0.06, k=5), 0.5, 0),
               (hp(noise(0.3, 0.07, 2), 0.85), 0.8, 0))


def snare_snap():
    return mix((sweep(330, 200, 0.15, 0.03, k=6), 0.4, 0),
               (hp(noise(0.18, 0.04, 3), 0.9), 0.9, 0))


def clap(tail=0.25, tail_decay=0.06, seed=4):
    parts = [(hp(noise(0.02, 0.005, seed + j), 0.9), 0.8, 0.011 * j) for j in range(3)]
    parts.append((hp(noise(tail, tail_decay, seed + 9), 0.9), 0.7, 0.03))
    return mix(*parts)


def hat_closed():
    return hp(noise(0.08, 0.015, 5), 0.97)


def hat_open():
    return hp(noise(0.5, 0.12, 6), 0.97)


def crash():
    metal = [mix(*[(tone(f, 1.5, 0.4, "square", 0), 0.2, 0) for f in (205, 297, 369, 415, 531)])]
    return mix((hp(noise(1.5, 0.5, 7), 0.95), 0.8, 0), (hp(metal[0], 0.9), 0.3, 0))


def tom(f0, f1, dur=0.45):
    return mix((sweep(f0, f1, dur, 0.15, k=8), 1.0, 0),
               (hp(noise(0.01, 0.003, 8), 0.9), 0.2, 0))


def rimshot():
    return mix((sweep(1000, 800, 0.06, 0.01, k=3), 0.8, 0),
               (hp(noise(0.03, 0.008, 9), 0.9), 0.6, 0))


def cowbell():
    N = n(0.4)
    ph1 = ph2 = 0.0
    out = []
    for i in range(N):
        ph1 += TWO_PI * 540 / SR
        ph2 += TWO_PI * 800 / SR
        s = (shape(ph1, "square") + shape(ph2, "square")) * math.exp(-i / (SR * 0.1))
        out.append(s)
    return lp(out, 0.3)


def bass_synth():
    return lp(tone(55, 0.6, 0.3, "saw", 0.003), 0.08)


def synth_stab():
    return lp(chord([220.0, 261.63, 329.63], 0.45, 0.15), 0.25)


def fx_laser():
    return lp(sweep(2500, 150, 0.4, 0.2, "square", 7), 0.5)


# ---------------- Kit 2: house / dance ----------------

def kick_house():
    return mix((sweep(160, 48, 0.5, 0.18, k=9), 1.0, 0),
               (hp(noise(0.01, 0.003, 11), 0.9), 0.3, 0))


def kick_hard():
    return distort(sweep(200, 50, 0.45, 0.2, k=12), 3.0)


def snare_house():
    return mix((hp(noise(0.25, 0.06, 12), 0.9), 0.9, 0),
               (sweep(200, 180, 0.2, 0.05, k=4), 0.4, 0))


def snare_tight():
    return mix((hp(noise(0.12, 0.025, 13), 0.9), 0.9, 0),
               (sweep(240, 190, 0.1, 0.03, k=4), 0.4, 0))


def clap_big():
    return mix((clap(0.5, 0.12, 14), 1.0, 0),
               (lp(hp(noise(0.6, 0.2, 15), 0.9), 0.4), 0.25, 0.03))


def hat_closed2():
    return hp(noise(0.04, 0.008, 16), 0.98)


def hat_open2():
    return hp(noise(0.3, 0.07, 17), 0.98)


def shaker():
    rng = random.Random(18)
    N = n(0.15)
    out = []
    for i in range(N):
        t = i / N
        out.append(rng.uniform(-1, 1) * t * math.exp(-t * 8))
    return hp(out, 0.93)


def conga():
    return mix((sweep(380, 300, 0.3, 0.08, k=6), 1.0, 0),
               (hp(noise(0.01, 0.003, 19), 0.9), 0.2, 0))


def perc_blip():
    return sweep(1400, 900, 0.12, 0.03, k=8)


def bass_sub():
    return sweep(60, 45, 0.9, 0.5, k=3)


def bass_pluck():
    return lp(tone(82.41, 0.5, 0.12, "saw", 0.002), 0.12)


def lead_stab():
    return lp(chord([440.0, 554.37, 659.25], 0.35, 0.12, "square"), 0.3)


def pad_chord():
    return lp(chord([220.0, 277.18, 329.63, 440.0], 1.2, 0.8, "saw", 0.15), 0.06)


def riser():
    N = n(1.5)
    rng = random.Random(20)
    out, ph = [], 0.0
    for i in range(N):
        t = i / N
        ph += TWO_PI * (200 + 1800 * t * t) / SR
        out.append((rng.uniform(-1, 1) * 0.6 + shape(ph, "saw") * 0.4) * (t ** 2))
    return lp(out, 0.4)


def fx_zap():
    return lp(sweep(3000, 300, 0.25, 0.1, "saw", 10), 0.4)


# (nombre de archivo, funcion, pico objetivo)
SOUNDS = [
    ("01_kick_808", kick_808, 0.95),
    ("02_kick_punch", kick_punch, 0.95),
    ("03_snare", snare, 0.9),
    ("04_snare_snap", snare_snap, 0.85),
    ("05_clap", clap, 0.85),
    ("06_hat_closed", hat_closed, 0.55),
    ("07_hat_open", hat_open, 0.55),
    ("08_crash", crash, 0.6),
    ("09_tom_low", lambda: tom(140, 75, 0.5), 0.9),
    ("10_tom_mid", lambda: tom(190, 105, 0.4), 0.9),
    ("11_tom_high", lambda: tom(260, 150, 0.35), 0.9),
    ("12_rimshot", rimshot, 0.8),
    ("13_cowbell", cowbell, 0.6),
    ("14_bass_synth", bass_synth, 0.85),
    ("15_synth_stab", synth_stab, 0.7),
    ("16_fx_laser", fx_laser, 0.6),
    ("17_kick_house", kick_house, 0.95),
    ("18_kick_hard", kick_hard, 0.9),
    ("19_snare_house", snare_house, 0.85),
    ("20_snare_tight", snare_tight, 0.85),
    ("21_clap_big", clap_big, 0.85),
    ("22_hat_closed2", hat_closed2, 0.5),
    ("23_hat_open2", hat_open2, 0.5),
    ("24_shaker", shaker, 0.5),
    ("25_conga", conga, 0.85),
    ("26_perc_blip", perc_blip, 0.7),
    ("27_bass_sub", bass_sub, 0.9),
    ("28_bass_pluck", bass_pluck, 0.85),
    ("29_lead_stab", lead_stab, 0.6),
    ("30_pad_chord", pad_chord, 0.6),
    ("31_riser", riser, 0.7),
    ("32_fx_zap", fx_zap, 0.6),
]


def write_wav(path, samples, peak):
    m = max(abs(v) for v in samples) or 1.0
    g = peak / m
    fade = n(0.004)
    L = len(samples)
    data = bytearray()
    for i, v in enumerate(samples):
        v *= g
        if i >= L - fade:
            v *= (L - i) / fade
        data += struct.pack("<h", int(max(-1.0, min(1.0, v)) * 32767))
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(bytes(data))


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "sounds"
    os.makedirs(out, exist_ok=True)
    for name, fn, peak in SOUNDS:
        write_wav(os.path.join(out, name + ".wav"), fn(), peak)
        print("ok", name)


if __name__ == "__main__":
    main()
