# mini_vips

mini_vips packages the native libvips image operations used by [discourse/discourse](https://github.com/discourse/discourse). It supports letter-avatar generation and resizing, dominant-color extraction, and SVG-to-PNG conversion. It does not expose arbitrary libvips operations.

The platform gems link dynamically to libvips 8.13 or newer. Follow the [libvips building and installation instructions](https://github.com/libvips/libvips/wiki#building-and-installing) before using the helper. Precompiled gems are available for glibc Linux and macOS on x86-64 and ARM64.

## CLI

The packaged `libexec/mini_vips` executable follows the libvips CLI convention:

```text
mini_vips OPERATION REQUIRED-ARGUMENTS [--option-name option-value ...]
```

The examples use `mini_vips` as the name of the packaged file. The gem does not add it to `PATH`.

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
   letter-avatar letter out --background-color color [--size pixels] [--letter-size pixels]

where:
   letter             - Text displayed in the avatar
   out                - Output PNG path

options:
   background-color   - Six-character RGB background color
                        required
   size               - Output width and height in pixels
                        optional, default: 360
                        min: 1, max: 4096
   letter-size        - Letter size in pixels
                        optional, default: 7/9 of size
                        min: 1, max: 4096
```

Letter avatars use the bundled Noto Sans font. The font is distributed under the SIL Open Font License 1.1 included in the platform gem.

### `resize-png`

Create a square PNG at the requested size.

```text
$ mini_vips resize-png avatar.png avatar-90.png --size 90
```

```text
usage:
   resize-png in out --size pixels

where:
   in                 - Input PNG path
   out                - Output PNG path

options:
   size               - Output width and height in pixels
                        required
                        min: 1, max: 4096
```

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

### `svg-to-png`

Convert an SVG into an opaque PNG.

```text
$ mini_vips svg-to-png image.svg image.png
```

```text
usage:
   svg-to-png in out

where:
   in                 - Input SVG path
   out                - Output PNG path
```

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
