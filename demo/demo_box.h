#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DEMO_TYPE_BOX (demo_box_get_type())
G_DECLARE_FINAL_TYPE(DemoBox, demo_box, DEMO, BOX, GtkBox)

GtkWidget *demo_box_new(void);

G_END_DECLS
