#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#define FONT_SIZE 32

int main (int argc, char *argv[]) {

	if (argc != 3) {
		fprintf(stderr, "usage: %s font-file.ttf text\n", argv[0]);
		exit(1);
	}

	// Load fonts
	cairo_font_face_t *cairo_ft_face;
	hb_font_t *hb_ft_font;
	{
		// For CAIRO, load using FreeType
		FT_Library ft_library;
		assert(FT_Init_FreeType(&ft_library) == 0);
		FT_Face ft_face;
		assert(FT_New_Face(ft_library, argv[1], 0, &ft_face) == 0);
		cairo_ft_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);

		// For Harfbuzz, load using OpenType (HarfBuzz FT does not support bitmap font)
		hb_blob_t *blob = hb_blob_create_from_file(argv[1]);
		hb_face_t *face = hb_face_create (blob, 0);
		hb_ft_font = hb_font_create (face);
		hb_ot_font_set_funcs(hb_ft_font);
		hb_font_set_scale(hb_ft_font, FONT_SIZE*64, FONT_SIZE*64);
	}

	// Create  HarfBuzz buffer
	hb_buffer_t *buf = hb_buffer_create();

	// Set buffer to LTR direction, common script and default language
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(buf, HB_SCRIPT_COMMON);
	hb_buffer_set_language(buf, hb_language_get_default());

	// Add text and layout it
	hb_buffer_add_utf8(buf, argv[2], -1, 0, -1);
	hb_shape(hb_ft_font, buf, NULL, 0);

	// Get buffer data
	unsigned int        glyph_count = hb_buffer_get_length (buf);
	hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, NULL);
	hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, NULL);

	unsigned int string_width_in_pixels = 0;
	for (int i = 0; i < glyph_count; ++i) {
		string_width_in_pixels += glyph_pos[i].x_advance / 64.0;
	}

	printf("glyph_count=%d\n", glyph_count);
	printf("string_width=%d \n", string_width_in_pixels);

	// Shape glyph for Cairo
	cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate(glyph_count);
	int x = 0;
	int y = 0;
	for (int i = 0 ; i < glyph_count ; ++i) {
		cairo_glyphs[i].index = glyph_info[i].codepoint;
		cairo_glyphs[i].x = x + (glyph_pos[i].x_offset / (64.0));
		cairo_glyphs[i].y = -(y + glyph_pos[i].y_offset / (64.0));
		x += glyph_pos[i].x_advance / (64.0);
		y += glyph_pos[i].y_advance / (64.0);

		printf("glyph_codepoint=%lu size=(%g, %g) advance=(%g, %g)\n", cairo_glyphs[i].index, glyph_pos[i].x_advance/64.0, glyph_pos[i].y_advance/64.0, glyph_pos[i].x_advance/64.0, glyph_pos[i].y_advance/64.0);
	}

	// Draw text in SDL2 with Cairo
	int width      = 200;
	int height     = 100;
	SDL_WindowFlags videoFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

	SDL_Window* window = NULL;

	assert(SDL_Init(SDL_INIT_VIDEO) == 0);

	window = SDL_CreateWindow("harfbuzz + sdl2 + cairo + freetype", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,width, height, videoFlags);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Move glyph to be on window middle
	for (int i = 0 ; i < glyph_count ; ++i) {
		cairo_glyphs[i].x += width / 2 - string_width_in_pixels / 2;
		cairo_glyphs[i].y += height / 2;
	}

	// Compute screen resolution
	// For instance, on a retina screen, renderer size is twice as window size
	int window_width;
	int window_height;
	SDL_GetWindowSize(window, &window_width, &window_height);

	int renderer_width;
	int renderer_height;
	SDL_GetRendererOutputSize(renderer, &renderer_width, &renderer_height);

	int cairo_x_multiplier = renderer_width / window_width;
	int cairo_y_multiplier = renderer_height / window_height;

	// Create a SDL surface for Cairo to render onto
	SDL_Surface *sdl_surface = SDL_CreateRGBSurface (
			0,
			renderer_width,
			renderer_height,
			32,
			0x00ff0000,
			0x0000ff00,
			0x000000ff,
			0
	);

	// Get Cairo surface form SDL2 surface
	cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data (
			(unsigned char *)sdl_surface->pixels,
			CAIRO_FORMAT_RGB24,
			sdl_surface->w,
			sdl_surface->h,
			sdl_surface->pitch);

	// Scale cairo to use screen resolution
	cairo_surface_set_device_scale(cairo_surface, cairo_x_multiplier, cairo_y_multiplier);

	// Get Cairo context from Cairo surface
	cairo_t *cairo_context = cairo_create(cairo_surface);
	cairo_set_source_rgba (cairo_context, 0, 0, 0, 1.0);
	cairo_set_font_face(cairo_context, cairo_ft_face);
	cairo_set_font_size(cairo_context, FONT_SIZE);

	int done = 0;
    while (done == 0) {
    	// Fill background in white
        SDL_FillRect(sdl_surface, NULL, SDL_MapRGB(sdl_surface->format, 255, 255, 255));

        // Render glyph onto cairo context (which render onto SDL2 surface)
        cairo_show_glyphs(cairo_context, cairo_glyphs, glyph_count);

        // Render SDL2 surface onto SDL2 renderer
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, sdl_surface);
		SDL_RenderCopy(renderer, texture, 0, 0);
		SDL_RenderPresent(renderer);

        // Quit app on close event
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            switch (event.type) {
            	case SDL_QUIT:
            		done = 1;
            		break;

	            default:break;
            }
        }
    }

    // Free stuff
    free(cairo_glyphs);
	cairo_surface_destroy(cairo_surface);
	cairo_destroy(cairo_context);
    cairo_font_face_destroy(cairo_ft_face);
    hb_font_destroy(hb_ft_font);
    SDL_FreeSurface(sdl_surface);

    SDL_Quit();
    return 0;
}
