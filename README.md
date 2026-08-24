# mini_vips

mini_vips packages the native libvips image operations used by [discourse/discourse](https://github.com/discourse/discourse). It supports letter-avatar generation and resizing, dominant-color extraction, and SVG-to-PNG conversion. It does not expose arbitrary libvips operations.

The platform gems link dynamically to libvips 8.13 or newer. Follow the [libvips building and installation instructions](https://github.com/libvips/libvips/wiki#building-and-installing) before using the helper. Precompiled gems are available for glibc Linux on x86-64 and ARM64, and macOS on ARM64.

## CLI

The packaged `libexec/mini_vips` executable accepts one command followed by its options. The examples use `mini_vips` as the name of that file; the gem does not add it to `PATH`.

### `version`

Print the linked libvips version:

```text
mini_vips version
```

### `letter-avatar`

Generate a 360 by 360 letter avatar with centered white text on the selected background color:

```text
mini_vips letter-avatar --letter TEXT --background-color RRGGBB --font-family NAME [--font-file PATH] --output PATH
```

- `--letter` sets the text shown in the avatar.
- `--background-color` sets the background using a six-character RGB value.
- `--font-family` selects the font family.
- `--font-file` optionally loads the family from a font file when it is not installed.
- `--output` sets the destination PNG path.

### `resize-letter-avatar`

Create a square letter avatar at the requested size:

```text
mini_vips resize-letter-avatar --input PATH --output PATH --size 1..4096
```

- `--input` sets the source letter-avatar PNG path.
- `--output` sets the destination PNG path.
- `--size` sets both output dimensions in pixels, from 1 through 4096.

### `dominant-color`

Print a representative color for a supported image as an uppercase six-character RGB value:

```text
mini_vips dominant-color --input PATH
```

- `--input` sets the source image path.

Supported inputs are JPEG, PNG, GIF, WebP, HEIF, and JPEG XL. Transparent pixels contribute black when calculating the color.

### `svg-to-png`

Convert an SVG into an opaque PNG:

```text
mini_vips svg-to-png --input PATH --output PATH
```

- `--input` sets the source SVG path.
- `--output` sets the destination PNG path.

Commands with an `--output` option write the generated image there. The executable returns 0 on success, 1 when image processing fails, and 2 when the command is unsupported or a required value is missing or invalid. Errors are written to standard error.

## Development

Install [libvips](https://github.com/libvips/libvips/wiki#building-and-installing) and pkg-config first. Then set up the repository once by installing its Ruby development dependencies:

```sh
bundle install
```

Use the default task during development. It compiles the native helper, runs the tests, and checks Ruby formatting and style:

```sh
bundle exec rake
```
