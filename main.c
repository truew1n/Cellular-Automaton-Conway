#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#define CELL_ROWS 750
#define CELL_COLS 1250

#define CELL_SIZE 1

#define HEIGHT CELL_ROWS*CELL_SIZE
#define WIDTH CELL_COLS*CELL_SIZE


int8_t exitloop = False;
int8_t cycle = False;

int8_t cells[CELL_ROWS][CELL_COLS] = {0};
int8_t next_cells[CELL_ROWS][CELL_COLS] = {0};

int8_t count_neighbours(int32_t, int32_t);
void next_cycle();
void render(void *);
void gc_put_pixel(void *, int32_t, int32_t, uint32_t);
void gc_draw_rectangle(void *, int32_t, int32_t, int32_t, int32_t, uint32_t);
void swap_boards(int8_t *, int8_t*);
int8_t in_bounds(int32_t, int32_t, int64_t, int64_t);

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main(void)
{

    for(int j = 0; j < CELL_ROWS; ++j) {
        for(int i = 0; i < CELL_COLS; ++i) {
            if((i+j)%2 == 0)
            cells[j][i] = 1;
        }
    }

    Display *display = XOpenDisplay(NULL);

    int32_t screen = XDefaultScreen(display);

    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0,
        WIDTH, HEIGHT,
        0, 0,
        0
    );

    char *memory = (char *) malloc(sizeof(uint32_t)*WIDTH*HEIGHT);

    XWindowAttributes winattr = {0};
    XGetWindowAttributes(display, window, &winattr);

    XImage *image = XCreateImage(
        display, winattr.visual, winattr.depth,
        ZPixmap, 0, (char *) memory,
        WIDTH, HEIGHT,
        32, WIDTH*4
    );

    GC graphics = XCreateGC(display, window, 0, NULL);

    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &delete_window, 1);
    
    Mask button_mask = ButtonPressMask | ButtonReleaseMask | Button1MotionMask;
    Mask key_mask = KeyPressMask | KeyReleaseMask;
    XSelectInput(display, window, ExposureMask | key_mask | button_mask);

    XMapWindow(display, window);
    XSync(display, False);

    XEvent event;

    while(!exitloop) {
        while(XPending(display) > 0) {
            XNextEvent(display, &event);
            if(event.type == ClientMessage) {
                if((Atom) event.xclient.data.l[0] == delete_window) {
                    exitloop = 1;   
                }
            }

            if(event.type == KeyPress) {
                if(event.xkey.keycode == 0x24) {
                    next_cycle();
                }
                if(event.xkey.keycode == 0x41) {
                    cycle = !cycle;
                }
            }

            if(event.type == ButtonPress) {
                if(event.xbutton.button == Button1) {
                    int32_t mx = event.xbutton.x;
                    int32_t my = event.xbutton.y;
                    if(cells[my/CELL_SIZE][mx/CELL_SIZE] == 0) {
                        cells[my/CELL_SIZE][mx/CELL_SIZE] = 1;
                        next_cells[my/CELL_SIZE][mx/CELL_SIZE] = 1;
                    }
                    else {
                        cells[my/CELL_SIZE][mx/CELL_SIZE] = 0;
                        next_cells[my/CELL_SIZE][mx/CELL_SIZE] = 0;
                    }
                }
            }

            if(event.type == MotionNotify) {
                int32_t mx = event.xmotion.x;
                int32_t my = event.xmotion.y;
                if(cells[my/CELL_SIZE][mx/CELL_SIZE] == 0) {
                        cells[my/CELL_SIZE][mx/CELL_SIZE] = 1;
                        next_cells[my/CELL_SIZE][mx/CELL_SIZE] = 1;
                }
                else {
                    cells[my/CELL_SIZE][mx/CELL_SIZE] = 0;
                    next_cells[my/CELL_SIZE][mx/CELL_SIZE] = 0;
                }
            }
        }

        if(cycle) next_cycle();

        gc_draw_rectangle(memory, 0, 0, WIDTH, HEIGHT, 0x00000000);
        render(memory);

        XPutImage(
            display,
            window,
            graphics,
            image,
            0, 0,
            0, 0,
            WIDTH, HEIGHT
        );

        XSync(display, False);
    }
    XCloseDisplay(display);

    free(memory);
    
    return 0;
}

int8_t in_bounds(int32_t x, int32_t y, int64_t maxx, int64_t maxy)
{
    return (x >= 0 && x < maxx && y >= 0 && y < maxy);
}

int8_t count_neighbours(int32_t x, int32_t y)
{
    int8_t alive = 0;
    for(int j = y-1; j <= y+1; ++j) {
        for(int i = x-1; i <= x+1; ++i) {
            if(in_bounds(i, j, CELL_COLS, CELL_ROWS)) {
                if(cells[j][i] == 1) {
                    alive++;
                }
            }
        }
    } alive = ((cells[y][x] == 1)?alive-1:alive);
    return alive;
}


void next_cycle()
{
    for(int j = 0; j < CELL_ROWS; ++j) {
        for(int i = 0; i < CELL_COLS; ++i) {
            int8_t alive = count_neighbours(i, j);
            if(cells[j][i] == 1) {
                if(alive < 2) next_cells[j][i] = 0;
                else if(alive > 3) next_cells[j][i] = 0;
                else if(alive == 2 || alive == 3) next_cells[j][i] = 1;
            } else {
                if(alive == 3) next_cells[j][i] = 1;
                else next_cells[j][i] = 0;
            }
        }
    }
    
    for(int j = 0; j < CELL_ROWS; ++j) {
        for(int i = 0; i < CELL_COLS; ++i) {
            cells[j][i] = next_cells[j][i];
        }
    }
}

void render(void *memory)
{
    for(int j = 0; j < CELL_ROWS; ++j) {
        for(int i = 0; i < CELL_COLS; ++i) {
            if(cells[j][i] == 1)
            gc_draw_rectangle(
                memory,
                i*CELL_SIZE, j*CELL_SIZE,
                (i+1)*CELL_SIZE, (j+1)*CELL_SIZE,
                (map(i+j, 0, CELL_COLS + CELL_ROWS, 0, 255) << 16) + 
                (map(i+j, 0, CELL_COLS + CELL_ROWS, 205, 155) << 8) + 
                (map(i+j, 0, CELL_COLS + CELL_ROWS, 155, 205))
            );
        }
    }
}

void gc_put_pixel(void *memory, int32_t x, int32_t y, uint32_t color)
{
    if(in_bounds(x, y, WIDTH, HEIGHT))
        *((uint32_t *) memory + y * WIDTH + x) = color;
}

void gc_draw_rectangle(void *memory, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
    for(int y = y0; y <= y1; ++y) {
        for(int x = x0; x <= x1; ++x) {
            gc_put_pixel(memory, x, y, color);
        }
    }
}











