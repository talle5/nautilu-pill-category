#pragma once

#include <gtk/gtk.h>
#include "nautilus-sidebar-row.h"

G_BEGIN_DECLS

#define NAUTILUS_TYPE_SIDEBAR_FILTER_ROW (nautilus_sidebar_filter_row_get_type())

G_DECLARE_FINAL_TYPE (NautilusSidebarFilterRow,
                      nautilus_sidebar_filter_row,
                      NAUTILUS,
                      SIDEBAR_FILTER_ROW,
                      GtkListBoxRow)

G_END_DECLS
