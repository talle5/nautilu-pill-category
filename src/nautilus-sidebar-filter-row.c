#include <gtk/gtk.h>
#include "nautilus-sidebar-filter-row.h"
#include "nautilus-file.h"
#include "nautilus-window.h"
#include "nautilus-window-slot.h"
#include "nautilus-query.h"
#include "nautilus-enums.h"
#include "nautilus-enum-types.h"
#include "nautilus-view-item.h"

struct _NautilusSidebarFilterRow
{
    GtkListBoxRow parent_instance;

    GtkWidget *images_button;
    GtkWidget *videos_button;
    GtkWidget *audio_button;
    GtkWidget *documents_button;
    GtkWidget *archives_button;
    GtkWidget *text_button;

    char *uri;
    char *label;
    char *mime_type;
    NautilusSidebarSectionType section_type;
    NautilusSidebarRowType place_type;
    gboolean updating_buttons;
};

G_DEFINE_TYPE (NautilusSidebarFilterRow,
               nautilus_sidebar_filter_row,
               GTK_TYPE_LIST_BOX_ROW)

enum { PROP_0, PROP_URI, PROP_LABEL, PROP_SECTION_TYPE, PROP_PLACE_TYPE, PROP_SIDEBAR, LAST_PROP };

static void
nautilus_sidebar_filter_row_set_property (GObject *obj, guint id, const GValue *v, GParamSpec *ps)
{
    NautilusSidebarFilterRow *self = NAUTILUS_SIDEBAR_FILTER_ROW (obj);
    if (id == PROP_URI) self->uri = g_value_dup_string (v);
    if (id == PROP_LABEL) self->label = g_value_dup_string (v);
}

static void
nautilus_sidebar_filter_row_get_property (GObject *obj, guint id, GValue *v, GParamSpec *ps)
{
    NautilusSidebarFilterRow *self = NAUTILUS_SIDEBAR_FILTER_ROW (obj);
    if (id == PROP_URI) g_value_set_string (v, self->uri);
    if (id == PROP_LABEL) g_value_set_string (v, self->label);
}

static gboolean
filter_by_mime_type_func (gpointer item, gpointer user_data)
{
    NautilusSidebarFilterRow *self = user_data;

    if (!NAUTILUS_IS_VIEW_ITEM (item))
    {
        return TRUE;
    }

    NautilusViewItem *view_item = NAUTILUS_VIEW_ITEM (item);
    NautilusFile *file = nautilus_view_item_get_file (view_item);

    if (self->mime_type == NULL) return TRUE;

    return nautilus_file_is_mime_type (file, self->mime_type);
}

static gboolean
is_any_filter_button_active (NautilusSidebarFilterRow *self)
{
    return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->images_button)) ||
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->videos_button)) ||
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->audio_button)) ||
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->documents_button)) ||
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->archives_button)) ||
           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->text_button));
}

static void
set_only_active_button (NautilusSidebarFilterRow *self,
                        GtkToggleButton          *active_button)
{
    self->updating_buttons = TRUE;

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->images_button),
                                  GTK_TOGGLE_BUTTON (self->images_button) == active_button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->videos_button),
                                  GTK_TOGGLE_BUTTON (self->videos_button) == active_button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->audio_button),
                                  GTK_TOGGLE_BUTTON (self->audio_button) == active_button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->documents_button),
                                  GTK_TOGGLE_BUTTON (self->documents_button) == active_button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->archives_button),
                                  GTK_TOGGLE_BUTTON (self->archives_button) == active_button);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->text_button),
                                  GTK_TOGGLE_BUTTON (self->text_button) == active_button);

    self->updating_buttons = FALSE;
}

static void
apply_filter_to_current_view (NautilusSidebarFilterRow *self)
{
    GtkRoot *root = gtk_widget_get_root (GTK_WIDGET (self));
    if (!NAUTILUS_IS_WINDOW (root)) return;

    NautilusWindow *window = NAUTILUS_WINDOW (root);
    NautilusWindowSlot *slot = NULL;
    g_object_get (window, "active-slot", &slot, NULL);
    if (slot == NULL) return;

    if (self->mime_type == NULL)
    {
        nautilus_window_slot_set_filter (slot, NULL);
        g_object_unref (slot);
        return;
    }

    // 1. Criamos um filtro customizado do GTK
    GtkCustomFilter *custom_filter = gtk_custom_filter_new (filter_by_mime_type_func, self, NULL);

    // 2. Aplicamos o filtro DIRETAMENTE no slot (isso não abre a pesquisa!)
    nautilus_window_slot_set_filter (slot, GTK_FILTER (custom_filter));

    // O slot assume a referência do filtro, podemos soltar a nossa
    g_object_unref (custom_filter);
    g_object_unref (slot);
}

static void
apply_mime_filter (NautilusSidebarFilterRow *self,
                   const char              *mime_type)
{
    g_free (self->mime_type);
    self->mime_type = mime_type ? g_strdup (mime_type) : NULL;

    apply_filter_to_current_view (self);
}

static void
on_button_toggled (GtkToggleButton *button, NautilusSidebarFilterRow *self)
{
    if (self->updating_buttons)
    {
        return;
    }

    if (!gtk_toggle_button_get_active (button))
    {
        if (!is_any_filter_button_active (self))
        {
            apply_mime_filter (self, NULL);
        }
        return;
    }

    set_only_active_button (self, button);

    const char *mime = NULL;
    if (GTK_WIDGET (button) == self->images_button) mime = "image/*";
    else if (GTK_WIDGET (button) == self->videos_button) mime = "video/*";
    else if (GTK_WIDGET (button) == self->audio_button) mime = "audio/*";
    else if (GTK_WIDGET (button) == self->documents_button) mime = "application/pdf";
    else if (GTK_WIDGET (button) == self->archives_button) mime = "application/zip";
    else if (GTK_WIDGET (button) == self->text_button) mime = "text/*";

    apply_mime_filter (self, mime);
}

static void
nautilus_sidebar_filter_row_init (NautilusSidebarFilterRow *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->uri = g_strdup ("");
    self->label = g_strdup ("Filtros");

    g_signal_connect (self->images_button, "toggled", G_CALLBACK (on_button_toggled), self);
    g_signal_connect (self->videos_button, "toggled", G_CALLBACK (on_button_toggled), self);
    g_signal_connect (self->audio_button, "toggled", G_CALLBACK (on_button_toggled), self);
    g_signal_connect (self->documents_button, "toggled", G_CALLBACK (on_button_toggled), self);
    g_signal_connect (self->archives_button, "toggled", G_CALLBACK (on_button_toggled), self);
    g_signal_connect (self->text_button, "toggled", G_CALLBACK (on_button_toggled), self);
}

static void
nautilus_sidebar_filter_row_class_init (NautilusSidebarFilterRowClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS (klass);
    GtkWidgetClass *wclass = GTK_WIDGET_CLASS (klass);

    oclass->set_property = nautilus_sidebar_filter_row_set_property;
    oclass->get_property = nautilus_sidebar_filter_row_get_property;

    g_object_class_install_property (oclass, PROP_URI, g_param_spec_string ("uri", "uri", "uri", "", G_PARAM_READWRITE));
    g_object_class_install_property (oclass, PROP_LABEL, g_param_spec_string ("label", "label", "label", "", G_PARAM_READWRITE));
    g_object_class_install_property (oclass, PROP_SECTION_TYPE, g_param_spec_enum ("section-type", "s", "s", NAUTILUS_TYPE_SIDEBAR_SECTION_TYPE, 0, G_PARAM_READWRITE));
    g_object_class_install_property (oclass, PROP_PLACE_TYPE, g_param_spec_enum ("place-type", "p", "p", NAUTILUS_TYPE_SIDEBAR_ROW_TYPE, 0, G_PARAM_READWRITE));
    g_object_class_install_property (oclass, PROP_SIDEBAR, g_param_spec_object ("sidebar", "s", "s", G_TYPE_OBJECT, G_PARAM_READWRITE));

    gtk_widget_class_set_template_from_resource (wclass, "/org/gnome/nautilus/ui/nautilus-sidebar-filter-row.ui");
    gtk_widget_class_bind_template_child (wclass, NautilusSidebarFilterRow, images_button);
    gtk_widget_class_bind_template_child (wclass, NautilusSidebarFilterRow, videos_button);
    gtk_widget_class_bind_template_child (wclass, NautilusSidebarFilterRow, audio_button);
    gtk_widget_class_bind_template_child (wclass, NautilusSidebarFilterRow, documents_button);
    gtk_widget_class_bind_template_child (wclass, NautilusSidebarFilterRow, archives_button);
    gtk_widget_class_bind_template_child (wclass, NautilusSidebarFilterRow, text_button);
}
