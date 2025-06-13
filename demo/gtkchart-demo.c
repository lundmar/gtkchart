/*
 * GtkChart - demo application
 *
 */

#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gtkchart.h>
#include <adwaita.h>

#define VIEWPORT_WIDTH 25  // Show 5 units of x-axis at a time

static GtkChart *chart;
static GMutex producer_mutex;

struct point_t
{
    double x;
    double y;
};

static gboolean gui_chart_plot_thread(gpointer user_data)
{
    struct point_t *point = user_data;

    // Update x-axis viewport to follow the latest data
    if (point->x > gtk_chart_get_x_max(chart)) {
        gtk_chart_set_x_min(chart, point->x - VIEWPORT_WIDTH);
        gtk_chart_set_x_max(chart, point->x);
    }

    gtk_chart_plot_point(chart, point->x, point->y);

    return G_SOURCE_REMOVE;
}

static void activate_cb(GApplication *app, gpointer user_data)
{
    GtkWidget *window;
    (void)user_data;

    window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "gtkchart-demo");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

    chart = GTK_CHART(gtk_chart_new());
    gtk_chart_set_type(chart, GTK_CHART_TYPE_LINE);
    gtk_chart_set_title(chart, "Sine signal, f(x) = 3sin(x)");
    gtk_chart_set_label(chart, "Label");
    gtk_chart_set_x_label(chart, "X label [unit]");
    gtk_chart_set_y_label(chart, "Y label [unit]");

    // Set fixed y-axis range to show complete sine wave
    gtk_chart_set_x_min(chart, 0.0);
    gtk_chart_set_x_max(chart, VIEWPORT_WIDTH);
    gtk_chart_set_y_max(chart, 3.5);   // Above maximum of sine wave
    gtk_chart_set_y_min(chart, -3.5);  // Below minimum of sine wave
    gtk_chart_set_width(chart, 800);

    // gtk_chart_set_color(chart, "text_color", "red");
    // gtk_chart_set_color(chart, "line_color", "blue");
    // gtk_chart_set_color(chart, "grid_color", "green");
    // gtk_chart_set_color(chart, "axis_color", "yellow");
    // gtk_chart_set_font(chart, "Arial");

    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(chart));

    gtk_widget_show(window);

    g_mutex_unlock(&producer_mutex);
}

static gpointer producer_function(gpointer user_data)
{
    (void)user_data;
    struct point_t point;
    double x = 0;
    double y;

    g_mutex_lock(&producer_mutex);

    // Plot demo sine wave with negative values
    while (x < 100)
    {
        y = 3.0 * sin(x);  // Will oscillate between -3 and +3
        point.x = x;
        point.y = y;
        g_idle_add(gui_chart_plot_thread, &point);
        usleep(20*1000);
        x += 0.1;
    }

    gtk_chart_save_csv(chart, "chart0.csv");
    gtk_chart_save_png(chart, "chart0.png");

    g_mutex_unlock(&producer_mutex);

    return NULL;
}

int main(int argc, char **argv)
{
    int stat;
    g_autoptr (AdwApplication) app = NULL;

    // Spawn data generating thread
    g_mutex_lock(&producer_mutex);
    GThread *producer_thread = g_thread_new("producer thread", producer_function, NULL);

    // Start GTK/Adwaita application
    app = adw_application_new("com.github.lundmar.gtkchart", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK (activate_cb), NULL);
    stat = g_application_run(G_APPLICATION (app), argc, argv);

    // Clean up
    g_object_unref(app);
    g_thread_join(producer_thread);

    return stat;
}
