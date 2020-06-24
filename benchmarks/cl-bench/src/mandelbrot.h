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
