#ifndef PAINTCLI_C_
#define PAINTCLI_C_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

typedef int Errno;

#define return_defer(value) do { result = (value); goto defer; } while (0)

#define PAINTCLI_SWAP(T, a, b) do { T t = a; a = b; b = t; } while (0)

void swap_int(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

void paintcli_fill(uint32_t *pixels, size_t width, size_t height, uint32_t color) {
    for (size_t i = 0; i < width * height; ++i) {
        pixels[i] = color;
    }
}

Errno paintcli_save_to_ppm_file(uint32_t *pixels, size_t width, size_t height, const char *file_path) {
    int result = 0;
    FILE *f = NULL;

    f = fopen(file_path, "wb");
    if (f == NULL) return_defer(errno);

    fprintf(f, "P6\n%zu %zu 255\n", width, height);
    if (ferror(f)) return_defer(errno);

    for (size_t i = 0; i < width * height; ++i) {
        uint32_t pixel = pixels[i];
        uint8_t bytes[3] = {
            (pixel >> (8 * 0)) & 0xFF,
            (pixel >> (8 * 1)) & 0xFF,
            (pixel >> (8 * 2)) & 0xFF,
        };
        fwrite(bytes, sizeof(bytes), 1, f);
        if (ferror(f)) return_defer(errno);
    }

defer:
    if (f) fclose(f);
    return result;
}

void paintcli_fill_rect(uint32_t *pixels, size_t pixels_width, size_t pixels_height, int x0, int y0, size_t w, size_t h, uint32_t color) {
    for (int dy = 0; dy < (int)h; ++dy) {
        int y = y0 + dy;
        if (0 <= y && y < (int)pixels_height) {
            for (int dx = 0; dx < (int)w; ++dx) {
                int x = x0 + dx;
                if (0 <= x && x < (int)pixels_width) {
                    pixels[y * pixels_width + x] = color;
                }
            }
        }
    }
}

void paintcli_fill_circle(uint32_t *pixels, size_t pixels_width, size_t pixels_height, int cx, int cy, int r, uint32_t color) {
    int x1 = cx - r;
    int y1 = cy - r;
    int x2 = cx + r;
    int y2 = cy + r;
    for (int y = y1; y <= y2; ++y) {
        if (0 <= y && y < (int)pixels_height) {
            for (int x = x1; x <= x2; ++x) {
                if (0 <= x && x < (int)pixels_width) {
                    int dx = x - cx;
                    int dy = y - cy;
                    if (dx * dx + dy * dy <= r * r) {
                        pixels[y * pixels_width + x] = color;
                    }
                }
            }
        }
    }
}

void paintcli_draw_line(uint32_t *pixels, size_t pixels_width, size_t pixels_height, int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = x2 - x1;
    int dy = y2 - y1;

    if (dx != 0) {
        int c = y1 - dy * x1 / dx;

        if (x1 > x2) PAINTCLI_SWAP(int, x1, x2);
        for (int x = x1; x <= x2; ++x) {
            if (0 <= x && x < (int)pixels_width) {
                int sy1 = dy * x / dx + c;
                int sy2 = dy * (x + 1) / dx + c;
                if (sy1 > sy2) PAINTCLI_SWAP(int, sy1, sy2);
                for (int y = sy1; y <= sy2; ++y) {
                    if (0 <= y && y < (int)pixels_height) {
                        pixels[y * pixels_width + x] = color;
                    }
                }
            }
        }
    } else {
        int x = x1;
        if (0 <= x && x < (int)pixels_width) {
            if (y1 > y2) PAINTCLI_SWAP(int, y1, y2);
            for (int y = y1; y <= y2; ++y) {
                if (0 <= y && y < (int)pixels_height) {
                    pixels[y * pixels_width + x] = color;
                }
            }
        }
    }
}

#endif // PAINTCLI_C_

#define WIDTH 800
#define HEIGHT 600

#define COLS (8 * 2)
#define ROWS (6 * 2)
#define CELL_WIDTH (WIDTH / COLS)
#define CELL_HEIGHT (HEIGHT / ROWS)

#define BACKGROUND_COLOR 0xFF202020
#define FOREGROUND_COLOR 0xFF2020FF

static uint32_t pixels[WIDTH * HEIGHT];

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

bool checker_example(void) {
    paintcli_fill(pixels, WIDTH, HEIGHT, BACKGROUND_COLOR);

    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            uint32_t color = BACKGROUND_COLOR;
            if ((x + y) % 2 == 0) {
                color = 0xFF2020FF;
            }
            paintcli_fill_rect(pixels, WIDTH, HEIGHT, x * CELL_WIDTH, y * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, color);
        }
    }

    const char *file_path = "checker.ppm";
    Errno err = paintcli_save_to_ppm_file(pixels, WIDTH, HEIGHT, file_path);
    if (err) {
        fprintf(stderr, "ERROR: could not save file %s: %s\n", file_path, strerror(errno));
        return false;
    }

    return true;
}

bool circle_example(void) {
    paintcli_fill(pixels, WIDTH, HEIGHT, BACKGROUND_COLOR);

    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            float u = (float)x / COLS;
            float v = (float)y / ROWS;
            float t = (u + v) / 2;

            size_t radius = CELL_WIDTH;
            if (CELL_HEIGHT < radius) radius = CELL_HEIGHT;

            paintcli_fill_circle(pixels, WIDTH, HEIGHT,
                x * CELL_WIDTH + CELL_WIDTH / 2, y * CELL_HEIGHT + CELL_HEIGHT / 2,
                (size_t)lerpf(radius / 8, radius / 2, t),
                FOREGROUND_COLOR);
        }
    }

    const char *file_path = "circle.ppm";
    Errno err = paintcli_save_to_ppm_file(pixels, WIDTH, HEIGHT, file_path);
    if (err) {
        fprintf(stderr, "ERROR: could not save file %s: %s\n", file_path, strerror(errno));
        return false;
    }
    return true;
}

bool lines_example(void) {
    paintcli_fill(pixels, WIDTH, HEIGHT, BACKGROUND_COLOR);

    paintcli_draw_line(pixels, WIDTH, HEIGHT,
        0, 0, WIDTH, HEIGHT,
        FOREGROUND_COLOR);

    paintcli_draw_line(pixels, WIDTH, HEIGHT,
        WIDTH, 0, 0, HEIGHT,
        FOREGROUND_COLOR);

    paintcli_draw_line(pixels, WIDTH, HEIGHT,
        0, 0, WIDTH / 4, HEIGHT,
        0xFF20FF20);

    paintcli_draw_line(pixels, WIDTH, HEIGHT,
        WIDTH / 4, 0, 0, HEIGHT,
        0xFF20FF20);

    paintcli_draw_line(pixels, WIDTH, HEIGHT,
        WIDTH, 0, WIDTH / 4 * 3, HEIGHT,
        0xFF20FF20);

    paintcli_draw_line(pixels, WIDTH, HEIGHT,
        WIDTH / 4 * 3, 0, WIDTH, HEIGHT,
        0xFF20FF20);

    paintcli_draw_line(pixels, WIDTH, HEIGHT,
        0, HEIGHT / 2, WIDTH, HEIGHT / 2,
        0xFFFF3030);

    paintcli_draw_line(pixels, WIDTH, HEIGHT,
        WIDTH / 2, 0, WIDTH / 2, HEIGHT,
        0xFFFF3030);

    const char *file_path = "lines.ppm";
    Errno err = paintcli_save_to_ppm_file(pixels, WIDTH, HEIGHT, file_path);
    if (err) {
        fprintf(stderr, "ERROR: could not save file %s: %s\n", file_path, strerror(errno));
        return false;
    }
    return true;
}

bool brick_example(void) {
    paintcli_fill(pixels, WIDTH, HEIGHT, 0xFF000000); // Black background

    // Front face
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 200, 400, 400, 400, 0xFFFFFFFF); // White lines
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 400, 400, 400, 300, 0xFFFFFFFF);
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 400, 300, 200, 300, 0xFFFFFFFF);
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 200, 300, 200, 400, 0xFFFFFFFF);

    // Top face
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 200, 300, 250, 250, 0xFFFFFFFF);
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 250, 250, 450, 250, 0xFFFFFFFF);

    // Right face
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 400, 400, 450, 350, 0xFFFFFFFF);
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 450, 350, 450, 250, 0xFFFFFFFF);
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 450, 250, 400, 300, 0xFFFFFFFF);
    paintcli_draw_line(pixels, WIDTH, HEIGHT, 400, 300, 400, 400, 0xFFFFFFFF);

    const char *file_path = "brick.ppm";
    Errno err = paintcli_save_to_ppm_file(pixels, WIDTH, HEIGHT, file_path);
    if (err) {
        fprintf(stderr, "ERROR: could not save file %s: %s\n", file_path, strerror(errno));
        return false;
    }

    return true;
}

int main(void) {
    if (!checker_example()) return -1;
    if (!circle_example()) return -1;
    if (!lines_example()) return -1;
    if (!brick_example()) return -1;
    return 0;
}
