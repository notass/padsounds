// PadSounds - sampler de pads con SDL2 + SDL_mixer (Android / PS Vita / PC)
//
// Los sonidos se cargan de una carpeta (ver get_sound_dir). Para anadir
// sonidos basta con copiar archivos .wav/.ogg/.mp3/.flac a esa carpeta:
// se asignan a los pads por orden alfabetico, 16 por pagina.

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(p, m) _mkdir(p)
#endif

#define W 960
#define H 544
#define COLS 4
#define ROWS 4
#define PER_PAGE (COLS * ROWS)
#define MAX_SOUNDS 256
#define FLASH_MS 130
#define MARGIN 20
#define GAP 12
#define TOP 40

static Mix_Chunk *sounds[MAX_SOUNDS];
static int nsounds = 0;
static Uint32 flash_until[PER_PAGE];
static int page = 0;
static int cursor = 0;
static int show_cursor = 0;
static SDL_Renderer *ren = NULL;
static SDL_Window *win = NULL;

static const SDL_Color palette[8] = {
    {231, 76, 60, 255},  {230, 126, 34, 255}, {241, 196, 15, 255},
    {46, 204, 113, 255}, {26, 188, 156, 255}, {52, 152, 219, 255},
    {155, 89, 182, 255}, {233, 30, 140, 255}};

static const SDL_Scancode pad_keys[PER_PAGE] = {
    SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
    SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
    SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V};

static void get_sound_dir(char *out, size_t n) {
#if defined(__vita__)
    snprintf(out, n, "ux0:data/PadSounds/");
#elif defined(__ANDROID__)
    const char *p = SDL_AndroidGetExternalStoragePath();
    snprintf(out, n, "%s/sounds/", p ? p : ".");
#else
    snprintf(out, n, "sounds/");
#endif
}

static int has_audio_ext(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    return !strcasecmp(dot, ".wav") || !strcasecmp(dot, ".ogg") ||
           !strcasecmp(dot, ".mp3") || !strcasecmp(dot, ".flac");
}

static int cmp_names(const void *a, const void *b) {
    return strcasecmp(*(char *const *)a, *(char *const *)b);
}

static void load_sounds(const char *dir) {
    DIR *d = opendir(dir);
    if (!d) {
        SDL_Log("No se pudo abrir %s", dir);
        return;
    }
    char *names[MAX_SOUNDS];
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) && n < MAX_SOUNDS) {
        if (e->d_name[0] == '.' || !has_audio_ext(e->d_name)) continue;
        names[n++] = strdup(e->d_name);
    }
    closedir(d);
    qsort(names, n, sizeof(char *), cmp_names);
    for (int i = 0; i < n; i++) {
        char path[512];
        snprintf(path, sizeof path, "%s%s", dir, names[i]);
        Mix_Chunk *c = Mix_LoadWAV(path);
        if (c && nsounds < MAX_SOUNDS)
            sounds[nsounds++] = c;
        else if (c)
            Mix_FreeChunk(c);
        else
            SDL_Log("No se pudo cargar %s: %s", path, Mix_GetError());
        free(names[i]);
    }
    SDL_Log("%d sonidos cargados desde %s", nsounds, dir);
}

#ifdef __ANDROID__
// Sonidos incluidos en el APK (assets/sounds/). Android no permite listar
// assets, asi que se lee assets/sounds/index.txt (una linea por archivo).
static void load_builtin_sounds(void) {
    SDL_RWops *f = SDL_RWFromFile("sounds/index.txt", "rb");
    if (!f) {
        SDL_Log("No hay sounds/index.txt en el APK");
        return;
    }
    Sint64 sz = SDL_RWsize(f);
    if (sz <= 0 || sz > 65536) {
        SDL_RWclose(f);
        return;
    }
    char *buf = malloc((size_t)sz + 1);
    size_t got = SDL_RWread(f, buf, 1, (size_t)sz);
    SDL_RWclose(f);
    buf[got] = 0;
    for (char *line = strtok(buf, "\r\n"); line && nsounds < MAX_SOUNDS;
         line = strtok(NULL, "\r\n")) {
        char path[256];
        snprintf(path, sizeof path, "sounds/%s", line);
        SDL_RWops *rw = SDL_RWFromFile(path, "rb");
        if (!rw) continue;
        Mix_Chunk *c = Mix_LoadWAV_RW(rw, 1);
        if (c)
            sounds[nsounds++] = c;
        else
            SDL_Log("No se pudo cargar %s: %s", path, Mix_GetError());
    }
    free(buf);
}
#endif

static int num_pages(void) {
    int p = (nsounds + PER_PAGE - 1) / PER_PAGE;
    return p < 1 ? 1 : p;
}

static SDL_Rect pad_rect(int i) {
    int cw = (W - 2 * MARGIN - (COLS - 1) * GAP) / COLS;
    int ch = (H - TOP - MARGIN - (ROWS - 1) * GAP) / ROWS;
    SDL_Rect r = {MARGIN + (i % COLS) * (cw + GAP),
                  TOP + (i / COLS) * (ch + GAP), cw, ch};
    return r;
}

static int pad_at(int x, int y) {
    for (int i = 0; i < PER_PAGE; i++) {
        SDL_Rect r = pad_rect(i);
        SDL_Point p = {x, y};
        if (SDL_PointInRect(&p, &r)) return i;
    }
    return -1;
}

static void play_pad(int i) {
    int idx = page * PER_PAGE + i;
    if (i < 0 || i >= PER_PAGE || idx >= nsounds) return;
    Mix_PlayChannel(-1, sounds[idx], 0);
    flash_until[i] = SDL_GetTicks() + FLASH_MS;
}

static void change_page(int delta) {
    int n = num_pages();
    page = ((page + delta) % n + n) % n;
}

// Convierte coordenadas de ventana (pixeles) a coordenadas logicas 960x544.
static void window_to_logical(float wx, float wy, int *lx, int *ly) {
#if SDL_VERSION_ATLEAST(2, 0, 18)
    float fx, fy;
    SDL_RenderWindowToLogical(ren, (int)wx, (int)wy, &fx, &fy);
    *lx = (int)fx;
    *ly = (int)fy;
#else
    int ww, wh;
    SDL_GetWindowSize(win, &ww, &wh);
    *lx = (int)(wx * W / (ww ? ww : W));
    *ly = (int)(wy * H / (wh ? wh : H));
#endif
}

static void draw(void) {
    Uint32 now = SDL_GetTicks();
    SDL_SetRenderDrawColor(ren, 18, 18, 24, 255);
    SDL_RenderClear(ren);

    // Indicador de pagina
    int np = num_pages();
    int total_w = np * 20 - 6;
    for (int p = 0; p < np; p++) {
        SDL_Rect r = {W / 2 - total_w / 2 + p * 20, 14, 14, 8};
        if (p == page)
            SDL_SetRenderDrawColor(ren, 240, 240, 240, 255);
        else
            SDL_SetRenderDrawColor(ren, 70, 70, 80, 255);
        SDL_RenderFillRect(ren, &r);
    }

    for (int i = 0; i < PER_PAGE; i++) {
        int idx = page * PER_PAGE + i;
        SDL_Rect r = pad_rect(i);
        SDL_Color c;
        if (idx >= nsounds) {
            c = (SDL_Color){38, 38, 48, 255};
        } else if (now < flash_until[i]) {
            c = (SDL_Color){255, 255, 255, 255};
        } else {
            c = palette[idx % 8];
            c.r = c.r * 7 / 10;
            c.g = c.g * 7 / 10;
            c.b = c.b * 7 / 10;
        }
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
        SDL_RenderFillRect(ren, &r);

        if (show_cursor && i == cursor) {
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            for (int t = 0; t < 4; t++) {
                SDL_Rect f = {r.x - t, r.y - t, r.w + 2 * t, r.h + 2 * t};
                SDL_RenderDrawRect(ren, &f);
            }
        }
    }
    SDL_RenderPresent(ren);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3 | MIX_INIT_FLAC);
#ifdef __ANDROID__
    int buffer = 1024;
#else
    int buffer = 512;
#endif
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, buffer) < 0) {
        SDL_Log("Mix_OpenAudio: %s", Mix_GetError());
        return 1;
    }
    Mix_AllocateChannels(32);

    Uint32 wflags = SDL_WINDOW_SHOWN;
#if defined(__ANDROID__) || defined(__vita__)
    wflags |= SDL_WINDOW_FULLSCREEN;
#endif
    win = SDL_CreateWindow("Pad Sounds", SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED, W, H, wflags);
    ren = SDL_CreateRenderer(win, -1,
                             SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!win || !ren) {
        SDL_Log("Ventana/render: %s", SDL_GetError());
        return 1;
    }
    SDL_RenderSetLogicalSize(ren, W, H);

    for (int i = 0; i < SDL_NumJoysticks(); i++)
        if (SDL_IsGameController(i)) SDL_GameControllerOpen(i);

    char dir[512];
    get_sound_dir(dir, sizeof dir);
#if defined(__vita__)
    load_sounds("app0:sounds/"); // incluidos en el VPK
#elif defined(__ANDROID__)
    load_builtin_sounds();       // incluidos en el APK
#endif
    mkdir(dir, 0777);
    load_sounds(dir);            // los que anada el usuario

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                running = 0;
                break;

            case SDL_FINGERDOWN: {
                int ww, wh, lx, ly;
                SDL_GetWindowSize(win, &ww, &wh);
                window_to_logical(e.tfinger.x * ww, e.tfinger.y * wh, &lx, &ly);
                show_cursor = 0;
                play_pad(pad_at(lx, ly));
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
                if (e.button.which != SDL_TOUCH_MOUSEID) {
                    int lx, ly;
                    window_to_logical((float)e.button.x, (float)e.button.y, &lx, &ly);
                    show_cursor = 0;
                    play_pad(pad_at(lx, ly));
                }
                break;

            case SDL_KEYDOWN:
                if (e.key.repeat) break;
                if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = 0;
                else if (e.key.keysym.scancode == SDL_SCANCODE_PAGEUP) change_page(-1);
                else if (e.key.keysym.scancode == SDL_SCANCODE_PAGEDOWN ||
                         e.key.keysym.scancode == SDL_SCANCODE_TAB) change_page(1);
                else
                    for (int i = 0; i < PER_PAGE; i++)
                        if (e.key.keysym.scancode == pad_keys[i]) play_pad(i);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                show_cursor = 1;
                switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    if (cursor % COLS > 0) cursor--;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    if (cursor % COLS < COLS - 1) cursor++;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    if (cursor >= COLS) cursor -= COLS;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    if (cursor < PER_PAGE - COLS) cursor += COLS;
                    break;
                case SDL_CONTROLLER_BUTTON_A: // Cruz en Vita
                    play_pad(cursor);
                    break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    change_page(-1);
                    break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                    change_page(1);
                    break;
                }
                break;

            case SDL_CONTROLLERDEVICEADDED:
                SDL_GameControllerOpen(e.cdevice.which);
                break;
            }
        }
        draw();
    }

    for (int i = 0; i < nsounds; i++) Mix_FreeChunk(sounds[i]);
    Mix_CloseAudio();
    Mix_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
