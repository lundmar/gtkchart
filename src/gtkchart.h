/*
 * Copyright (c) 2022  Martin Lund
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <gtk/gtk.h>

#if defined _WIN32 || defined __CYGWIN__
  #define EXPORT __declspec(dllexport)
#else
  #if defined __GNUC__
    #define EXPORT __attribute__ ((visibility("default")))
  #else
    #pragma message ("Compiler does not support symbol visibility.")
    #define EXPORT
  #endif
#endif

typedef struct chart_point_t ChartPoint;

G_BEGIN_DECLS

#define GTK_TYPE_CHART (gtk_chart_get_type ())
G_DECLARE_FINAL_TYPE (GtkChart, gtk_chart, GTK, CHART, GtkWidget)

typedef enum
{
  GTK_CHART_TYPE_UNKNOWN,
  GTK_CHART_TYPE_LINE,
  GTK_CHART_TYPE_SCATTER,
  GTK_CHART_TYPE_GAUGE_ANGULAR,
  GTK_CHART_TYPE_GAUGE_LINEAR,
  GTK_CHART_TYPE_PIE,
  GTK_CHART_TYPE_NUMBER
} GtkChartType;

EXPORT GtkWidget * gtk_chart_new (void);
EXPORT void gtk_chart_set_type(GtkChart *chart, GtkChartType type);
EXPORT void gtk_chart_set_title(GtkChart *chart, const char *title);
EXPORT void gtk_chart_set_label(GtkChart *chart, const char *label);
EXPORT void gtk_chart_set_x_label(GtkChart *chart, const char *x_label);
EXPORT void gtk_chart_set_y_label(GtkChart *chart, const char *y_label);
EXPORT void gtk_chart_set_x_max(GtkChart *chart, double x_max);
EXPORT void gtk_chart_set_y_max(GtkChart *chart, double y_max);
EXPORT void gtk_chart_set_x_min(GtkChart *chart, double x_min);
EXPORT void gtk_chart_set_y_min(GtkChart *chart, double y_min);
EXPORT double gtk_chart_get_x_max(GtkChart *chart);
EXPORT double gtk_chart_get_y_min(GtkChart *chart);
EXPORT void gtk_chart_set_width(GtkChart *chart, int width);
EXPORT void gtk_chart_plot_point(GtkChart *chart, double x, double y);

EXPORT void gtk_chart_set_value(GtkChart *chart, double value);
EXPORT void gtk_chart_set_value_min(GtkChart *chart, double value);
EXPORT void gtk_chart_set_value_max(GtkChart *chart, double value);
EXPORT double gtk_chart_get_value_min(GtkChart *chart);
EXPORT double gtk_chart_get_value_max(GtkChart *chart);

EXPORT bool gtk_chart_save_csv(GtkChart *chart, const char *filename, GError **error);
EXPORT bool gtk_chart_save_png(GtkChart *chart, const char *filename, GError **error);
EXPORT GSList * gtk_chart_get_points(GtkChart *chart);

EXPORT void gtk_chart_set_user_data(GtkChart *chart, void *user_data);
EXPORT void * gtk_chart_get_user_data(GtkChart *chart);

EXPORT bool gtk_chart_set_color(GtkChart *chart, char *name, char *color);
EXPORT void gtk_chart_set_font(GtkChart *chart, const char *name);

EXPORT void gtk_chart_add_slice(GtkChart *chart, double value, const char *color, const char *label);
EXPORT void gtk_chart_set_slice_value(GtkChart *chart, int index, double value);
EXPORT bool gtk_chart_set_slice_color(GtkChart *chart, int index, const char *color);
EXPORT void gtk_chart_set_slice_label(GtkChart *chart, int index, const char *label);

G_END_DECLS
