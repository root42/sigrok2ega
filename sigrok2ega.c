#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_surface.h>

#define IMG_WIDTH 420
#define IMG_HEIGHT 247

void pset( SDL_Surface *surface,
      unsigned int x,
      unsigned int y,
      unsigned char color
      )
{
  if( x >= IMG_WIDTH || y >= IMG_HEIGHT ) {
    return;
  }
  Uint32 pixel =
    (  ((color & (1 << 0)) << 1) + ((color & (1 << 3)) >> 3)) * 0x000055  // R
    + (((color & (1 << 1)) >> 0) + ((color & (1 << 3)) >> 3)) * 0x005500  // G
    + (((color & (1 << 2)) >> 1) + ((color & (1 << 3)) >> 3)) * 0x550000  // B
    ;

  Uint32 *target_pixel = surface->pixels + y * surface->pitch +
    x * sizeof *target_pixel;
  *target_pixel = pixel;
}

int main()
{
  int new_frame = 0;
  unsigned char value, color1;
  int hsync = 0, vsync = 0;
  int frame = 0;
  unsigned int vsyncc = 0, ref_len = 0;
  unsigned int x = 0, y = 0;
  SDL_Window *window;

  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow(
    "SIGROK2EGA",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    IMG_WIDTH,
    IMG_HEIGHT,
    SDL_WINDOW_SHOWN
    );

  // Check that the window was successfully created
  if (window == NULL) {
    // In the case that the window could not be made...
    printf("Could not create window: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Surface *surface =
    SDL_CreateRGBSurface(0, IMG_WIDTH, IMG_HEIGHT, 32, 0, 0, 0, 0);
  if (surface == NULL) {
    SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
    exit(1);
  }
  SDL_Surface *screen = SDL_GetWindowSurface(window);
  if (screen == NULL) {
    SDL_Log("SDL_GetWindowSurface() failed: %s", SDL_GetError());
    exit(1);
  }

  while(!feof(stdin)) {
    value = fgetc(stdin);
    vsync = value & 128;
    hsync = value & 64;
    if( vsync == 0 ) {
      vsyncc = 0;
      new_frame = 0;
    } else {
      vsyncc++;
      if( vsyncc > 20 ) {
	vsyncc = 0;
	y = 0;
	// this means a new frame?
	if( !new_frame ) {
	  new_frame = 1;
	  frame++;
	  printf("frame %u\n", frame);
	  // char fn[255];
	  // snprintf(fn, 255, "public_html/public/ega-%u.bmp", frame);
	  // SDL_SaveBMP(surface, fn);
	  SDL_BlitSurface(surface, NULL, screen, NULL);
	  SDL_UpdateWindowSurface(window);
	  // SDL_FillRect(surface, NULL, 0);
	  SDL_Event e;
	  if (SDL_PollEvent(&e)) {
	    if (e.type == SDL_QUIT) {
	      break;
	    }
	  }
	}
      }
    }
    color1 = value & 0x3F;
    int x_scaled = round(319.0/1070.0*(x-184));
    int y_scaled = y - 13;
    pset(surface, x_scaled, y_scaled, color1);
    x++;
    if( hsync ) {
      ref_len++;
      if( ref_len > 30 && x > 700 ) {
	pset(surface, x_scaled, y_scaled, 5);
	y++;
	x = 0;
      }
    } else {
      ref_len = 0;
    }
  }

  SDL_DestroyWindow( window );
  SDL_Quit();

  return 0;
}
