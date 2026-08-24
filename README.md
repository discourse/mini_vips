# mini_vips

mini_vips packages a small native helper for fixed-purpose image operations powered by libvips. It supports letter-avatar generation and resizing, dominant-color extraction, and topic Open Graph image rendering. It does not expose arbitrary libvips operations.

The platform gems link dynamically to libvips 8.13 or newer. Install libvips with your operating system package manager before using the helper. The GNU builds target Debian 12 and glibc 2.36. The musl builds target Alpine 3.21 and musl 1.2.5. The macOS builds set a macOS 13.0 deployment target.

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

Install libvips and pkg-config. Installing a musl build requires Bundler 2.5.6 or newer. Then run:

```sh
bundle install
bundle exec rake compile
bundle exec rake
MINI_VIPS_PLATFORM=arm64-darwin bundle exec rake build
```
