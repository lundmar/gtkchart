#include <gtk/gtk.h>
#include <adwaita.h>

#include "demo_box.h"

static void activate_cb(GApplication *app, gpointer user_data)
{
    (void) user_data;
    GtkWidget *window;

    window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "GtkChart Demo");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 800);

    GtkWidget *box = demo_box_new();
    gtk_window_set_child(GTK_WINDOW(window), box);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    g_autoptr(AdwApplication) app = NULL;

    app = adw_application_new("com.github.lundmar.gtkchart", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate_cb), NULL);

    return g_application_run(G_APPLICATION(app), argc, argv);
}
