# Pad Sounds

Sampler de pads (4x4, varias páginas) en C con SDL2 + SDL_mixer.
Un solo `main.c` para PC, Android y PS Vita.

## Añadir sonidos
Copia archivos `.wav`, `.ogg`, `.mp3` o `.flac` a la carpeta de sonidos.
Se asignan a los pads por orden alfabético, 16 por página.

| Plataforma | Carpeta |
|---|---|
| PC | `./sounds/` |
| Android | `Android/data/<paquete>/files/sounds/` |
| PS Vita | `ux0:data/PadSounds/` |

## Sonidos incluidos
La carpeta `sounds/` trae 32 sonidos originales sintetizados (generados con `gen_sounds.py`, sin samples de terceros):
- **Página 1 – Electro (01-16):** kicks 808 y punch, snares, clap, hats, crash, toms, rimshot, cowbell, bajo, stab, láser.
- **Página 2 – House (17-32):** kicks, snares, clap grande, hats, shaker, conga, bajos, lead, pad, riser, zap.

Para regenerarlos o modificarlos: `python3 gen_sounds.py sounds` (solo usa la biblioteca estándar de Python).
Para copiarlos a Android o Vita, sube el contenido de `sounds/` a la carpeta de la tabla anterior.

## Compilar en GitHub (sin instalar nada)
1. Sube todo este contenido a un repositorio de GitHub (rama `main`).
2. Abre la pestaña **Actions**: se ejecutan `Android APK` y `PS Vita VPK` en cada subida
   (o con **Run workflow**).
3. Al terminar, entra en la ejecución y descarga `padsounds-apk` y `padsounds-vpk` desde **Artifacts**.
4. Los sonidos de `sounds/` ya van dentro del APK y del VPK. Los que añada el usuario
   (carpeta de la tabla de arriba) se suman a los incluidos.

El APK es de depuración (firma de prueba). Para Google Play hará falta compilar en modo release con tu clave.

## Controles
- Táctil (Android / Vita): tocar el pad.
- Vita: cruceta mueve, Cruz toca, L/R cambia de página.
- PC: teclas `1234 QWER ASDF ZXCV`, `Tab`/`AvPág`/`RePág` para páginas, `Esc` sale.

## Compilar
**PC:** `cmake -B build && cmake --build build && mkdir -p build/sounds`

**Vita:** con `VITASDK` configurado: `cmake -B build-vita && cmake --build build-vita` → `padsounds.vpk`.
Si el enlazado falla, ajusta las librerías de audio en `CMakeLists.txt` según tu build de SDL2_mixer.

**Android:** crea el proyecto desde la plantilla `android-project` de SDL2, añade SDL2_mixer en `jni/`
y apunta `Android.mk` a `main.c`. La app lee los sonidos de su carpeta externa (arriba).
