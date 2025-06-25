#include "demo_box.h"
#include "gtkchart.h"

#define VIEWPORT_WIDTH 25

struct _DemoBox {
    GtkBox parent_instance;
};

G_DEFINE_TYPE(DemoBox, demo_box, GTK_TYPE_BOX)

static gboolean plot_line_chart_timeout(gpointer user_data)
{
    static double x = 0.0;

    GtkChart *chart = GTK_CHART(user_data);
    double y = 3.0 * sin(x);

    if (x > gtk_chart_get_x_max(chart)) {
        gtk_chart_set_x_min(chart, x - VIEWPORT_WIDTH);
        gtk_chart_set_x_max(chart, x);
    }

    gtk_chart_plot_point(chart, x, y);
    x += 0.1;

    return G_SOURCE_CONTINUE;
}

static gboolean plot_scatter_chart_timeout(gpointer user_data)
{
    static double x = 0.0;

    GtkChart *chart = GTK_CHART(user_data);
    double y = 3.0 * sin(x);

    if (x > gtk_chart_get_x_max(chart)) {
        gtk_chart_set_x_min(chart, x - VIEWPORT_WIDTH);
        gtk_chart_set_x_max(chart, x);
    }

    gtk_chart_plot_point(chart, x, y);
    x += 0.3;

    return G_SOURCE_CONTINUE;
}

static gboolean angular_gauge_chart_timeout(gpointer user_data)
{
    static double value = 0.0;

    GtkChart *chart = GTK_CHART(user_data);

    gtk_chart_set_value (chart, value);
    value += 0.1;

    if (value > 50.0)
    {
        value = 0.0;
    }

    return G_SOURCE_CONTINUE;
}

static gboolean linear_gauge_chart_timeout(gpointer user_data)
{

    GtkChart *chart = GTK_CHART(user_data);

    static double value = 70;

    gtk_chart_set_value (chart, value);
    value -= 0.2;

    if (value < 0.0) {
        value = 70.0;
    }

    return G_SOURCE_CONTINUE;
}

static gboolean number_chart_timeout(gpointer user_data)
{
    GtkChart *chart = GTK_CHART(user_data);

    gtk_chart_set_value (chart, g_random_double());

    return G_SOURCE_CONTINUE;
}

static void demo_box_dispose(GObject *object);
static void demo_box_finalize(GObject *object);

static void demo_box_init(DemoBox *self)
{
    gtk_orientable_set_orientation(GTK_ORIENTABLE(self), GTK_ORIENTATION_VERTICAL);

    GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    // --- Line Chart ---
    GtkChart *line_chart = GTK_CHART(gtk_chart_new());
    gtk_chart_set_type(line_chart, GTK_CHART_TYPE_LINE);
    gtk_chart_set_title(line_chart, "Line Chart");
    gtk_chart_set_x_min(line_chart, 0.0);
    gtk_chart_set_x_max(line_chart, VIEWPORT_WIDTH);
    gtk_chart_set_y_min(line_chart, -3.5);
    gtk_chart_set_y_max(line_chart, 3.5);
    gtk_widget_set_hexpand(GTK_WIDGET(line_chart), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(line_chart), TRUE);
    gtk_box_append(GTK_BOX(hbox1), GTK_WIDGET(line_chart));
    g_timeout_add(30, plot_line_chart_timeout, line_chart);

    // --- Scatter Chart ---
    GtkChart *scatter_chart = GTK_CHART(gtk_chart_new());
    gtk_chart_set_type(scatter_chart, GTK_CHART_TYPE_SCATTER);
    gtk_chart_set_title(scatter_chart, "Scatter Chart");
    gtk_chart_set_x_min(scatter_chart, 0.0);
    gtk_chart_set_x_max(scatter_chart, VIEWPORT_WIDTH);
    gtk_chart_set_y_min(scatter_chart, -3.5);
    gtk_chart_set_y_max(scatter_chart, 3.5);
    gtk_widget_set_hexpand(GTK_WIDGET(scatter_chart), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(scatter_chart), TRUE);
    gtk_box_append(GTK_BOX(hbox1), GTK_WIDGET(scatter_chart));
    g_timeout_add(50, plot_scatter_chart_timeout, scatter_chart);

    // --- Gauge Angular Chart ---
    GtkChart *gauge_angular_chart = GTK_CHART(gtk_chart_new());
    gtk_chart_set_type(gauge_angular_chart, GTK_CHART_TYPE_GAUGE_ANGULAR);
    gtk_chart_set_title(gauge_angular_chart, "Gauge Angular Chart");
    gtk_widget_set_hexpand(GTK_WIDGET(gauge_angular_chart), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(gauge_angular_chart), TRUE);
    gtk_chart_set_value_min(gauge_angular_chart, 0.0);
    gtk_chart_set_value_max(gauge_angular_chart, 50.0);
    gtk_box_append(GTK_BOX(hbox2), GTK_WIDGET(gauge_angular_chart));
    g_timeout_add(50, angular_gauge_chart_timeout, gauge_angular_chart);
    // --- Linear Angular Chart ---
    GtkChart *gauge_linear_chart = GTK_CHART(gtk_chart_new());
    gtk_chart_set_type(gauge_linear_chart, GTK_CHART_TYPE_GAUGE_LINEAR);
    gtk_chart_set_title(gauge_linear_chart, "Gauge Angular Chart");
    gtk_widget_set_hexpand(GTK_WIDGET(gauge_linear_chart), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(gauge_linear_chart), TRUE);
    gtk_chart_set_value_min(gauge_linear_chart, 0.0);
    gtk_chart_set_value_max(gauge_linear_chart, 70.0);
    gtk_box_append(GTK_BOX(hbox2), GTK_WIDGET(gauge_linear_chart));
    g_timeout_add(30, linear_gauge_chart_timeout, gauge_linear_chart);

    // --- Number Chart ---
    GtkChart *number_chart = GTK_CHART(gtk_chart_new());
    gtk_chart_set_type(number_chart, GTK_CHART_TYPE_NUMBER);
    gtk_chart_set_title(number_chart, "Number Chart");
    gtk_widget_set_hexpand(GTK_WIDGET(number_chart), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(number_chart), TRUE);
    g_timeout_add(50, number_chart_timeout, number_chart);

    gtk_box_append(GTK_BOX(self), GTK_WIDGET(hbox1));
    gtk_box_append(GTK_BOX(self), GTK_WIDGET(hbox2));
    gtk_box_append(GTK_BOX(self), GTK_WIDGET(number_chart));
}

static void demo_box_class_init(DemoBoxClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = demo_box_dispose;
    object_class->finalize = demo_box_finalize;
}

static void demo_box_dispose(GObject *object)
{
    DemoBox *self = DEMO_BOX(object);

    (void) self;

    G_OBJECT_CLASS(demo_box_parent_class)->dispose(object);
}

static void demo_box_finalize(GObject *object)
{
    DemoBox *self = DEMO_BOX(object);

    (void) self;

    G_OBJECT_CLASS(demo_box_parent_class)->finalize(object);
}

GtkWidget *demo_box_new(void)
{
    return g_object_new(DEMO_TYPE_BOX, NULL);
}
