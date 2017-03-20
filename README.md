audit-normalmap is a tool to check internal consistency of normal map image
files.

### Normalmaps

Normal mapping is a technique to simulate surfaces with bumps of dents in 3D
computer graphics. It works by storing additional per-pixel information for
material textures: a single vector, indicating a normal protruding
perpendicularly from the surface.

This information then can be used from a shader script using an equation like:

```
color *= dot3(lightdirection, normalize(texture(normalmap, texcoord)));
```

which will essentially darken surfaces more, the more their normal points away
from the light source.

(Note: typically, normalmaps will be encoded in tangentspace, so for above
 formula, lightdirection needs to be computed in tangentspace first too.)

As the normalmap contains information about surface steepness at each pixel,
some consistency expectations can be made - e.g. one would want to assume that
a normalmap corresponds to some heightmap, which essentially means that
integrating the gradient (which is implied from the normal vectors) across any
closed path must yield zero.

Furthermore, commonly normalmaps contain height information in the alpha
channel; this also should usually be consistent with the normal vectors.

This tool verifies these consistency properties of normalmaps to aid with
quality control of gaming artwork. It supports both the common Direct3D and
OpenGL Y axis conventions (a.k.a. green channel inversion).

### Usage

```
git submodule update --remote
make
./audit-normalmap infile.tga
```

Options:

* `-o output.hdr`: additional to a text report, output an image file
  highlighting the regions of the image that contain inconsistent normals.
  * Red channel: inconsistencies with heightmap (if present).
  * Green channel: denormal vectors.
  * Blue channel: internal inconsistencies (M.C.Escher).
* `-c`: do not show global inconsistencies that can be fixed by scaling the
  image (requires `-o`).
* `-y`: assume the y coordinate is inverted (i.e. assume the 0,0 coordinate of
  the image is at the bottom left, as opposed to the top left which is default;
  this is also known as green channel inversion). This matches most OpenGL based
  game engines. By default, the y coordinate is interpreted according to the
  usual convention of Direct3D based game engines.
* `-7`: assume the range of the normal vector encoding is centered at 127; in
  this case the "flat" normal vector (0,0,1) would be encoded as (127,127,255).
* `-8`: assume the range of the normal vector encoding is centered at 128; in
  this case the "flat" normal vector (0,0,1) would be encoded as (128,128,255).
* `-n`: assume `GL_NEAREST` normalmap interpolation. By default `GL_LINEAR` will
  be assumed as that is what will typically happen when rendering in 3D game
  engines.

### Copyright

See the `LICENSE` file.

### Contributing

See the `CONTRIBUTING.md` file.

### Disclaimer

This is not an official Google product.
