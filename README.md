# mini_vips

mini_vips packages the native libvips image operations used by [discourse/discourse](https://github.com/discourse/discourse).

mini_vips requires libvips 8.13 or newer to be installed. Follow the [libvips building and installation instructions](https://github.com/libvips/libvips/wiki#building-and-installing) before using it. Precompiled versions of mini_vips are available for glibc Linux and macOS on x86-64 and ARM64.

## CLI

### `version`

Print the linked libvips version.

```text
$ mini_vips version
8.17.2
```

### `letter-avatar`

Generate a square letter-avatar PNG, 360 by 360 pixels by default.

```text
$ mini_vips letter-avatar A avatar.png --background-color C67D28
```

```text
usage:
   letter-avatar letter out [--size pixels] [--letter-size pixels] [--background-color color]

where:
   letter             - Text displayed in the avatar
   out                - Output PNG path

options:
   background-color   - Six-character RGB background color
                        optional, default: 000000
   size               - Output width and height in pixels
                        optional, default: 360
                        min: 1, max: 4096
   letter-size        - Letter size in pixels
                        optional, default: 7/9 of size
                        min: 1, max: 4096
```

Letter avatars use the bundled Noto Sans font.

### `resize`

Resize a supported image and select the output format from the output filename.

```text
$ mini_vips resize avatar.png avatar-90.png --width 90 --height 90 --fit cover
```

```text
usage:
   resize in out (--width pixels --height pixels | --scale ratio | --max-pixels pixels)
                 [--fit contain|cover] [--position center|top]
                 [--without-enlargement] [--quality 1..100]
                 [--colors count] [--strip-metadata]

where:
   in                    - Input image path
   out                   - Output image path

options:
   width                 - Bounding-box or cover width in pixels
   height                - Bounding-box or cover height in pixels
                          use together, min: 1, max: 65535
   scale                 - Proportional scale ratio
                          min: greater than 0, max: 100
   max-pixels            - Maximum output pixel area
                          min: 1; never enlarges the image
   fit                   - Fit within the dimensions or cover them exactly
                          optional, default: contain
   position              - Crop from the center or top when fit is cover
                          optional, default: center
   without-enlargement   - Keep an image within the requested dimensions at
                          its original size when it is already smaller
   quality               - Output encoder quality
                          optional, min: 1, max: 100; unavailable for GIF
   colors                - Maximum palette size for PNG or GIF output
                          optional, min: 2, max: 256; rounded down to the
                          nearest palette bit depth supported by the format
   strip-metadata        - Remove image metadata from the output
```

Supported inputs are JPEG, PNG, GIF, WebP, HEIF, JPEG XL, and SVG. Supported outputs are JPEG, PNG, GIF, WebP, HEIF/AVIF, and JPEG XL. Resize uses the first frame of animated inputs, applies image orientation, preserves transparency when the output format supports it, and sharpens only images produced with `--fit cover`.

### `dominant-color`

Print a representative color as an uppercase six-character RGB value.

```text
$ mini_vips dominant-color image.png
3A3730
```

```text
usage:
   dominant-color in

where:
   in                 - Input image path
```

Supported inputs are JPEG, PNG, GIF, WebP, HEIF, and JPEG XL. Transparent pixels contribute black when calculating the color.

### `convert`

Convert an SVG into an opaque PNG.

```text
$ mini_vips convert image.svg image.png
```

```text
usage:
   convert in out

where:
   in                 - Input SVG path
   out                - Output PNG path
```

The initial release supports SVG input and PNG output only.

## Exit status

The executable returns:

- `0` when the operation succeeds.
- `1` when image processing fails.
- `2` when the command or its arguments are invalid.

Errors are written to standard error.

## Development

Install [libvips](https://github.com/libvips/libvips/wiki#building-and-installing) and pkg-config first. Then set up the repository once by installing its Ruby development dependencies:

```sh
bundle install
```

Use the default task during development. It compiles the native helper, runs the tests, and checks Ruby formatting and style:

```sh
bundle exec rake
```

Build, install, and test the platform gem with:

```sh
bundle exec rake test:installed
```
