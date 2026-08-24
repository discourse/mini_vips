# mini_vips

mini_vips packages the native libvips image operations used by [discourse/discourse](https://github.com/discourse/discourse). It supports letter-avatar generation and resizing, dominant-color extraction, and topic Open Graph image rendering. It does not expose arbitrary libvips operations.

The platform gems link dynamically to libvips 8.13 or newer. Follow the [libvips building and installation instructions](https://github.com/libvips/libvips/wiki#building-and-installing) before using the helper. Precompiled gems are available for glibc Linux on x86-64 and ARM64, and macOS on ARM64.

## Usage

Add the gem to your bundle:

```ruby
gem "mini_vips"
```

Resolve the packaged helper when configuring the process runner owned by your application:

```ruby
require "mini_vips"

MiniVips.executable
```

mini_vips only resolves its executable. It does not run commands or configure a sandbox.

## Development

Install [libvips](https://github.com/libvips/libvips/wiki#building-and-installing) and pkg-config first. Then set up the repository once by installing its Ruby development dependencies:

```sh
bundle install
```

Use the default task during development. It compiles the native helper, runs the tests, and checks Ruby formatting and style:

```sh
bundle exec rake
```
