#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_surface.h>

#define IMG_WIDTH 380
#define IMG_HEIGHT 280

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
static const Uint32 rmask = 0xff000000;
static const Uint32 gmask = 0x00ff0000;
static const Uint32 bmask = 0x0000ff00;
static const Uint32 amask = 0x000000ff;
#else
static const Uint32 rmask = 0x000000ff;
static const Uint32 gmask = 0x0000ff00;
static const Uint32 bmask = 0x00ff0000;
static const Uint32 amask = 0xff000000;
#endif

static Uint32 egapal[64];

static int comp(const void* a, const void* b)
{
  return *(Uint32 *)a < *(Uint32 *)b;
}

void
init_pal()
{
  int i;
  for( i = 0; i < 64; ++i ) {
    Uint32 color =
      (  ((i & (1 << 0)) << 1) + ((i & (1 << 3)) >> 3)) * (0x55555555 & bmask)
      + (((i & (1 << 1)) >> 0) + ((i & (1 << 3)) >> 3)) * (0x55555555 & gmask)
      + (((i & (1 << 2)) >> 1) + ((i & (1 << 3)) >> 3)) * (0x55555555 & rmask)
      + amask
      ;
    printf("%xd ", color);
    egapal[i] = color;
  }
}

inline
void
pset(
  SDL_Surface *surface,
  unsigned int x,
  unsigned int y,
  unsigned char color
  )
{
  if( x >= IMG_WIDTH || y >= IMG_HEIGHT ) {
    return;
  }

  Uint32 *target_pixel = surface->pixels + y * surface->pitch +
    x * sizeof( *target_pixel );
  *target_pixel = egapal[color];
}

int main()
{
  int new_frame = 0;
  int hsync = 0, vsync = 0;
  int frame = 0;
  unsigned int vsyncc = 0, ref_len = 0;
  unsigned int x = 0;
  unsigned int y = 0;
  const double sclk = 24000000;
  //double pclk = 0.298130841121; //0.298295416667;
  //int pclk = 7155140;
  double pclk = 7159090;
  //int pclk = 14318180;
  double phase = 0;
  double erracc = sclk;
  SDL_Window *window;

  const int MEDIAN_SIZE = 5;
  Uint32 median[MEDIAN_SIZE];
  int sample_number = 0;

  init_pal();
  
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow(
    "SIGROK2EGA",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    IMG_WIDTH * 2,
    IMG_HEIGHT * 2,
    SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE
    );

  if (window == NULL) {
    SDL_Log( "Could not create window: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ren == NULL) {
    SDL_DestroyWindow( window );
    SDL_Log( "SDL_CreateRenderer Error: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Surface *surface =
    SDL_CreateRGBSurface(
      0,
      IMG_WIDTH,
      IMG_HEIGHT,
      32,
      rmask,
      gmask,
      bmask,
      amask
      );
  if (surface == NULL) {
    SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surface);
  if( tex == NULL ) {
    SDL_Log( "SDL_CreateTextueFromSurface() failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  while(!feof(stdin)) {
    unsigned char value, color1;
    value = getchar_unlocked(); // fgetc(stdin);
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
	  //printf("frame %u\n", frame);

	  SDL_UpdateTexture(tex, NULL, surface->pixels, surface->pitch);
	  SDL_RenderClear(ren);
	  SDL_RenderCopy(ren, tex, NULL, NULL);
	  SDL_RenderPresent(ren);

	  SDL_Event e;
	  if (SDL_PollEvent(&e)) {
	    if (e.type == SDL_QUIT) {
	      break;
	    } else if( e.type == SDL_KEYDOWN ) {
	      int old_phase = phase;
	      int old_pclk = pclk;
	      switch( e.key.keysym.sym )
	      {
	      case SDLK_LEFT:
		phase-=0.1000000;
		break;
	      case SDLK_RIGHT:
		phase+=0.1000000;
		break;
	      case SDLK_UP:
		pclk-=0.0001;
		break;
	      case SDLK_DOWN:
		pclk+=0.0001;
		break;
	      default:
		break;
	      }
	      if( old_phase != phase ) {
		printf("phase %f\n",phase);
	      }
	      if( old_pclk != pclk ) {
		printf("pclk %f\n",pclk);
	      }
	    }
	  }
	}
      }
    }
    color1 = value & 0x3F;
    median[ sample_number % MEDIAN_SIZE ] = color1;
    sample_number++;
    if( erracc < 0 ) {
      erracc += sclk;
      Uint32 median2[ MEDIAN_SIZE ] = {
	median[ 0 ],
	median[ 1 ],
	median[ 2 ],
	median[ 3 ],
	median[ 4 ]
      };
      qsort(median2, MEDIAN_SIZE, sizeof(Uint32), comp);
      pset(surface, x, y, median2[MEDIAN_SIZE / 2]);
      //pset(surface, x, y, color1);
      x++;
    } else {
      erracc -= pclk;
    }
    if( hsync ) {
      ref_len++;
      if( ref_len > 30 && x > 208 ) {
	pset(surface, x, y-13, 5);
	pset(surface, x+1, y-13, 0);
	y++;
	erracc = sclk + phase;
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
