# Debian letter-avatar fixtures

These files are the exact outputs of `mini_vips letter-avatar A..Z` with the bundled Noto Sans font, default sizes, and background color `123456`.

They were generated on ARM64 on 2026-08-24 with `discourse/ruby@sha256:1c527d6bf18d15dbf1ce22726d6489838c62ea3cf19263386978d5794e2fbf68`. The image installed Debian Trixie packages for libvips 8.16.1-1+deb13u1, Pango 1.56.3-1, Cairo 1.18.4-1+b1, FreeType 2.13.3+dfsg-1+deb13u1, libpng 1.6.48-1+deb13u5, and zlib 1:1.3.dfsg+really1.3.1-1+b1.

Regenerate the fixtures inside that image after accepting an intentional rendering change:

```sh
script/build
for letter in A B C D E F G H I J K L M N O P Q R S T U V W X Y Z; do
  libexec/mini_vips letter-avatar "$letter" "test/fixtures/images/letter_avatars/debian/$letter.png" --background-color 123456
done
```
