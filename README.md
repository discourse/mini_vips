# mini_vips

mini_vips packages the native libvips image operations used by [discourse/discourse](https://github.com/discourse/discourse). It supports letter-avatar generation and resizing, dominant-color extraction, and SVG-to-PNG conversion. It does not expose arbitrary libvips operations.

The platform gems link dynamically to libvips 8.13 or newer. Follow the [libvips building and installation instructions](https://github.com/libvips/libvips/wiki#building-and-installing) before using the helper. Precompiled gems are available for glibc Linux on x86-64 and ARM64, and macOS on ARM64.

## CLI

The packaged `libexec/mini_vips` executable accepts one operation followed by its options. The examples use `mini_vips` as the name of that file; the gem does not add it to `PATH`.

```text
mini_vips version
mini_vips letter-avatar --output PATH --markup TEXT --font NAME --fontfile PATH --red 0..255 --green 0..255 --blue 0..255
mini_vips resize-letter-avatar --input PATH --output PATH --size 1..4096
mini_vips dominant-color --input PATH
mini_vips svg-to-png --input PATH --output PATH
```

Commands with an `--output` option write the generated image there. `version` prints the linked libvips version, and `dominant-color` prints an uppercase six-character RGB value. The executable returns 0 on success, 1 when image processing fails, and 2 when the command is unsupported or a required value is missing or invalid. Errors are written to standard error.

## Development

Install [libvips](https://github.com/libvips/libvips/wiki#building-and-installing) and pkg-config first. Then set up the repository once by installing its Ruby development dependencies:

```sh
bundle install
```

Use the default task during development. It compiles the native helper, runs the tests, and checks Ruby formatting and style:

```sh
bundle exec rake
```
