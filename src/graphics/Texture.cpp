//! --------------------------------------------------------------------------
//! -------------------------- INCLUDES
//! --------------------------------------------------------------------------

#include "Texture.hpp"

#include "SDL.h"                    // Needed for IMG_Load
#include "SDL_image.h"

#include "opengl.h"                 // Needed for OpenGL/GLES
#include "../debug/assert.h"        // Needed for ASSERT macro
#include "../debug/warn.h"
#include "../math/wjd_math.h"       // Needed for ISPWR2
#include "../global.hpp"

//! --------------------------------------------------------------------------
//! -------------------------- CONSTRUCTORS, DESTRUCTOR
//! --------------------------------------------------------------------------

Texture::Texture() :
handle(0),
loaded(false)
{
}

int Texture::load(const char* filepath)
{
  // Free any previous content
  if(loaded)
    unload();

  // Load the image using SDL_image
  SDL_Surface* surface = IMG_Load(filepath);

  ASSERT_SDL(surface, "Opening image file");

  // continue working from this surface
  int result = this->from_surface(surface);

  // Be sure to delete the bitmap from CPU memory before returning the result!
  SDL_FreeSurface(surface);

  return result;
}

int Texture::from_surface(SDL_Surface* surface)
{
  // Free any previous content
  if(loaded)
    unload();

  // Local variables for extracting properties about the image
  GLuint n_colours = 0;
  GLenum format = (GLenum) NULL;

  // Make sure the image length and width are powers of 2
	area = iRect(0, 0,
                ISPWR2(surface->w) ? surface->w : nextpwr2(surface->w),
                ISPWR2(surface->h) ? surface->h : nextpwr2(surface->h));
  if(area.w != surface->w || area.h != surface->h)
  {
    // enlarge the surface if needed
    SDL_Surface* previous_surface = surface;
    // NB - Hexadecimal parameters are: Rmask, Gmask, Bmask and Amask
    surface = SDL_CreateRGBSurface(0, area.w, area.h, 32,
                                0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    // past contents of previous (small) surface onto new (larger) surface
    SDL_BlitSurface(previous_surface, 0, surface, 0);
  }

  //get number of channels in the SDL surface
  n_colours = surface->format->BytesPerPixel;
  switch(n_colours)
  {
      case 1: format = GL_LUMINANCE;          break;
      case 2: format = GL_LUMINANCE_ALPHA;    break;
      case 3: format = GL_RGB;                break;
      case 4: format = GL_RGBA;               break;
      default:
        log(LOG_ERROR, "Load texture failed : %d colours Image must be LUMINANCE, RGB or RGBA");
        return EXIT_FAILURE;
       break;
  }

  // Request an OpenGL unassigned GLuint to identify this texture
  glGenTextures(1, &handle);

  // Bind the texture object to the current block
  glBindTexture(GL_TEXTURE_2D, handle);

  // Set the texture’s properties
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Finally: convert the image to a texture
  glTexImage2D(GL_TEXTURE_2D, 0, n_colours, area.w, area.h, 0,
                  format, GL_UNSIGNED_BYTE, surface->pixels);

  // Unbind the texture
  glBindTexture(GL_TEXTURE_2D, 0);

  // The return result reports the success of the operation
  loaded = true;
  return EXIT_SUCCESS;
}

int Texture::unload()
{
  if(!loaded)
    WARN_RTN("Texture::unload()", "Texture is not loaded!", EXIT_SUCCESS);

  // Free the texture from video memory
  glDeleteTextures(1, &handle);

  // Success !
  return EXIT_SUCCESS;
}

Texture::~Texture()
{
  // don't force unload here: this may not be the only copy of the handle!
}

//! --------------------------------------------------------------------------
//! -------------------------- ACCESSORS
//! --------------------------------------------------------------------------

iRect Texture::getArea() const
{
  return area;
}

GLuint Texture::getHandle() const
{
  return handle;
}

void Texture::draw(const fRect* src_ptr, const fRect* dst_ptr, float angle)
{
  glPushMatrix();

  // Crop the source rectangle if necessary
  fRect src(area);
  if(src_ptr) // if no source is given the full texture is used!
    src = src_ptr->getInter(area);

  // Crop the destination rectangle if necessary
  fRect dst(global::viewport);
  if(dst_ptr) // if no destination is given the full viewport is used!
    dst = (*dst_ptr);

  // Set up position, rotation, colour
  glTranslatef(global::scale.x*(dst.x + dst.w/2),
                global::scale.y*(dst.y + dst.h/2), 0.0);
  glRotatef(angle, 0.0, 0.0, 1.0);
  glScalef(global::scale.x, global::scale.y, 0.0f);

  // Bind the texture to which subsequent calls refer to
  glBindTexture(GL_TEXTURE_2D, handle);

  // Tell graphics hardware what to expect
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  // Set up the polygon in which we'll draw
  GLfloat
  min_x = -dst.w/2,
  min_y = -dst.h/2,
  max_x = -min_x,
  max_y = -min_y;
  GLfloat polygon[8]  =   {min_x, min_y,      // Top-left
                          max_x,  min_y,      // Top-right
                          min_x,  max_y,      // Bottom-left
                          max_x,  max_y };    // Bottom-right
  glVertexPointer(2, GL_FLOAT, 0, polygon);

  // Set up the binding of the skin (texture) to this polygon
  min_x = src.x/area.w;
  min_y = src.y/area.h;
  max_x = (src.x + src.w)/area.w;
  max_y = (src.y + src.h)/area.h;
  GLfloat skin[8]     =    {min_x, min_y,      // Top-left
                            max_x,  min_y,      // Top-right
                            min_x,  max_y,      // Bottom-left
                            max_x,  max_y };    // Bottom-right
  glTexCoordPointer(2, GL_FLOAT, 0, skin);

  // Draw everything (finally)!
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Stop drawing texture
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  // Reset back to normal
  glPopMatrix();
  glBindTexture(GL_TEXTURE_2D, 0);
}
