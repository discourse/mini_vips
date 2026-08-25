#include <glib.h>
#include <glib/gstdio.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vips/vips.h>

typedef struct {
  const char *key;
  const char *value;
} option_t;

typedef struct {
  option_t items[32];
  int count;
} options_t;

static const char *option(options_t *options, const char *key) {
  for (int index = 0; index < options->count; index++) {
    if (strcmp(options->items[index].key, key) == 0) {
      return options->items[index].value;
    }
  }
  return NULL;
}

static gboolean has_option(options_t *options, const char *key) {
  return option(options, key) != NULL;
}

static int integer_option(options_t *options, const char *key, int fallback) {
  const char *value = option(options, key);
  if (!value) {
    return fallback;
  }

  char *end = NULL;
  long parsed = strtol(value, &end, 10);
  if (!end || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
    fprintf(stderr, "invalid integer for --%s\n", key);
    exit(2);
  }
  return (int)parsed;
}

static double double_option(options_t *options, const char *key,
                            double fallback) {
  const char *value = option(options, key);
  if (!value) {
    return fallback;
  }

  char *end = NULL;
  double parsed = g_ascii_strtod(value, &end);
  if (!end || *end != '\0' || !isfinite(parsed)) {
    fprintf(stderr, "invalid number for --%s\n", key);
    exit(2);
  }
  return parsed;
}

static void color_option(options_t *options, const char *key, int channels[3]) {
  const char *value = option(options, key);
  value = value ? value : "000000";
  if (strlen(value) != 6) {
    fprintf(stderr, "--%s must be a six-character RGB value\n", key);
    exit(2);
  }

  for (int index = 0; index < 3; index++) {
    int high = g_ascii_xdigit_value(value[index * 2]);
    int low = g_ascii_xdigit_value(value[index * 2 + 1]);
    if (high < 0 || low < 0) {
      fprintf(stderr, "--%s must be a six-character RGB value\n", key);
      exit(2);
    }
    channels[index] = high * 16 + low;
  }
}

static const char *required_argument(int argc, char **argv, int index,
                                     const char *name) {
  if (index >= argc || strncmp(argv[index], "--", 2) == 0) {
    fprintf(stderr, "missing required argument %s\n", name);
    exit(2);
  }
  return argv[index];
}

static void parse_options(int argc, char **argv, int start,
                          options_t *options) {
  options->count = 0;
  for (int index = start; index < argc; index++) {
    if (strncmp(argv[index], "--", 2) != 0) {
      fprintf(stderr, "unexpected argument: %s\n", argv[index]);
      exit(2);
    }
    if (options->count >= 32) {
      fprintf(stderr, "too many options\n");
      exit(2);
    }
    const char *key = argv[index] + 2;
    for (int option_index = 0; option_index < options->count; option_index++) {
      if (strcmp(options->items[option_index].key, key) == 0) {
        fprintf(stderr, "duplicate option --%s\n", key);
        exit(2);
      }
    }
    options->items[options->count].key = key;
    if (strcmp(key, "without-enlargement") == 0 ||
        strcmp(key, "strip-metadata") == 0) {
      options->items[options->count].value = "1";
    } else {
      if (index + 1 >= argc) {
        fprintf(stderr, "missing value for %s\n", argv[index]);
        exit(2);
      }
      options->items[options->count].value = argv[++index];
    }
    options->count++;
  }
}

static void validate_options(options_t *options, const char **allowed,
                             int allowed_count) {
  for (int index = 0; index < options->count; index++) {
    gboolean valid = FALSE;
    for (int allowed_index = 0; allowed_index < allowed_count;
         allowed_index++) {
      if (strcmp(options->items[index].key, allowed[allowed_index]) == 0) {
        valid = TRUE;
        break;
      }
    }
    if (!valid) {
      fprintf(stderr, "unsupported option --%s\n", options->items[index].key);
      exit(2);
    }
  }
}

static void report_error(const char *message) {
  fprintf(stderr, "%s\n", message);
}

static void report_vips_error(void) {
  const char *message = vips_error_buffer();
  report_error(message && message[0] ? message : "libvips error");
}

static void block_loaders(void) {
  vips_block_untrusted_set(TRUE);
  vips_operation_block_set("VipsForeignLoad", TRUE);
  vips_operation_block_set("VipsForeignLoadMagick", TRUE);
  vips_operation_block_set("VipsForeignLoadMagick6", TRUE);
  vips_operation_block_set("VipsForeignLoadMagick7", TRUE);
}

static void allow_loader_family(const char *base) {
  vips_operation_block_set(base, FALSE);
}

static void allow_dominant_color_loaders(void) {
  allow_loader_family("VipsForeignLoadJpeg");
  allow_loader_family("VipsForeignLoadPng");
  allow_loader_family("VipsForeignLoadNsgif");
  allow_loader_family("VipsForeignLoadWebp");
  allow_loader_family("VipsForeignLoadHeif");
  allow_loader_family("VipsForeignLoadJxl");
}

static void allow_resize_loaders(void) {
  allow_dominant_color_loaders();
  allow_loader_family("VipsForeignLoadSvg");
}

static int initialize_vips(const char *program) {
  if (VIPS_INIT(program)) {
    return -1;
  }
  if (vips_version(0) < 8 || (vips_version(0) == 8 && vips_version(1) < 13)) {
    vips_error("mini-vips", "libvips >= 8.13 is required (found %d.%d)",
               vips_version(0), vips_version(1));
    return -1;
  }
  block_loaders();
  return 0;
}

static unsigned char quantize_sample(double value, double sample_max) {
  value = fmin(sample_max, fmax(0.0, value));
  return (unsigned char)floor(value * 255.0 / sample_max + 0.5);
}

static int command_version(void) {
  printf("%d.%d.%d\n", vips_version(0), vips_version(1), vips_version(2));
  return 0;
}

static char *bundled_font_path(const char *program) {
  char *resolved_program = g_find_program_in_path(program);
  char *program_directory =
      g_path_get_dirname(resolved_program ? resolved_program : program);
  char *font_path =
      g_build_filename(program_directory, "..", "lib", "mini_vips", "fonts",
                       "NotoSans-Regular.ttf", NULL);
  g_free(resolved_program);
  g_free(program_directory);
  return font_path;
}

static int command_letter_avatar(const char *letter, const char *output,
                                 const char *program, options_t *options) {
  int size = integer_option(options, "size", 360);
  if (size < 1 || size > 4096) {
    report_error("--size must be 1..4096");
    return 2;
  }
  int letter_size = integer_option(options, "letter-size", (size * 7 + 4) / 9);
  if (letter_size < 1 || letter_size > 4096) {
    report_error("--letter-size must be 1..4096");
    return 2;
  }

  int background_channels[3];
  color_option(options, "background-color", background_channels);
  if (vips_type_find("VipsOperation", "text") == 0) {
    report_error("libvips has no Pango text renderer");
    return 1;
  }

  char *escaped_letter = g_markup_escape_text(letter, -1);
  char *markup = g_strdup_printf(
      "<span foreground=\"#ffffff\" alpha=\"80%%\">%s</span>",
      escaped_letter);
  char *font = g_strdup_printf("Noto Sans %d", letter_size);
  char *font_file = bundled_font_path(program);
  VipsImage *text = NULL;
  VipsImage *canvas = NULL;
  VipsImage *flattened = NULL;
  double canvas_background_values[4] = {
      background_channels[0], background_channels[1], background_channels[2],
      255};
  double flattened_background_values[3] = {
      background_channels[0], background_channels[1], background_channels[2]};
  VipsArrayDouble *canvas_background =
      vips_array_double_new(canvas_background_values, 4);
  VipsArrayDouble *flattened_background =
      vips_array_double_new(flattened_background_values, 3);

  int result = vips_text(&text, markup, "font", font, "dpi", 72, "fontfile",
                         font_file, "rgba", TRUE, NULL);
  if (result == 0) {
    result = vips_gravity(text, &canvas, VIPS_COMPASS_DIRECTION_CENTRE, size,
                          size, "extend", VIPS_EXTEND_BACKGROUND, "background",
                          canvas_background, NULL);
  }
  if (result == 0) {
    result = vips_flatten(canvas, &flattened, "background",
                          flattened_background, NULL);
  }
  if (result == 0) {
    result = vips_pngsave(flattened, output, "compression", 6, NULL);
  }
  if (result != 0) {
    report_vips_error();
  }
  VIPS_UNREF(text);
  VIPS_UNREF(canvas);
  VIPS_UNREF(flattened);
  g_free(escaped_letter);
  g_free(markup);
  g_free(font);
  g_free(font_file);
  vips_area_unref((VipsArea *)canvas_background);
  vips_area_unref((VipsArea *)flattened_background);
  return result == 0 ? 0 : 1;
}

static int load_oriented(const char *input, VipsImage **oriented) {
  VipsImage *source = vips_image_new_from_file(
      input, "access", VIPS_ACCESS_SEQUENTIAL, "fail_on", VIPS_FAIL_ON_NONE,
      NULL);
  int result = source ? vips_autorot(source, oriented, NULL) : -1;
  VIPS_UNREF(source);
  return result;
}

static gboolean valid_input_path(const char *input) {
  char *basename = g_path_get_basename(input);
  gboolean valid = !strpbrk(basename, "[]");
  g_free(basename);
  return valid;
}

static char *temporary_output(const char *output) {
  char *directory = g_path_get_dirname(output);
  char *basename = g_path_get_basename(output);
  char *uuid = g_uuid_string_random();
  char *temporary =
      g_strdup_printf("%s/.mini-vips-%s-%s", directory, uuid, basename);
  g_free(directory);
  g_free(basename);
  g_free(uuid);
  return temporary;
}

static const char *output_extension(const char *output) {
  const char *extension = strrchr(output, '.');
  return extension ? extension + 1 : "";
}

typedef enum {
  OUTPUT_UNSUPPORTED,
  OUTPUT_JPEG,
  OUTPUT_PNG,
  OUTPUT_GIF,
  OUTPUT_WEBP,
  OUTPUT_HEIF,
  OUTPUT_AVIF,
  OUTPUT_JXL,
} output_format_t;

static output_format_t output_format(const char *output) {
  const char *extension = output_extension(output);
  if (g_ascii_strcasecmp(extension, "jpg") == 0 ||
      g_ascii_strcasecmp(extension, "jpeg") == 0) {
    return OUTPUT_JPEG;
  }
  if (g_ascii_strcasecmp(extension, "png") == 0) {
    return OUTPUT_PNG;
  }
  if (g_ascii_strcasecmp(extension, "gif") == 0) {
    return OUTPUT_GIF;
  }
  if (g_ascii_strcasecmp(extension, "webp") == 0) {
    return OUTPUT_WEBP;
  }
  if (g_ascii_strcasecmp(extension, "heif") == 0 ||
      g_ascii_strcasecmp(extension, "heic") == 0) {
    return OUTPUT_HEIF;
  }
  if (g_ascii_strcasecmp(extension, "avif") == 0) {
    return OUTPUT_AVIF;
  }
  if (g_ascii_strcasecmp(extension, "jxl") == 0) {
    return OUTPUT_JXL;
  }
  return OUTPUT_UNSUPPORTED;
}

static int palette_bitdepth(int colors, gboolean gif, gboolean has_alpha) {
  int selected = 0;
  int png_bitdepths[] = {1, 2, 4, 8};
  int count = gif ? 8 : (int)G_N_ELEMENTS(png_bitdepths);
  for (int index = 0; index < count; index++) {
    int bitdepth = gif ? index + 1 : png_bitdepths[index];
    int capacity = (1 << bitdepth) - (gif && has_alpha ? 1 : 0);
    if (capacity <= colors) {
      selected = bitdepth;
    }
  }
  return selected;
}

static int save_image(VipsImage *image, const char *input, const char *output,
                      output_format_t format, int quality, int colors,
                      gboolean strip_metadata) {
  char *save_path = strcmp(input, output) == 0 ? temporary_output(output) : NULL;
  const char *destination = save_path ? save_path : output;
  int result;
  if (format == OUTPUT_PNG && colors > 0) {
    int bitdepth = palette_bitdepth(colors, FALSE, vips_image_hasalpha(image));
    result = quality > 0
                 ? vips_pngsave(image, destination, "palette", TRUE, "bitdepth",
                                bitdepth, "Q", quality, "strip",
                                strip_metadata, NULL)
                 : vips_pngsave(image, destination, "palette", TRUE, "bitdepth",
                                bitdepth, "strip", strip_metadata, NULL);
  } else if (format == OUTPUT_PNG) {
    result = quality > 0
                 ? vips_pngsave(image, destination, "Q", quality, "strip",
                                strip_metadata, NULL)
                 : vips_pngsave(image, destination, "strip", strip_metadata,
                                NULL);
  } else if (format == OUTPUT_GIF) {
    int bitdepth = palette_bitdepth(colors, TRUE, vips_image_hasalpha(image));
    result = colors > 0
                 ? vips_gifsave(image, destination, "bitdepth", bitdepth,
                                "strip", strip_metadata, NULL)
                 : vips_gifsave(image, destination, "strip", strip_metadata,
                                NULL);
  } else if (format == OUTPUT_JPEG) {
    result = quality > 0
                 ? vips_jpegsave(image, destination, "Q", quality, "strip",
                                 strip_metadata, NULL)
                 : vips_jpegsave(image, destination, "strip", strip_metadata,
                                 NULL);
  } else if (format == OUTPUT_WEBP) {
    result = quality > 0
                 ? vips_webpsave(image, destination, "Q", quality, "strip",
                                 strip_metadata, NULL)
                 : vips_webpsave(image, destination, "strip", strip_metadata,
                                 NULL);
  } else if (format == OUTPUT_JXL) {
    result = quality > 0
                 ? vips_jxlsave(image, destination, "Q", quality, "strip",
                                strip_metadata, NULL)
                 : vips_jxlsave(image, destination, "strip", strip_metadata,
                                NULL);
  } else {
    VipsForeignHeifCompression compression =
        format == OUTPUT_AVIF ? VIPS_FOREIGN_HEIF_COMPRESSION_AV1
                              : VIPS_FOREIGN_HEIF_COMPRESSION_HEVC;
    result = quality > 0
                 ? vips_heifsave(image, destination, "compression", compression,
                                 "Q", quality, "strip", strip_metadata, NULL)
                 : vips_heifsave(image, destination, "compression", compression,
                                 "strip", strip_metadata, NULL);
  }
  if (result == 0 && save_path && g_rename(save_path, output) != 0) {
    vips_error("mini-vips", "unable to replace output file");
    result = -1;
  }
  if (result != 0 && save_path) {
    g_unlink(save_path);
  }
  g_free(save_path);
  return result;
}

static int command_resize(const char *input, const char *output,
                          options_t *options) {
  gboolean has_width = has_option(options, "width");
  gboolean has_height = has_option(options, "height");
  gboolean has_dimensions = has_width || has_height;
  gboolean has_scale = has_option(options, "scale");
  gboolean has_max_pixels = has_option(options, "max-pixels");
  int selector_count = has_dimensions + has_scale + has_max_pixels;
  if ((has_width != has_height) || selector_count != 1) {
    report_error("specify exactly one of --width and --height, --scale, or "
                 "--max-pixels");
    return 2;
  }

  const char *fit = option(options, "fit");
  fit = fit ? fit : "contain";
  if (strcmp(fit, "contain") != 0 && strcmp(fit, "cover") != 0) {
    report_error("--fit must be contain or cover");
    return 2;
  }
  const char *position = option(options, "position");
  position = position ? position : "center";
  if (strcmp(position, "center") != 0 && strcmp(position, "top") != 0) {
    report_error("--position must be center or top");
    return 2;
  }
  if ((!has_dimensions && has_option(options, "fit")) ||
      (!has_dimensions && has_option(options, "position")) ||
      (strcmp(fit, "contain") == 0 && has_option(options, "position")) ||
      (!has_dimensions && has_option(options, "without-enlargement"))) {
    report_error("--fit, --position, and --without-enlargement require --width "
                 "and --height");
    return 2;
  }
  if (strcmp(fit, "cover") == 0 &&
      has_option(options, "without-enlargement")) {
    report_error("--without-enlargement cannot be used with --fit cover");
    return 2;
  }
  int width = integer_option(options, "width", 0);
  int height = integer_option(options, "height", 0);
  int max_pixels = integer_option(options, "max-pixels", 0);
  int quality = integer_option(options, "quality", -1);
  int colors = integer_option(options, "colors", -1);
  double scale = double_option(options, "scale", 0.0);
  if (has_dimensions &&
      (width < 1 || height < 1 || width > 65535 || height > 65535)) {
    report_error("--width and --height must be 1..65535");
    return 2;
  }
  if (has_scale && (scale <= 0.0 || scale > 100.0)) {
    report_error("--scale must be greater than 0 and at most 100");
    return 2;
  }
  if (has_max_pixels && max_pixels < 1) {
    report_error("--max-pixels must be greater than 0");
    return 2;
  }
  if (quality != -1 && (quality < 1 || quality > 100)) {
    report_error("--quality must be 1..100");
    return 2;
  }
  if (colors != -1 && (colors < 2 || colors > 256)) {
    report_error("--colors must be 2..256");
    return 2;
  }
  output_format_t format = output_format(output);
  if (format == OUTPUT_UNSUPPORTED) {
    report_error("unsupported output format");
    return 2;
  }
  if (format == OUTPUT_JXL) {
    vips_operation_block_set("VipsForeignSaveJxl", FALSE);
  }
  gboolean palette_output = format == OUTPUT_PNG || format == OUTPUT_GIF;
  if (colors > 0 && !palette_output) {
    report_error("--colors is supported only for PNG and GIF output");
    return 2;
  }
  if (quality > 0 && format == OUTPUT_GIF) {
    report_error("--quality is not supported for GIF output");
    return 2;
  }
  allow_resize_loaders();
  if (!valid_input_path(input)) {
    report_error("input filename options are not supported");
    return 2;
  }
  const char *loader = vips_foreign_find_load(input);
  if (!loader) {
    report_error("unsupported input format");
    return 1;
  }

  VipsImage *oriented = NULL;
  if (load_oriented(input, &oriented) != 0) {
    report_vips_error();
    return 1;
  }
  int source_width = vips_image_get_width(oriented);
  int source_height = vips_image_get_height(oriented);

  int target_width = width;
  int target_height = height;
  if (has_scale) {
    target_width = MAX(1, (int)round(source_width * scale));
    target_height = MAX(1, (int)round(source_height * scale));
  } else if (has_max_pixels) {
    double area = (double)source_width * source_height;
    double factor = area > max_pixels ? sqrt(max_pixels / area) : 1.0;
    target_width = MAX(1, (int)floor(source_width * factor));
    target_height = MAX(1, (int)floor(source_height * factor));
    if ((double)target_width * target_height > max_pixels) {
      if (target_width >= target_height) {
        target_width = MAX(1, max_pixels / target_height);
      } else {
        target_height = MAX(1, max_pixels / target_width);
      }
    }
  }
  if (target_width > 65535 || target_height > 65535) {
    report_error("resulting dimensions exceed 65535 pixels");
    VIPS_UNREF(oriented);
    return 2;
  }

  VipsImage *thumbnail = NULL;
  VipsImage *positioned = NULL;
  VipsImage *sharpened = NULL;
  int result = 0;
  if (has_dimensions && strcmp(fit, "cover") == 0) {
    double factor = fmax((double)width / source_width,
                         (double)height / source_height);
    result = vips_resize(oriented, &thumbnail, factor, NULL);
    if (result == 0) {
      VipsCompassDirection direction =
          strcmp(position, "top") == 0 ? VIPS_COMPASS_DIRECTION_NORTH
                                        : VIPS_COMPASS_DIRECTION_CENTRE;
      result = vips_gravity(thumbnail, &positioned, direction, width, height,
                            NULL);
    }
    if (result == 0) {
      result = vips_sharpen(positioned, &sharpened, "sigma", 0.5, "m1", 0.7,
                            NULL);
    }
  } else {
    VipsSize size = has_option(options, "without-enlargement") || has_max_pixels
                        ? VIPS_SIZE_DOWN
                        : VIPS_SIZE_BOTH;
    result = has_scale
                 ? vips_resize(oriented, &thumbnail, scale, NULL)
                 : vips_thumbnail(input, &thumbnail, target_width, "height",
                                  target_height, "size", size, "no_rotate",
                                  FALSE, "fail_on", VIPS_FAIL_ON_NONE, NULL);
  }

  if (result == 0) {
    result = save_image(sharpened ? sharpened : thumbnail, input, output,
                        format, quality, colors,
                        has_option(options, "strip-metadata"));
  }
  if (result != 0) {
    report_vips_error();
  }
  VIPS_UNREF(oriented);
  VIPS_UNREF(thumbnail);
  VIPS_UNREF(positioned);
  VIPS_UNREF(sharpened);
  return result == 0 ? 0 : 1;
}

static int command_dominant_color(const char *input) {
  VipsImage *thumbnail = NULL;
  double *components = NULL;
  int result = 1;

  allow_dominant_color_loaders();
  if (!valid_input_path(input)) {
    report_error("input filename options are not supported");
    return 2;
  }
  const char *loader = vips_foreign_find_load(input);
  if (!loader) {
    report_error("unsupported input format");
    goto cleanup;
  }

  if (vips_thumbnail(input, &thumbnail, 1, "height", 1, "size",
                     VIPS_SIZE_FORCE, "fail_on", VIPS_FAIL_ON_NONE, NULL) !=
      0) {
    report_vips_error();
    goto cleanup;
  }
  if (vips_image_get_width(thumbnail) != 1 ||
      vips_image_get_height(thumbnail) != 1) {
    report_error("dominant color did not produce one pixel");
    goto cleanup;
  }

  VipsBandFormat format = vips_image_get_format(thumbnail);
  int bands = vips_image_get_bands(thumbnail);
  if ((format != VIPS_FORMAT_UCHAR && format != VIPS_FORMAT_USHORT) ||
      bands < 1 || bands > 4) {
    report_error("unsupported pixel format");
    goto cleanup;
  }
  double sample_max = format == VIPS_FORMAT_USHORT ? 65535.0 : 255.0;

  int component_count = 0;
  if (vips_getpoint(thumbnail, &components, &component_count, 0, 0, NULL) != 0 ||
      !components || component_count != bands) {
    report_error("unable to read dominant color pixel");
    goto cleanup;
  }

  double alpha = vips_image_hasalpha(thumbnail)
                     ? components[bands - 1] / sample_max
                     : 1.0;
  unsigned char red = quantize_sample(components[0] * alpha, sample_max);
  unsigned char green =
      quantize_sample((bands < 3 ? components[0] : components[1]) * alpha,
                      sample_max);
  unsigned char blue =
      quantize_sample((bands < 3 ? components[0] : components[2]) * alpha,
                      sample_max);
  char color[7];
  snprintf(color, sizeof(color), "%02X%02X%02X", red, green, blue);
  puts(color);
  result = 0;

cleanup:
  g_free(components);
  VIPS_UNREF(thumbnail);
  return result;
}

static int command_convert(const char *input, const char *output) {
  allow_loader_family("VipsForeignLoadSvg");

  if (!valid_input_path(input)) {
    report_error("input filename options are not supported");
    return 2;
  }
  const char *loader = vips_foreign_find_load(input);
  const char *extension = strrchr(output, '.');
  if (!loader || !g_str_has_prefix(loader, "VipsForeignLoadSvg") ||
      !extension || g_ascii_strcasecmp(extension, ".png") != 0) {
    report_error("convert currently supports only SVG input and PNG output");
    return 2;
  }

  VipsImage *svg = NULL;
  VipsImage *flattened = NULL;
  int result = vips_svgload(input, &svg, NULL);
  if (result == 0) {
    result = vips_flatten(svg, &flattened, NULL);
  }
  if (result == 0) {
    result = vips_pngsave(flattened, output, "compression", 9, NULL);
  }
  if (result != 0) {
    report_vips_error();
  }
  VIPS_UNREF(svg);
  VIPS_UNREF(flattened);
  return result == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s COMMAND [OPTIONS]\n", argv[0]);
    return 2;
  }

  const char *command = argv[1];
  if (initialize_vips(argv[0]) != 0) {
    report_vips_error();
    return 1;
  }

  options_t options;
  int result = 2;
  if (strcmp(command, "version") == 0) {
    parse_options(argc, argv, 2, &options);
    validate_options(&options, NULL, 0);
    result = command_version();
  } else if (strcmp(command, "letter-avatar") == 0) {
    const char *letter = required_argument(argc, argv, 2, "letter");
    const char *output = required_argument(argc, argv, 3, "out");
    parse_options(argc, argv, 4, &options);
    const char *allowed[] = {"background-color", "size", "letter-size"};
    validate_options(&options, allowed, 3);
    result = command_letter_avatar(letter, output, argv[0], &options);
  } else if (strcmp(command, "resize") == 0) {
    const char *input = required_argument(argc, argv, 2, "in");
    const char *output = required_argument(argc, argv, 3, "out");
    parse_options(argc, argv, 4, &options);
    const char *allowed[] = {
        "width",       "height",     "scale",    "max-pixels",
        "fit",         "position",   "without-enlargement",
        "quality",     "colors",     "strip-metadata",
    };
    validate_options(&options, allowed, 10);
    result = command_resize(input, output, &options);
  } else if (strcmp(command, "dominant-color") == 0) {
    const char *input = required_argument(argc, argv, 2, "in");
    parse_options(argc, argv, 3, &options);
    validate_options(&options, NULL, 0);
    result = command_dominant_color(input);
  } else if (strcmp(command, "convert") == 0) {
    const char *input = required_argument(argc, argv, 2, "in");
    const char *output = required_argument(argc, argv, 3, "out");
    parse_options(argc, argv, 4, &options);
    validate_options(&options, NULL, 0);
    result = command_convert(input, output);
  } else {
    report_error("unsupported helper command");
  }

  vips_shutdown();
  return result;
}
