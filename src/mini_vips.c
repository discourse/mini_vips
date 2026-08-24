#include <glib.h>
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

static const char *required_option(options_t *options, const char *key) {
  const char *value = option(options, key);
  if (!value || value[0] == '\0') {
    fprintf(stderr, "missing required option --%s\n", key);
    exit(2);
  }
  return value;
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

static void color_option(options_t *options, const char *key, int channels[3]) {
  const char *value = required_option(options, key);
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
    if (index + 1 >= argc) {
      fprintf(stderr, "missing value for %s\n", argv[index]);
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
    options->items[options->count].value = argv[++index];
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
      g_build_filename(program_directory, "NotoSans-Regular.ttf", NULL);
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

static int command_resize_png(const char *input, const char *output,
                              options_t *options) {
  int size = integer_option(options, "size", 0);
  if (size < 1 || size > 4096) {
    report_error("--size must be 1..4096");
    return 2;
  }

  allow_loader_family("VipsForeignLoadPng");
  VipsImage *thumbnail = NULL;
  VipsImage *sharpened = NULL;
  int result = vips_thumbnail(input, &thumbnail, size, "height", size, "size",
                              VIPS_SIZE_BOTH, "crop", VIPS_INTERESTING_CENTRE,
                              NULL);
  if (result == 0) {
    result =
        vips_sharpen(thumbnail, &sharpened, "sigma", 0.5, "m1", 0.7, NULL);
  }
  if (result == 0) {
    result = vips_pngsave(sharpened, output, "palette", TRUE, "Q", 100,
                          "compression", 6, "strip", TRUE, NULL);
  }
  if (result != 0) {
    report_vips_error();
  }
  VIPS_UNREF(thumbnail);
  VIPS_UNREF(sharpened);
  return result == 0 ? 0 : 1;
}

static int command_dominant_color(const char *input) {
  VipsImage *source = NULL;
  VipsImage *opaque = NULL;
  VipsImage *thumbnail = NULL;
  double *components = NULL;
  int result = 1;

  allow_dominant_color_loaders();
  const char *loader = vips_foreign_find_load(input);
  if (!loader) {
    report_error("unsupported input format");
    goto cleanup;
  }

  source = vips_image_new_from_file(input, "access", VIPS_ACCESS_SEQUENTIAL,
                                    "fail_on", VIPS_FAIL_ON_NONE, NULL);
  if (!source) {
    report_vips_error();
    goto cleanup;
  }
  if (vips_image_hasalpha(source)) {
    if (vips_flatten(source, &opaque, NULL) != 0) {
      report_vips_error();
      goto cleanup;
    }
  } else {
    opaque = g_object_ref(source);
  }
  if (vips_thumbnail_image(opaque, &thumbnail, 1, "height", 1, "size",
                           VIPS_SIZE_FORCE, NULL) != 0) {
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

  unsigned char red = quantize_sample(components[0], sample_max);
  unsigned char green =
      quantize_sample(bands < 3 ? components[0] : components[1], sample_max);
  unsigned char blue =
      quantize_sample(bands < 3 ? components[0] : components[2], sample_max);
  char color[7];
  snprintf(color, sizeof(color), "%02X%02X%02X", red, green, blue);
  puts(color);
  result = 0;

cleanup:
  g_free(components);
  VIPS_UNREF(source);
  VIPS_UNREF(opaque);
  VIPS_UNREF(thumbnail);
  return result;
}

static int command_svg_to_png(const char *input, const char *output) {
  allow_loader_family("VipsForeignLoadSvg");

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
  } else if (strcmp(command, "resize-png") == 0) {
    const char *input = required_argument(argc, argv, 2, "in");
    const char *output = required_argument(argc, argv, 3, "out");
    parse_options(argc, argv, 4, &options);
    const char *allowed[] = {"size"};
    validate_options(&options, allowed, 1);
    result = command_resize_png(input, output, &options);
  } else if (strcmp(command, "dominant-color") == 0) {
    const char *input = required_argument(argc, argv, 2, "in");
    parse_options(argc, argv, 3, &options);
    validate_options(&options, NULL, 0);
    result = command_dominant_color(input);
  } else if (strcmp(command, "svg-to-png") == 0) {
    const char *input = required_argument(argc, argv, 2, "in");
    const char *output = required_argument(argc, argv, 3, "out");
    parse_options(argc, argv, 4, &options);
    validate_options(&options, NULL, 0);
    result = command_svg_to_png(input, output);
  } else {
    report_error("unsupported helper command");
  }

  vips_shutdown();
  return result;
}
