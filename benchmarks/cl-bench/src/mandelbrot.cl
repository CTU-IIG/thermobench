// vim: filetype=c

// number dimensions
#define X_MIN (-2.5)
#define X_MAX (1.5)
#define Y_MIN (-2.0)
#define Y_MAX (2.0)

// screen -> number conversion ratios
#define PIXEL_WIDTH ((X_MAX - X_MIN) / HEIGHT)
#define PIXEL_HEIGHT ((Y_MAX - Y_MIN) / WIDTH)

// color parameters
#define MAX_COLOR_COMPONENT_VALUE (255)
#define MAX_ITER (256)
#define ESCAPE_RADIUS (2)

__kernel
void mandelbrot(__global unsigned char* buffer) {
    float Zr, Zi;   // Z = Zr+Zi*i; Z0 = 0
    float Zr2, Zi2; // Zr2 = Zr*Zr; Zi2 = Zi*Zi

    for (int y_px = get_global_id(0); y_px < WIDTH; y_px += GLOBAL_WS) {
        float y = Y_MIN + y_px * PIXEL_HEIGHT;
        if (fabs(y) < PIXEL_HEIGHT / 2) {
            y = 0.0; // Main antenna
        }

        for (int x_px = 0; x_px < HEIGHT; x_px++) {
            const float x = X_MIN + x_px * PIXEL_WIDTH;

            // initial value of orbit = critical point Z = 0
            Zr = 0.0;
            Zi = 0.0;
            Zr2 = Zr * Zr;
            Zi2 = Zi * Zi;

            int iter;
            for (iter = 0; iter < MAX_ITER && ((Zr2 + Zi2) < (ESCAPE_RADIUS * ESCAPE_RADIUS)); iter++) {
                Zi = 2 * Zr * Zi + y;
                Zr = Zr2 - Zi2 + x;
                Zr2 = Zr * Zr;
                Zi2 = Zi * Zi;
            }

            *(buffer + (3 * WIDTH * y_px) + (3 * x_px)) = (-iter) & 0xFF;
        }
    }
}
