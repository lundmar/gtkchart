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

#include <ctype.h>
#include "gtkchart.h"
#include "glib.h"

#define UNUSED(expr) do { (void)(expr); } while (0)

struct chart_point_t
{
    double x;
    double y;
};

struct chart_slice_t
{
    double value;
    GdkRGBA color;
    gchar *label;
};

struct chart_column_t
{
  double value;
  GdkRGBA color;
  gchar *label;
};

struct _GtkChart
{
    GtkWidget parent_instance;
    GtkChartType type;
    char *title;
    char *label;
    char *x_label;
    char *y_label;
    double x_max;
    double y_max;
    double x_min;
    double y_min;
    double value;
    double value_min;
    double value_max;
    int width;
    void *user_data;
    GSList *point_list;
    GSList *slice_list;
    GSList *column_list;
    GtkSnapshot *snapshot;
    GdkRGBA text_color;
    GdkRGBA line_color;
    GdkRGBA grid_color;
    GdkRGBA axis_color;
    gchar *font_name;
    int ticks;
};

struct _GtkChartClass
{
    GtkWidgetClass parent_class;
};

G_DEFINE_TYPE (GtkChart, gtk_chart, GTK_TYPE_WIDGET)

static void gtk_chart_init(GtkChart *self)
{
    // Defaults
    self->type = GTK_CHART_TYPE_UNKNOWN;
    self->title = NULL;
    self->label = NULL;
    self->x_label = NULL;
    self->y_label = NULL;
    self->x_max = 100;
    self->y_max = 100;
    self->value_min = 0;
    self->value_max = 100;
    self->width = 500;
    self->snapshot = NULL;
    self->text_color.alpha = -1.0;
    self->line_color.alpha = -1.0;
    self->grid_color.alpha = -1.0;
    self->axis_color.alpha = -1.0;
    self->font_name = NULL;
    self->ticks = 4;

    // Automatically use GTK font
    GtkSettings *widget_settings = gtk_widget_get_settings(&self->parent_instance);
    GValue font_name_value = G_VALUE_INIT;
    g_object_get_property(G_OBJECT (widget_settings), "gtk-font-name", &font_name_value);
    gchar *font_string = g_strdup_value_contents(&font_name_value);

    // Extract name of font from font string ("<name> <size>")
    gchar *font_name = &font_string[1]; // Skip "
    for (unsigned int i=0; i<strlen(font_name); i++)
    {
        if (isdigit((int)font_name[i]))
        {
            font_name[i-1] = 0;
            break;
        }
    }
    self->font_name = g_strdup(font_name);
    g_free(font_string);

    //gtk_widget_init_template (GTK_WIDGET (self));
}

static void gtk_chart_finalize (GObject *object)
{
    GtkChart *self = GTK_CHART (object);

    G_OBJECT_CLASS (gtk_chart_parent_class)->finalize (G_OBJECT (self));
}

static void gtk_chart_dispose (GObject *object)
{
    GtkChart *self = GTK_CHART (object);
    GtkWidget *child;

    while ((child = gtk_widget_get_first_child (GTK_WIDGET (object))))
    {
        gtk_widget_unparent (child);
    }

    // Cleanup
    g_free(self->title);
    g_free(self->label);
    g_free(self->x_label);
    g_free(self->y_label);

    g_clear_slist(&self->point_list, g_free);

    g_clear_slist(&self->slice_list, g_free);

    gdk_display_sync(gdk_display_get_default());

    G_OBJECT_CLASS (gtk_chart_parent_class)->dispose (object);
}

static void chart_draw_line_or_scatter(GtkChart *self,
                                       GtkSnapshot *snapshot,
                                       float h,
                                       float w)
{
    cairo_text_extents_t extents;
    char value[20];

    // Assume aspect ratio w:h = 2:1

    // Set up Cairo region
    cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT(0, 0, w, h));
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);
    cairo_set_tolerance (cr, 1.5);
    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_select_font_face (cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Move coordinate system to bottom left
    cairo_translate(cr, 0, h);

    // Invert y-axis
    cairo_scale(cr, 1, -1);

    // Draw title
    double title_font_size = 0.05 * h;  // 5% of total height
    cairo_set_font_size (cr, title_font_size);
    cairo_text_extents(cr, self->title, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.9 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, self->title);
    cairo_restore(cr);

    // Draw x-axis label
    cairo_set_font_size (cr, 11.0 * (w/650));
    cairo_text_extents(cr, self->x_label, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.075 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, self->x_label);
    cairo_restore(cr);

    // Draw y-axis label
    cairo_text_extents(cr, self->y_label, &extents);
    cairo_move_to (cr, 0.035 * w, 0.5 * h - extents.width/2);
    cairo_save(cr);
    cairo_rotate(cr, M_PI/2);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, self->y_label);
    cairo_restore(cr);

    // Draw x-axis
    gdk_cairo_set_source_rgba (cr, &self->axis_color);
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.1 * w, 0.2 * h);
    cairo_line_to (cr, 0.9 * w, 0.2 * h);
    cairo_stroke (cr);

    // Draw y-axis
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.1 * w, 0.8 * h);
    cairo_line_to (cr, 0.1 * w, 0.2 * h);
    cairo_stroke (cr);

    // Draw x-axis value at 100% mark
    gdk_cairo_set_source_rgba (cr, &self->text_color);
    g_snprintf(value, sizeof(value), "%.1f", self->x_max);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.9 * w - extents.width/2, 0.16 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw x-axis value at 75% mark
    g_snprintf(value, sizeof(value), "%.1f", (self->x_max/4) * 3);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.7 * w - extents.width/2, 0.16 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw x-axis value at 50% mark
    g_snprintf(value, sizeof(value), "%.1f", self->x_max/2);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.16 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw x-axis value at 25% mark
    g_snprintf(value, sizeof(value), "%.1f", self->x_max/4);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.3 * w - extents.width/2, 0.16 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw x-axis value at 0% mark
    g_snprintf(value, sizeof(value), "%.1f", self->x_min);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, "0", &extents);
    cairo_move_to (cr, 0.1 * w - extents.width/2, 0.16 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw y-axis value at 0% mark
    g_snprintf(value, sizeof(value), "%.1f", self->y_min);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.091 * w - extents.width, 0.191 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw y-axis value at 25% mark
    g_snprintf(value, sizeof(value), "%.1f", self->y_min + (self->y_max - self->y_min) * 0.25);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.091 * w - extents.width, 0.34 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw y-axis value at 50% mark
    g_snprintf(value, sizeof(value), "%.1f", self->y_min + (self->y_max - self->y_min) * 0.5);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.091 * w - extents.width, 0.49 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw y-axis value at 75% mark
    g_snprintf(value, sizeof(value), "%.1f", self->y_min + (self->y_max - self->y_min) * 0.75);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.091 * w - extents.width, 0.64 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw y-axis value at 100% mark
    g_snprintf(value, sizeof(value), "%.1f", self->y_max);
    cairo_set_font_size (cr, 8.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to (cr, 0.091 * w - extents.width, 0.79 * h);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, value);
    cairo_restore(cr);

    // Draw grid x-line 25%
    gdk_cairo_set_source_rgba (cr, &self->grid_color);
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.1 * w, 0.35 * h);
    cairo_line_to (cr, 0.9 * w, 0.35 * h);
    cairo_stroke (cr);

    // Draw grid x-line 50%
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.1 * w, 0.5 * h);
    cairo_line_to (cr, 0.9 * w, 0.5 * h);
    cairo_stroke (cr);

    // Draw grid x-line 75%
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.1 * w, 0.65 * h);
    cairo_line_to (cr, 0.9 * w, 0.65 * h);
    cairo_stroke (cr);

    // Draw grid x-line 100%
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.1 * w, 0.8 * h);
    cairo_line_to (cr, 0.9 * w, 0.8 * h);
    cairo_stroke (cr);

    // Draw grid y-line 25%
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.3 * w, 0.8 * h);
    cairo_line_to (cr, 0.3 * w, 0.2 * h);
    cairo_stroke (cr);

    // Draw grid y-line 50%
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.5 * w, 0.8 * h);
    cairo_line_to (cr, 0.5 * w, 0.2 * h);
    cairo_stroke (cr);

    // Draw grid y-line 75%
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.7 * w, 0.8 * h);
    cairo_line_to (cr, 0.7 * w, 0.2 * h);
    cairo_stroke (cr);

    // Draw grid y-line 100%
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0.9 * w, 0.8 * h);
    cairo_line_to (cr, 0.9 * w, 0.2 * h);
    cairo_stroke (cr);

    // Move coordinate system to (0,0) of drawn coordinate system
    cairo_translate(cr, 0.1 * w, 0.2 * h);
    gdk_cairo_set_source_rgba (cr, &self->line_color);
    cairo_set_line_width (cr, 2.0);

    // Calc scales with min values taken into account
    float x_scale = (w - 2 * 0.1 * w) / (self->x_max - self->x_min);
    float y_scale = (h - 2 * 0.2 * h) / (self->y_max - self->y_min);

    // Draw data points from list
    GSList *l;
    gboolean last_point_visible = FALSE;
    double last_x = 0, last_y = 0;

    for (l = self->point_list; l != NULL; l = l->next)
    {
        struct chart_point_t *point = l->data;
        gboolean point_in_viewport = (point->x >= self->x_min &&
                                    point->x <= self->x_max &&
                                    point->y >= self->y_min &&
                                    point->y <= self->y_max);

        // Adjust coordinates by min values
        double x_coord = (point->x - self->x_min) * x_scale;
        double y_coord = (point->y - self->y_min) * y_scale;

        switch (self->type)
        {
            case GTK_CHART_TYPE_LINE:
                if (point_in_viewport)
                {
                    if (!last_point_visible)
                    {
                        // Start a new line segment when coming back into view
                        cairo_move_to(cr, x_coord, y_coord);
                    }
                    else
                    {
                        // Continue the line if previous point was visible
                        cairo_line_to(cr, x_coord, y_coord);
                        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
                        cairo_stroke(cr);
                        cairo_move_to(cr, x_coord, y_coord);
                    }
                    last_point_visible = TRUE;
                    last_x = x_coord;
                    last_y = y_coord;
                }
                else
                {
                    last_point_visible = FALSE;
                }
                break;

            case GTK_CHART_TYPE_SCATTER:
                if (point_in_viewport)
                {
                    // Draw point
                    cairo_set_line_width(cr, 3);
                    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
                    cairo_move_to(cr, x_coord, y_coord);
                    cairo_close_path(cr);
                    cairo_stroke(cr);
                }
                break;
        }
    }

    cairo_destroy (cr);
}

static void chart_draw_number(GtkChart *self,
                              GtkSnapshot *snapshot,
                              float h,
                              float w)
{
    cairo_text_extents_t extents;
    char value[20];

    // Assume aspect ratio w:h = 1:1

    // Set up Cairo region
    cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT(0, 0, w, h));
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);
    cairo_set_tolerance (cr, 1.5);
    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_select_font_face (cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Move coordinate system to bottom left
    cairo_translate(cr, 0, h);

    // Invert y-axis
    cairo_scale(cr, 1, -1);

    // Draw title
    double title_font_size = 0.05 * h;  // 5% of total height
    cairo_set_font_size (cr, title_font_size);
    cairo_text_extents(cr, self->title, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.9 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, self->title);
    cairo_restore(cr);

    // Draw label
    cairo_set_font_size (cr, 25.0 * (w/650));
    cairo_text_extents(cr, self->label, &extents);
    cairo_move_to(cr, 0.5 * w - extents.width/2, 0.2 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text(cr, self->label);
    cairo_restore(cr);

    // Draw number
    g_snprintf(value, sizeof(value), "%.1f", self->value);
    cairo_set_font_size (cr, 140.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to(cr, 0.5 * w - extents.width/2, 0.5 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text(cr, value);
    cairo_restore(cr);

    cairo_destroy (cr);
}

static void chart_draw_gauge_linear(GtkChart *self,
                                    GtkSnapshot *snapshot,
                                    float h,
                                    float w)
{
    cairo_text_extents_t extents;
    char value[20];

    // Assume aspect ratio w:h = 1:2

    // Set up Cairo region
    cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT(0, 0, w, h));
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);
    cairo_set_tolerance (cr, 1.5);
    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_select_font_face (cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Move coordinate system to bottom left
    cairo_translate(cr, 0, h);

    // Invert y-axis
    cairo_scale(cr, 1, -1);

    // Draw title
    double title_font_size = 0.05 * h;  // 5% of total height
    cairo_set_font_size (cr, title_font_size);
    cairo_text_extents(cr, self->title, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.95 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, self->title);
    cairo_restore(cr);

    // Draw label
    cairo_set_font_size (cr, 25.0 * (w/650));
    cairo_text_extents(cr, self->label, &extents);
    cairo_move_to(cr, 0.5 * w - extents.width/2, 0.05 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text(cr, self->label);
    cairo_restore(cr);

    // Draw minimum value
    g_snprintf(value, sizeof(value), "%.0f", self->value_min);
    cairo_set_font_size (cr, 25.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to(cr, 0.7 * w, 0.1 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text(cr, value);
    cairo_restore(cr);

    // Draw maximum value
    g_snprintf(value, sizeof(value), "%.0f", self->value_max);
    cairo_set_font_size (cr, 25.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to(cr, 0.7 * w, 0.9 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text(cr, value);
    cairo_restore(cr);

    // Draw minimum line
    gdk_cairo_set_source_rgba (cr, &self->axis_color);
    cairo_move_to(cr, 0.375 * w, 0.1 * h);
    cairo_line_to(cr, 0.625 * w, 0.1 * h);
    cairo_set_line_width (cr, 1);
    cairo_stroke (cr);

    // Draw maximum line
    cairo_move_to(cr, 0.375 * w, 0.9 * h);
    cairo_line_to(cr, 0.625 * w, 0.9 * h);
    cairo_set_line_width (cr, 1);
    cairo_stroke (cr);

    // Move coordinate system to (0,0) of gauge line start
    cairo_translate(cr, 0.5 * w, 0.1 * h);

    // Draw gauge line
    gdk_cairo_set_source_rgba (cr, &self->line_color);
    cairo_move_to(cr, 0, 0);
    float y_scale = (h - 2 * 0.1 * h) / self->value_max;
    cairo_set_line_width (cr, 0.2 * w);
    cairo_line_to(cr, 0, self->value * y_scale);
    cairo_stroke (cr);

    cairo_destroy (cr);
}

static void chart_draw_gauge_angular(GtkChart *self,
                                     GtkSnapshot *snapshot,
                                     float h,
                                     float w)
{
    cairo_text_extents_t extents;
    char value[20];

    // Assume aspect ratio w:h = 1:1

    // Set up Cairo region
    cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT(0, 0, w, h));
    cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);
    //  cairo_set_tolerance (cr, 1.5);
    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_select_font_face (cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Move coordinate system to bottom left
    cairo_translate(cr, 0, h);

    // Invert y-axis
    cairo_scale(cr, 1, -1);

    // Draw title
    double title_font_size = 0.05 * h;  // 5% of total height
    cairo_set_font_size (cr, title_font_size);
    cairo_text_extents(cr, self->title, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.9 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, self->title);
    cairo_restore(cr);

    // Draw label
    cairo_set_font_size (cr, 25.0 * (w/650));
    cairo_text_extents(cr, self->label, &extents);
    cairo_move_to(cr, 0.5 * w - extents.width/2, 0.1 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text(cr, self->label);
    cairo_restore(cr);

    // Draw minimum value
    g_snprintf(value, sizeof(value), "%.0f", self->value_min);
    cairo_set_font_size (cr, 25.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to(cr, 0.225 * w, 0.25 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text(cr, value);
    cairo_restore(cr);

    // Draw maximum value
    g_snprintf(value, sizeof(value), "%.0f", self->value_max);
    cairo_set_font_size (cr, 25.0 * (w/650));
    cairo_text_extents(cr, value, &extents);
    cairo_move_to(cr, 0.77 * w - extents.width, 0.25 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text(cr, value);
    cairo_restore(cr);

    // Draw minimum line
    gdk_cairo_set_source_rgba (cr, &self->axis_color);
    cairo_move_to(cr, 0.08 * w, 0.25 * h);
    cairo_line_to(cr, 0.22 * w, 0.25 * h);
    cairo_set_line_width (cr, 1);
    cairo_stroke (cr);

    // Draw maximum line
    cairo_move_to(cr, 0.78 * w, 0.25 * h);
    cairo_line_to(cr, 0.92 * w, 0.25 * h);
    cairo_set_line_width (cr, 1);
    cairo_stroke (cr);

    // Re-invert y-axis
    cairo_scale(cr, 1, -1);

    // Draw arc
    gdk_cairo_set_source_rgba (cr, &self->line_color);
    double xc = 0.5 * w;
    double yc = -0.25 * h;
    double radius = 0.35 * w;
    double angle1 = 180 * (M_PI/180.0);
    double angle = self->value * (180 / (self->value_max));
    double angle2 = 180 * (M_PI/180.0) + angle * (M_PI/180.0);
    cairo_set_line_width (cr, 0.1 * w);
    cairo_arc (cr, xc, yc, radius, angle1, angle2);
    cairo_stroke (cr);

    cairo_destroy (cr);
}

static void chart_draw_pie(GtkChart *self,
                                     GtkSnapshot *snapshot,
                                     float h,
                                     float w)
{
    cairo_text_extents_t extents;

    // Set up Cairo region
    cairo_t *cr = gtk_snapshot_append_cairo(snapshot, &GRAPHENE_RECT_INIT(0, 0, w, h));

    cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);

    // Center of chart
    double cx = w / 2.0;
    double cy = h / 2.0;

    double radius = MIN(w, h) / 2.5; // margin

    double total = 0.0;
    GSList *l;
    for(l = self->slice_list; l != NULL; l = l->next)
    {
        struct chart_slice_t *slice = l->data;
        total += slice->value;
    }

    if(total <= 0.0)
    {
        cairo_destroy(cr);
        return;
    }

    double start_angle = 0.0;

    for (l = self->slice_list; l != NULL; l = l->next)
    {
        struct chart_slice_t *slice = l->data;

        // Angle of the slice proportional to its value
        double slice_angle = (slice->value / total) * 2.0 * G_PI;

        cairo_set_source_rgba(cr, slice->color.red, slice->color.green, slice->color.blue, slice->color.alpha);

        cairo_move_to(cr, w / 2, h / 2);
        cairo_arc(cr, w / 2, h / 2, radius, start_angle, start_angle + slice_angle);
        cairo_close_path(cr);
        cairo_fill(cr);

        if(slice->label != NULL) {
            double middle = start_angle + slice_angle / 2.0;
            double distance = radius + 20; // Position outside of the slide radius

            double lx = cx + cos(middle) * distance; // X = cx + cos(0) * radius
            double ly = cy + sin(middle) * distance; // Y = cy + cos(0) * radius

            cairo_set_source_rgba(cr, slice->color.red, slice->color.green, slice->color.blue, slice->color.alpha);
            cairo_select_font_face(cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, 12);
            cairo_text_extents(cr, slice->label, &extents);

            // Adjust x position if angle is between 90º (PI/2) and 270º (3PI/2)
            if(middle > G_PI / 2 && middle < 3 * G_PI / 2)
            {
                lx -= extents.width;
            }

            cairo_move_to(cr, lx, ly);
            cairo_show_text(cr, slice->label);
        }

        start_angle += slice_angle;
    }

    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_select_font_face (cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Move coordinate system to bottom left
    cairo_translate(cr, 0, h);

    // Invert y-axis
    cairo_scale(cr, 1, -1);

    // Draw title
    cairo_set_font_size (cr, MIN(w, h) / 20);
    cairo_text_extents(cr, self->title, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.95 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, self->title);

    cairo_destroy(cr);
}

static void chart_draw_column(GtkChart *self,
                              GtkSnapshot *snapshot,
                              float h,
                              float w)
{
    cairo_text_extents_t extents;

    // Set up Cairo region
    cairo_t *cr = gtk_snapshot_append_cairo(snapshot, &GRAPHENE_RECT_INIT(0, 0, w, h + 40));

    cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);

    int n_total = g_slist_length(self->column_list);
    if(n_total == 0) {
        cairo_destroy(cr);
        return;
    }

    float spacing = (w * 0.05) * (n_total + 1);
    float column_width = (w - spacing) / n_total;

    double max_value = 0.0;
    GSList *l;
    for(l = self->column_list; l != NULL; l = l->next)
    {
        struct chart_column_t *column = l->data;
        if (column->value > max_value) max_value = column->value;
    }

    if(max_value <= 0.0)
    {
        cairo_destroy(cr);
        return;
    }

    float y_scale = (h - (0.1 * h) - (0.1 * h)) / max_value;
    int i = 0;

    double ticks = max_value / self->ticks;

    for(int i = 0; i <= self->ticks; i++)
    {
        double value = i * ticks;
        float y_tick = h - 0.1 * h - value * y_scale;

        char label[64];
        snprintf(label, sizeof(label), "%.0f", value);

        // Draw ticks value
        cairo_set_source_rgba(cr, 0.6, 0.6, 0.6, 0.8);
        cairo_select_font_face(cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12);

        cairo_move_to(cr, 5, y_tick);
        cairo_show_text(cr, label);

        // Draw line horizontal
        cairo_set_source_rgba(cr, 0.6, 0.6, 0.6, 0.3);
        cairo_move_to(cr, 30, y_tick);
        cairo_line_to(cr, w, y_tick);
        cairo_stroke(cr);
    }

    for (l = self->column_list; l != NULL; l = l->next)
    {
        struct chart_column_t *column = l->data;


        float column_height = column->value * y_scale;
        if(column_height < 2.0f) { // value = 0
            column_height = 2.0f;
        }

        float x = (w * 0.05) + i * (column_width + (w * 0.05));
        float y = h - (0.1 * h) - column_height;
        i++;

        // Draw Column
        cairo_set_source_rgba(cr, column->color.red, column->color.green, column->color.blue, column->color.alpha);

        cairo_rectangle(cr, x, y, column_width, column_height);
        cairo_fill(cr);

        if(column->label != NULL) {
            // Draw Label
            cairo_set_source_rgba(cr, 0.6, 0.6, 0.6, 0.8);
            cairo_select_font_face(cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, 12);
            cairo_text_extents(cr, column->label, &extents);

            float label_x = x + column_width / 2;
            float label_y = h - (w * 0.03) + 20;

            cairo_save(cr);
            cairo_translate(cr, label_x, label_y);
            cairo_rotate(cr, -M_PI / 3); // rotate -60º (-PI/3)

            cairo_move_to(cr, -extents.width / 2, -extents.height);
            cairo_show_text(cr, column->label);
            cairo_restore(cr);
        }
    }

    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_select_font_face (cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Move coordinate system to bottom left
    cairo_translate(cr, 0, h);

    // Invert y-axis
    cairo_scale(cr, 1, -1);

    // Draw title
    cairo_set_font_size (cr, MIN(w, h) / 20);
    cairo_text_extents(cr, self->title, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.95 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, self->title);

    cairo_destroy(cr);
}

static void chart_draw_unknown_type(GtkChart *self,
                                    GtkSnapshot *snapshot,
                                    float h,
                                    float w)
{
    UNUSED(self);

    cairo_text_extents_t extents;
    const char *warning = "Unknown chart type";

    // Set up Cairo region
    cairo_t * cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT(0, 0, w, h));
    gdk_cairo_set_source_rgba (cr, &self->text_color);
    cairo_select_font_face (cr, self->font_name, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    // Move coordinate system to bottom left
    cairo_translate(cr, 0, h);

    // Invert y-axis
    cairo_scale(cr, 1, -1);

    // Draw title
    double title_font_size = 0.05 * h;  // 5% of total height
    cairo_set_font_size (cr, title_font_size);
    cairo_text_extents(cr, warning, &extents);
    cairo_move_to (cr, 0.5 * w - extents.width/2, 0.5 * h - extents.height/2);
    cairo_save(cr);
    cairo_scale(cr, 1, -1);
    cairo_show_text (cr, warning);
    cairo_restore(cr);

    cairo_destroy (cr);
}


static void gtk_chart_snapshot (GtkWidget   *widget,
                                GtkSnapshot *snapshot)
{
    GtkChart *self = GTK_CHART(widget);

    float width = gtk_widget_get_width (widget);
    float height = gtk_widget_get_height (widget);

    // Automatically update colors if none set
    GtkStyleContext *context = gtk_widget_get_style_context(&self->parent_instance);
    if (self->text_color.alpha == -1.0)
    {
        gtk_style_context_get_color(context, &self->text_color);
    }
    if (self->line_color.alpha == -1.0)
    {
        gtk_style_context_lookup_color (context, "theme_selected_bg_color", &self->line_color);
    }
    if (self->grid_color.alpha == -1.0)
    {
        gtk_style_context_get_color(context, &self->grid_color);
        self->grid_color.alpha = 0.1;
    }
    if (self->axis_color.alpha == -1.0)
    {
        gtk_style_context_get_color(context, &self->axis_color);
    }

    // Draw various chart types
    switch (self->type)
    {
        case GTK_CHART_TYPE_LINE:
        case GTK_CHART_TYPE_SCATTER:
            chart_draw_line_or_scatter(self, snapshot, height, width);
            break;

        case GTK_CHART_TYPE_NUMBER:
            chart_draw_number(self, snapshot, height, width);
            break;

        case GTK_CHART_TYPE_GAUGE_LINEAR:
            chart_draw_gauge_linear(self, snapshot, height, width);
            break;

        case GTK_CHART_TYPE_GAUGE_ANGULAR:
            chart_draw_gauge_angular(self, snapshot, height, width);
            break;

        case GTK_CHART_TYPE_PIE:
            chart_draw_pie(self, snapshot, height, width);
            break;

        case GTK_CHART_TYPE_COLUMN:
            chart_draw_column(self, snapshot, height, width);
            break;

        default:
            chart_draw_unknown_type(self, snapshot, height, width);
            break;
    }

    self->snapshot = snapshot;
}

static void gtk_chart_class_init (GtkChartClass *class)
{
    GObjectClass *object_class = G_OBJECT_CLASS (class);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

    object_class->finalize = gtk_chart_finalize;
    object_class->dispose = gtk_chart_dispose;

    widget_class->snapshot = gtk_chart_snapshot;
}

EXPORT GtkWidget * gtk_chart_new (void)
{

    return g_object_new (GTK_TYPE_CHART, NULL);
}

EXPORT void gtk_chart_set_user_data(GtkChart *chart, void *user_data)
{
    chart->user_data = user_data;
}

EXPORT void * gtk_chart_get_user_data(GtkChart *chart)
{
    return chart->user_data;
}

EXPORT void gtk_chart_set_type(GtkChart *chart, GtkChartType type)
{
    chart->type = type;
}

EXPORT void gtk_chart_set_title(GtkChart *chart, const char *title)
{

    g_assert_nonnull(chart);
    g_assert_nonnull(title);

    if (chart->title != NULL)
    {
        g_free(chart->title);
    }

    chart->title = g_strdup(title);
}

EXPORT void gtk_chart_set_label(GtkChart *chart, const char *label)
{
    g_assert_nonnull(chart);
    g_assert_nonnull(label);

    if (chart->label != NULL)
    {
        g_free(chart->label);
    }

    chart->label = g_strdup(label);
}

EXPORT void gtk_chart_set_x_label(GtkChart *chart, const char *x_label)
{
    g_assert_nonnull(chart);
    g_assert_nonnull(x_label);

    if (chart->x_label != NULL)
    {
        g_free(chart->x_label);
    }

    chart->x_label = g_strdup(x_label);
}

EXPORT void gtk_chart_set_y_label(GtkChart *chart, const char *y_label)
{
    g_assert_nonnull(chart);
    g_assert_nonnull(y_label);

    if (chart->y_label != NULL)
    {
        g_free(chart->y_label);
    }

    chart->y_label = g_strdup(y_label);
}

EXPORT void gtk_chart_set_x_max(GtkChart *chart, double x_max)
{
    chart->x_max = x_max;
}

EXPORT void gtk_chart_set_y_max(GtkChart *chart, double y_max)
{
    chart->y_max = y_max;
}

EXPORT void gtk_chart_set_x_min(GtkChart *chart, double x_min)
{
    chart->x_min = x_min;
}

EXPORT void gtk_chart_set_y_min(GtkChart *chart, double y_min)
{
    chart->y_min = y_min;
}

EXPORT void gtk_chart_set_width(GtkChart *chart, int width)
{
    chart->width = width;
}

EXPORT void gtk_chart_plot_point(GtkChart *chart, double x, double y)
{
    // Allocate memory for new point
    struct chart_point_t *point = g_new0(struct chart_point_t, 1);
    point->x = x;
    point->y = y;

    // Add point to list to be drawn
    chart->point_list = g_slist_append(chart->point_list, point);

    // Queue draw of widget
    if (GTK_IS_WIDGET(chart))
    {
        gtk_widget_queue_draw(GTK_WIDGET(chart));
    }
}

EXPORT void gtk_chart_add_slice(GtkChart *chart, double value, const char *color, const char *label)
{
    // Allocate memory for new slice
    struct chart_slice_t *slice = g_new0(struct chart_slice_t, 1);
    slice->value = value;
    gdk_rgba_parse(&slice->color, color);
    if(label != NULL) slice->label = g_strdup(label);

    // Add slice to list to be drawn
    chart->slice_list = g_slist_append(chart->slice_list, slice);

    // Queue draw of widget
    if (GTK_IS_WIDGET(chart))
    {
        gtk_widget_queue_draw(GTK_WIDGET(chart));
    }
}

EXPORT void gtk_chart_add_column(GtkChart *chart, double value, const char *color, const char *label)
{
    // Allocate memory for new column
    struct chart_column_t *column = g_new0(struct chart_column_t, 1);
    column->value = value;
    gdk_rgba_parse(&column->color, color);
    if(label != NULL)  column->label = g_strdup(label);

    // Add column to list to be drawn
    chart->column_list = g_slist_append(chart->column_list, column);

    // Queue draw of widget
    if (GTK_IS_WIDGET(chart))
    {
        gtk_widget_queue_draw(GTK_WIDGET(chart));
    }
}

EXPORT void gtk_chart_set_value(GtkChart *chart, double value)
{
    chart->value = value;

    // Queue draw of widget
    if (GTK_IS_WIDGET(chart))
    {
        gtk_widget_queue_draw(GTK_WIDGET(chart));
    }
}

EXPORT void gtk_chart_set_value_min(GtkChart *chart, double value)
{
    chart->value_min = value;
}

EXPORT void gtk_chart_set_value_max(GtkChart *chart, double value)
{
    chart->value_max = value;
}

EXPORT double gtk_chart_get_value_min(GtkChart *chart)
{
    return chart->value_min;
}

EXPORT double gtk_chart_get_value_max(GtkChart *chart)
{
    return chart->value_max;
}

EXPORT double gtk_chart_get_x_max(GtkChart *chart)
{
    return chart->x_max;
}

EXPORT double gtk_chart_get_y_min(GtkChart *chart)
{
    return chart->y_min;
}

EXPORT bool gtk_chart_save_csv(GtkChart *chart, const char *filename, GError **error)
{
    struct chart_point_t *point;
    GSList *l;
    g_autoptr (GString) csv;

    csv = g_string_new(NULL);

    for (l = chart->point_list; l != NULL; l = l->next)
    {
        point = l->data;
        g_string_append_printf(csv, "%f,%f\n", point->x, point->y);
    }

    return g_file_set_contents(filename, csv->str, csv->len, error);
}

EXPORT GSList * gtk_chart_get_points(GtkChart *chart)
{
  return chart->point_list;
}

EXPORT bool gtk_chart_save_png(GtkChart *chart, const char *filename, GError **error)
{
    int width = gtk_widget_get_width (GTK_WIDGET(chart));
    int height = gtk_widget_get_height (GTK_WIDGET(chart));

    // Get to the PNG image file from paintable
    GdkPaintable *paintable = gtk_widget_paintable_new (GTK_WIDGET(chart));
    GtkSnapshot *snapshot = gtk_snapshot_new ();
    gdk_paintable_snapshot (paintable, snapshot, width, height);
    GskRenderNode *node = gtk_snapshot_free_to_node (snapshot);
    GskRenderer *renderer = gsk_cairo_renderer_new ();
    if (!gsk_renderer_realize (renderer, NULL, error))
      return false;
    GdkTexture *texture = gsk_renderer_render_texture (renderer, node, NULL);
    gdk_texture_save_to_png (texture, filename);

    // Cleanup
    g_object_unref(texture);
    gsk_renderer_unrealize(renderer);
    g_object_unref(renderer);
    gsk_render_node_unref(node);
    g_object_unref(paintable);

    return true;
}

EXPORT bool gtk_chart_set_color(GtkChart *chart, char *name, char *color)
{
    g_assert_nonnull(chart);
    g_assert_nonnull(name);

    if (strcmp(name, "text_color") == 0)
    {
        return gdk_rgba_parse(&chart->text_color, color);
    }
    else if (strcmp(name, "line_color") == 0)
    {
        return gdk_rgba_parse(&chart->line_color, color);
    }
    else if (strcmp(name, "grid_color") == 0)
    {
        return gdk_rgba_parse(&chart->grid_color, color);
    }
    else if (strcmp(name, "axis_color") == 0)
    {
        return gdk_rgba_parse(&chart->axis_color, color);
    }

    return false;
}

EXPORT void gtk_chart_set_font(GtkChart *chart, const char *name)
{
    g_assert_nonnull(chart);
    g_assert_nonnull(name);

    if (chart->font_name != NULL)
    {
        g_free(chart->font_name);
    }

    chart->font_name = g_strdup(name);
}

EXPORT void gtk_chart_set_slice_value(GtkChart *chart, int index, double value)
{
  g_assert_nonnull(chart);
  if(index < 0) return;

  GSList *l = chart->slice_list;
  int i = 0;

  while(l != NULL && i < index)
  {
    l = l->next;
    i++;
  }

  if(l == NULL) return;

  struct chart_slice_t *slice = l->data;
  slice->value = value;
}

EXPORT bool gtk_chart_set_slice_color(GtkChart *chart, int index, const char *color)
{
  g_assert_nonnull(chart);
  g_assert_nonnull(color);
  if(index < 0) return false;

  GSList *l = chart->slice_list;
  int i = 0;

  while(l != NULL && i < index)
  {
    l = l->next;
    i++;
  }

  if(l == NULL) return false;

  struct chart_slice_t *slice = l->data;
  return gdk_rgba_parse(&slice->color, color);
}

EXPORT void gtk_chart_set_slice_label(GtkChart *chart, int index, const char *label)
{
  g_assert_nonnull(chart);
  g_assert_nonnull(label);
  if(index < 0) return;

  GSList *l = chart->slice_list;
  int i = 0;

  while(l != NULL && i < index)
  {
    l = l->next;
    i++;
  }

  if(l == NULL) return;

  struct chart_slice_t *slice = l->data;

  g_free(slice->label);
  slice->label = g_strdup(label);
}

EXPORT void gtk_chart_set_column_value(GtkChart *chart, int index, double value)
{
  g_assert_nonnull(chart);
  if(index < 0) return;

  GSList *l = chart->column_list;
  int i = 0;

  while(l != NULL && i < index)
  {
    l = l->next;
    i++;
  }

  if(l == NULL) return;

  struct chart_column_t *column = l->data;
  column->value = value;
}

EXPORT bool gtk_chart_set_column_color(GtkChart *chart, int index, const char *color)
{
  g_assert_nonnull(chart);
  g_assert_nonnull(color);
  if(index < 0) return false;

  GSList *l = chart->column_list;
  int i = 0;

  while(l != NULL && i < index)
  {
    l = l->next;
    i++;
  }

  if(l == NULL) return false;

  struct chart_column_t *column = l->data;
  return gdk_rgba_parse(&column->color, color);
}

EXPORT void gtk_chart_set_column_label(GtkChart *chart, int index, const char *label)
{
  g_assert_nonnull(chart);
  g_assert_nonnull(label);
  if(index < 0) return;

  GSList *l = chart->column_list;
  int i = 0;

  while(l != NULL && i < index)
  {
    l = l->next;
    i++;
  }

  if(l == NULL) return;

  struct chart_column_t *column = l->data;

  g_free(column->label);
  column->label = g_strdup(label);
}

EXPORT void gtk_chart_set_column_ticks(GtkChart *chart, int ticks)
{
  g_assert_nonnull(chart);

  chart->ticks = ticks;
}

EXPORT double gtk_chart_get_column_max_value(GtkChart *chart) {
  double max_value = 0.0;
  GSList *l;
  for(l = chart->column_list; l != NULL; l = l->next)
  {
    struct chart_column_t *column = l->data;
    if (column->value > max_value) {
      max_value = column->value;
    }
  }

  return max_value;
}
