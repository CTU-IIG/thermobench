// vim: filetype=c

// number dimensions
#define X_MIN (-2.5)
#define X_MAX (1.5)
#define Y_MIN (-2.0)
#define Y_MAX (2.0)

// screen -> number conversion ratios
#define PIXEL_WIDTH ((X_MAX - X_MIN) / HEIGHT)
#define PIXEL_HEIGHT ((Y_MAX - Y_MIN) / WIDTH)

__kernel
void mandelbrot(__global unsigned char* buffer) {
    float Zr, Zi;   // Z = Zr+Zi*i; Z0 = 0
    float Zr2, Zi2; // Zr2 = Zr*Zr; Zi2 = Zi*Zi

    const uint gs = get_global_size(0);
    const uint work_group = get_group_id(0);
    const bool run = (GROUP_MASK == 0) || !!((1U << work_group) & GROUP_MASK);

    for (int x_px = get_global_id(0); x_px < WIDTH; x_px += gs) {
        const float x = X_MIN + x_px * PIXEL_WIDTH;

        for (int y_px = 0; y_px < HEIGHT; y_px++) {
            float y = Y_MIN + y_px * PIXEL_HEIGHT;
            if (fabs(y) < PIXEL_HEIGHT / 2) {
                y = 0.0; // Main antenna
            }

            // initial value of orbit = critical point Z = 0
            Zr = 0.0;
            Zi = 0.0;
            Zr2 = Zr * Zr;
            Zi2 = Zi * Zi;

            int iter = 0;
            for (iter = 0; run && iter < MAX_ITER && !ESCAPED(Zr2 + Zi2); iter++) {
                Zi = 2 * Zr * Zi + y;
                Zr = Zr2 - Zi2 + x;
                Zr2 = Zr * Zr;
                Zi2 = Zi * Zi;
            }

            buffer[WIDTH * y_px + x_px] = (-iter) & 0xFF;
        }
    }
}
