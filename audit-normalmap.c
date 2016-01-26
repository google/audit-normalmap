/**
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <getopt.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb/stb_image_write.h"

char *json_prebuf = "";
void json_begin() {
  printf("%s{ ", json_prebuf);
  json_prebuf = "";
}
void json_key(const char *str) {
  printf("%s\"%s\": ", json_prebuf, str);
  json_prebuf = "";
}
void json_string(const char *str) {
  printf("%s\"%s\"\n", json_prebuf, str);
  json_prebuf = ", ";
}
void json_keystring(const char *key, const char *str) {
  json_key(key);
  json_string(str);
}
void json_number(double value) {
  if (isfinite(value)) {
    printf("%s%.17g\n", json_prebuf, value);
  } else {
    printf("%snull\n", json_prebuf);
  }
  json_prebuf = ", ";
}
void json_keynumber(const char *key, double val) {
  json_key(key);
  json_number(val);
}
void json_true() {
  printf("%strue\n", json_prebuf);
  json_prebuf = ", ";
}
void json_keytrue(const char *key) {
  json_key(key);
  json_true();
}
void json_end() {
  printf("}\n");
  json_prebuf = ", ";
}

void decode_normals(float *image, int width, int height, float center,
                    float range, bool invert_y);
void audit_normals(float *image, const char *image_name, int width, int height,
                   bool has_height, double normal_e, double height_e,
                   bool gl_nearest, bool post_correction_output, float *output,
                   const char *output_name);

int main(int argc, char **argv) {
  bool show_usage = false;
  bool invert_y = false;
  bool gl_nearest = false;
  bool post_correction_output = false;
  float discount_roundoff_errors_factor = 1.0;
  const char *infile = NULL;
  const char *outfile = NULL;
  float center = 0.5f;
  float range = 0.5f;

  int opt;
  while ((opt = getopt(argc, argv, "78cno:r:y")) != -1) {
    switch (opt) {
      case '7':
        center = 127.0f / 255.0f;
        range = 127.0f / 255.0f;
        break;
      case '8':
        center = 128.0f / 255.0f;
        range = 127.0f / 255.0f;
        break;
      case 'c':
        post_correction_output = true;
        break;
      case 'n':
        gl_nearest = true;
        break;
      case 'o':
        outfile = optarg;
        break;
      case 'r':
        discount_roundoff_errors_factor = atof(optarg);
        break;
      case 'y':
        invert_y = true;
        break;
      default:
        show_usage = true;
        break;
    }
  }
  if (optind == argc - 1) {
    infile = argv[optind];
  } else {
    show_usage = true;
  }
  if (!outfile && post_correction_output) {
    show_usage = true;
  }
  if (show_usage) {
    fprintf(stderr, "Usage: %s [-y] [-7] [-8] [[-c] -o outfile] infile\n",
            *argv);
    return 1;
  }

  // Configure the loader to always give us raw floats.
  stbi_ldr_to_hdr_gamma(1.0f);
  stbi_ldr_to_hdr_scale(1.0f);
  stbi_set_unpremultiply_on_load(true);

  int width, height, comp;
  float *image = stbi_loadf(infile, &width, &height, &comp, 4);
  if (!image) {
    fprintf(stderr, "Could not load image from %s: %s.\n", infile,
            stbi_failure_reason());
    return 1;
  }

  decode_normals(image, width, height, center, range, invert_y);
  float *output = NULL;
  if (outfile) {
    output = malloc(sizeof(float) * width * height * 3);
  }

  audit_normals(image, infile, width, height, comp == 4,
                discount_roundoff_errors_factor / (255.0 * range),
                discount_roundoff_errors_factor / 255.0, gl_nearest,
                post_correction_output, output, outfile);
  if (outfile) {
    if (!stbi_write_hdr(outfile, width, height, 3, output)) {
      fprintf(stderr, "Could not write image to %s: %s.\n", outfile,
              stbi_failure_reason());
      return 1;
    }
  }

  return 0;
}

void decode_normals(float *image, int width, int height, float center,
                    float range, bool invert_y) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float *p = image + 4 * (width * y + x);
      float nx = (p[0] - center) / range;
      float ny = (p[1] - center) / range;
      float nz = (p[2] - center) / range;
      if (invert_y) {
        ny *= -1;
      }
      p[0] = nx;
      p[1] = ny;
      p[2] = nz;
    }
  }
}

double integrate(double na, double da, double nb, double db, bool gl_nearest) {
  if (gl_nearest || da == db) {
    return -(na / da + nb / db) * 0.5;
  }
  // This is actually the integral of
  // -(na + (nb - na) * x) / (da + (db - da) * x), x=0..1
  return -((log(fabs(db)) - log(fabs(da))) * (db * na - da * nb) +
           (db - da) * (nb - na)) /
         ((db - da) * (db - da));
}

double fabs_without_explained_error(double error, double explained_error) {
  double abs_error = fabs(error);
  if (abs_error < explained_error) {
    return 0.0;
  }
  return abs_error - explained_error;
}

double ratio_e(double n, double d, double e) {
  return (fabs(n) + e) / (fabs(d) - e) - (fabs(n) - e) / (fabs(d) + e);
}

void audit_normals(float *image, const char *image_name, int width, int height,
                   bool has_height, double normal_e, double height_e,
                   bool gl_nearest, bool post_correction_output, float *output,
                   const char *output_name) {
  // Regression internals.
  double height_x_sxx = 0.0, height_x_sxy = 0.0, height_x_syy = 0.0;
  double height_y_sxx = 0.0, height_y_sxy = 0.0, height_y_syy = 0.0;
  double escher_sxx = 0.0, escher_sxy = 0.0, escher_syy = 0.0;

  // Square error sums.
  double height_ss = 0.0;
  double height_x_ss = 0.0;
  double height_y_ss = 0.0;
  double escher_xy_ss = 0.0;
  double escher_ss = 0.0;
  double length2_ss = 0.0;

  // Outputs.
  double height_x_m, height_x_R_2;
  double height_y_m, height_y_R_2;
  double height_m, height_R_2;
  double escher_xy_mx, escher_xy_my, escher_xy_R_2;
  double escher_R_2;
  double length_var;

  // Gather stats.
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float *p00 = image + 4 * (width * y + x);
      float *p01 = image + 4 * (width * ((y + 1) % height) + x);
      float *p10 = image + 4 * (width * y + (x + 1) % width);
      float *p11 = image + 4 * (width * ((y + 1) % height) + (x + 1) % width);
      // Calculate the line integral of the gradient implied by the normals
      // across a rectangular path defined by the centers of a four pixel block.
      // This is equivalent to the surface integral of the curl of the gradient
      // field over the same surface. If everything were perfect, this should be
      // zero thanks to curl . grad = const 0.
      double top = integrate(p00[0], p00[2], p10[0], p10[2], gl_nearest);
      double right = integrate(p10[1], p10[2], p11[1], p11[2], gl_nearest);
      double bottom = integrate(p11[0], p11[2], p01[0], p01[2], gl_nearest);
      double left = integrate(p01[1], p01[2], p00[1], p00[2], gl_nearest);
      double escher_x = top - bottom;
      double escher_y = left - right;
      double top_height = p10[3] - p00[3];
      double left_height = p01[3] - p00[3];

      // No need to correlate bottom and right - they are some other pixel's
      // top and left.
      height_x_sxx += top * top;
      height_x_sxy += top * top_height;
      height_x_syy += top_height * top_height;
      height_y_sxx += left * left;
      height_y_sxy += left * left_height;
      height_y_syy += left_height * left_height;
      escher_sxx += escher_x * escher_x;
      escher_sxy += escher_x * escher_y;
      escher_syy += escher_y * escher_y;
    }
  }

  // Calculate linear regression.
  double height_sxx = height_x_sxx + height_y_sxx;
  double height_sxy = height_x_sxy + height_y_sxy;
  height_x_m = height_x_sxy / height_x_sxx;
  height_y_m = height_y_sxy / height_y_sxx;
  height_m = height_sxy / height_sxx;

  // Find values mx and my so that:
  // mx * my = +-1
  // ("area preserving scaling")
  // minimize sum((x*mx - y*my)^2)
  // minimize sxx * mx^2 - sxy * 2*mx*my + syy * my^2
  // set mx = sqrt(m), my = 1/sqrt(m)
  // minimize sxx * m - sxy * 2 + syy / m
  // 0 = sxx + syy * -1/m^2
  // m = sqrt(syy / sxx)
  escher_xy_mx = sqrt(sqrt(escher_syy / escher_sxx));
  escher_xy_my = sqrt(sqrt(escher_sxx / escher_syy));
  if (escher_sxy < 0) {
    // Ensure that the 2nd term is negative.
    escher_xy_mx = -escher_xy_mx;
  }

  // Calculate errors and write them to the output.
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float *p00 = image + 4 * (width * y + x);
      float *p01 = image + 4 * (width * ((y + 1) % height) + x);
      float *p10 = image + 4 * (width * y + (x + 1) % width);
      float *p11 = image + 4 * (width * ((y + 1) % height) + (x + 1) % width);
      double top = integrate(p00[0], p00[2], p10[0], p10[2], gl_nearest);
      double top_e =
          ratio_e(p00[0], p00[2], normal_e) + ratio_e(p10[0], p10[2], normal_e);
      double right = integrate(p10[1], p10[2], p11[1], p11[2], gl_nearest);
      double right_e =
          ratio_e(p10[1], p10[2], normal_e) + ratio_e(p11[1], p11[2], normal_e);
      double bottom = integrate(p11[0], p11[2], p01[0], p01[2], gl_nearest);
      double bottom_e =
          ratio_e(p11[0], p11[2], normal_e) + ratio_e(p01[0], p01[2], normal_e);
      double left = integrate(p01[1], p01[2], p00[1], p00[2], gl_nearest);
      double left_e =
          ratio_e(p01[1], p01[2], normal_e) + ratio_e(p00[1], p00[2], normal_e);
      double escher_x = top - bottom;
      double escher_x_e = top_e + bottom_e;
      double escher_y = left - right;
      double escher_y_e = left_e + right_e;
      double top_height = p10[3] - p00[3];
      double top_height_e = 2 * height_e;
      double left_height = p01[3] - p00[3];
      double left_height_e = 2 * height_e;

      double x_error = fabs_without_explained_error(
          top_height - top * height_m, top_height_e + top_e * fabs(height_m));
      height_ss += x_error * x_error;
      double y_error =
          fabs_without_explained_error(left_height - left * height_m,
                                       left_height_e + left_e * fabs(height_m));
      height_ss += y_error * y_error;
      double x_x_error =
          fabs_without_explained_error(top_height - top * height_x_m,
                                       top_height_e + top_e * fabs(height_x_m));
      height_x_ss += x_x_error * x_x_error;
      double y_y_error = fabs_without_explained_error(
          left_height - left * height_y_m,
          left_height_e + left_e * fabs(height_y_m));
      height_y_ss += y_y_error * y_y_error;
      double escher_error = fabs_without_explained_error(
          escher_x - escher_y, escher_x_e + escher_y_e);
      escher_ss += escher_error * escher_error;
      double escher_xy_error = fabs_without_explained_error(
          escher_x * escher_xy_mx - escher_y * escher_xy_my,
          escher_x_e * fabs(escher_xy_mx) + escher_y_e * fabs(escher_xy_my));
      escher_xy_ss += escher_xy_error * escher_xy_error;
      double length2_error = fabs_without_explained_error(
          p00[0] * p00[0] + p00[1] * p00[1] + p00[2] * p00[2] - 1,
          2 * (fabs(p00[0]) + fabs(p00[1]) + fabs(p00[2])) * normal_e);
      length2_ss += length2_error * length2_error;

      if (output) {
        float *out = output + 3 * (width * y + x);
        if (post_correction_output) {
          out[0] = x_x_error + y_y_error;
          out[1] = length2_error;
          out[2] = escher_xy_error;
        } else {
          out[0] = x_error + y_error;
          out[1] = length2_error;
          out[2] = escher_error;
        }
      }
    }
  }

  // Calculate determination coefficients.
  double height_syy = height_x_syy + height_y_syy;
  double escher_xy_syy = escher_syy * escher_xy_my * escher_xy_my;
  height_x_R_2 = 1 - height_x_ss / height_x_syy;
  height_y_R_2 = 1 - height_y_ss / height_y_syy;
  height_R_2 = 1 - height_ss / height_syy;
  escher_xy_R_2 = 1 - escher_xy_ss / escher_xy_syy;
  escher_R_2 = 1 - escher_ss / escher_syy;
  // Known issue: escher_xy_R_2 is not necessarily >= escher_R_2, because the
  // values aren't comparable after nonuniform scaling (essentially, minimizing
  // RMS does not maximize R_2 because the variance changes too).
  length_var = length2_ss / ((double)width * (double)height) * 0.5;

  // Write report.
  json_begin();
  json_keystring("image", image_name);
  if (has_height) {
    json_keynumber("heightmap_scale", height_m);
    json_keynumber("heightmap_R_2", height_R_2);
    json_keynumber("heightmap_x_scale", height_x_m);
    json_keynumber("heightmap_x_R_2", height_x_R_2);
    json_keynumber("heightmap_y_scale", height_y_m);
    json_keynumber("heightmap_y_R_2", height_y_R_2);
    json_keynumber("heightmap_normalmap_scale", height_x_m / height_y_m);
    if (height_x_m < 0) {
      json_keytrue("error_heightmap_normalmap_inverted");
    }
    if (height_x_m * height_y_m < 0) {
      json_keytrue("error_heightmap_normalmap_inverted_y");
    }
    if (fabs(height_x_m / height_y_m) < 0.8 ||
        fabs(height_x_m / height_y_m) > 1.25) {
      json_keytrue("error_heightmap_normalmap_nonuniform_scaling");
    }
    if (height_R_2 < 0.5) {
      json_keytrue("error_heightmap_inconsistent");
    }
  } else {
    json_keytrue("error_heightmap_missing");
  }

  json_keynumber("normalmap_R_2", escher_R_2);
  json_keynumber("normalmap_fix_scale", escher_xy_mx / escher_xy_my);
  json_keynumber("normalmap_fix_R_2", escher_xy_R_2);
  json_keynumber("normalmap_length_var", length_var);
  if (escher_xy_mx * escher_xy_my < 0) {
    json_keytrue("error_normalmap_inverted_y");
  }
  if (fabs(escher_xy_mx / escher_xy_my) < 0.8 ||
      fabs(escher_xy_mx / escher_xy_my) > 1.25) {
    json_keytrue("error_normalmap_nonuniform_scaling");
  }
  if (escher_R_2 < 0.5) {
    json_keytrue("error_normalmap_inconsistent");
  }
  if (length_var > 0.001) {
    json_keytrue("error_normalmap_denormalized");
  }

  if (output) {
    json_keystring("output_name", output_name);
    if (post_correction_output) {
      json_keystring("output_channel_r", "heightmap_scale_error");
      json_keystring("output_channel_g", "length_error");
      json_keystring("output_channel_b", "normalmap_fix_error");
    } else {
      json_keystring("output_channel_r", "heightmap_error");
      json_keystring("output_channel_g", "length_error");
      json_keystring("output_channel_b", "normalmap_error");
    }
  }

  json_end();
}
