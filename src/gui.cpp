// GTK3 desktop interface for SmartRoad.
#include "gui.h"
#include <gtk/gtk.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <set>
#include "DefectManager.h"
#include "MapExporter.h"
#include "Pipeline.h"
#include "RoadGraph.h"

#ifndef SMARTROAD_WEB_DIR
#define SMARTROAD_WEB_DIR "web"
#endif

namespace {

enum Col { C_RANK, C_ID, C_TYPE, C_SEV, C_SCORE, C_LOC, C_IMG, C_PATH, C_COUNT };

struct App {
    DefectManager mgr;
    std::vector<std::string> images;
    std::set<std::string> analyzed;
    std::map<std::string, cv::Mat> annotated;
    RoutePlan route;
    bool hasRoute = false;

    GtkWidget *win = nullptr, *tree = nullptr, *preview = nullptr, *status = nullptr;
    GtkListStore* store = nullptr;
};

void setStatus(App* a, const std::string& s) { gtk_label_set_text(GTK_LABEL(a->status), s.c_str()); }

void message(App* a, GtkMessageType type, const std::string& text) {
    GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(a->win), GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, "%s", text.c_str());
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

void showImage(App* a, const std::string& path) {
    auto it = a->annotated.find(path);
    if (it == a->annotated.end() || it->second.empty()) {
        gtk_image_clear(GTK_IMAGE(a->preview));
        return;
    }
    cv::Mat rgb;
    cv::cvtColor(it->second, rgb, cv::COLOR_BGR2RGB);
    rgb = rgb.clone();  // continuous memory
    GdkPixbuf* wrap = gdk_pixbuf_new_from_data(rgb.data, GDK_COLORSPACE_RGB, FALSE, 8, rgb.cols, rgb.rows,
                                               static_cast<int>(rgb.step), nullptr, nullptr);
    GdkPixbuf* copy = gdk_pixbuf_copy(wrap);
    g_object_unref(wrap);
    int w = 520, h = std::max(1, rgb.rows * w / rgb.cols);
    GdkPixbuf* scaled = gdk_pixbuf_scale_simple(copy, w, h, GDK_INTERP_BILINEAR);
    g_object_unref(copy);
    gtk_image_set_from_pixbuf(GTK_IMAGE(a->preview), scaled);
    g_object_unref(scaled);
}

void refreshList(App* a) {
    gtk_list_store_clear(a->store);
    auto ranked = a->mgr.ranked();
    int rank = 1;
    for (const Defect* d : ranked) {
        char score[32], loc[64];
        std::snprintf(score, sizeof score, "%.1f", d->priorityScore());
        if (d->location().valid) std::snprintf(loc, sizeof loc, "%.5f, %.5f", d->location().lat, d->location().lon);
        else std::snprintf(loc, sizeof loc, "No GPS (flagged)");
        std::string name = std::filesystem::path(d->image()).filename().string();
        GtkTreeIter it;
        gtk_list_store_append(a->store, &it);
        gtk_list_store_set(a->store, &it, C_RANK, rank++, C_ID, d->id(), C_TYPE, toString(d->type()), C_SEV,
                           toString(d->severity()), C_SCORE, score, C_LOC, loc, C_IMG, name.c_str(), C_PATH,
                           d->image().c_str(), -1);
    }
    char buf[160];
    std::snprintf(buf, sizeof buf, "%zu defect(s) | %zu image(s) loaded | %zu without GPS", a->mgr.all().size(),
                  a->images.size(), a->mgr.unlocated().size());
    setStatus(a, buf);
}

std::string selectedPath(App* a) {
    GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(a->tree));
    GtkTreeModel* model;
    GtkTreeIter it;
    if (!gtk_tree_selection_get_selected(sel, &model, &it)) return "";
    gchar* p = nullptr;
    gtk_tree_model_get(model, &it, C_PATH, &p, -1);
    std::string s = p ? p : "";
    g_free(p);
    return s;
}

void onSelect(GtkTreeSelection*, gpointer data) {
    App* a = static_cast<App*>(data);
    std::string p = selectedPath(a);
    if (!p.empty()) showImage(a, p);
}

void onAdd(GtkButton*, gpointer data) {
    App* a = static_cast<App*>(data);
    GtkWidget* dlg = gtk_file_chooser_dialog_new("Add road images", GTK_WINDOW(a->win), GTK_FILE_CHOOSER_ACTION_OPEN,
                                                 "_Cancel", GTK_RESPONSE_CANCEL, "_Add", GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dlg), TRUE);
    GtkFileFilter* f = gtk_file_filter_new();
    gtk_file_filter_set_name(f, "Images");
    for (const char* pat : {"*.jpg", "*.jpeg", "*.JPG", "*.JPEG", "*.png", "*.PNG", "*.bmp"})
        gtk_file_filter_add_pattern(f, pat);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), f);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dlg));
        int added = 0;
        for (GSList* l = files; l; l = l->next) {
            std::string p = static_cast<char*>(l->data);
            if (std::find(a->images.begin(), a->images.end(), p) == a->images.end()) { a->images.push_back(p); ++added; }
            g_free(l->data);
        }
        g_slist_free(files);
        setStatus(a, std::to_string(added) + " image(s) added. Press Analyze.");
    }
    gtk_widget_destroy(dlg);
}

void onAnalyze(GtkButton*, gpointer data) {
    App* a = static_cast<App*>(data);
    if (a->images.empty()) { message(a, GTK_MESSAGE_INFO, "Add one or more road images first."); return; }
    int total = 0, clean = 0, failed = 0;
    std::string last;
    for (const auto& p : a->images) {
        if (a->analyzed.count(p)) continue;
        cv::Mat ann;
        std::string err;
        int n = processImage(p, a->mgr, ann, err);
        a->analyzed.insert(p);
        if (n < 0) { ++failed; continue; }
        if (n == 0) ++clean;
        total += n;
        a->annotated[p] = ann;
        last = p;
    }
    a->hasRoute = false;
    refreshList(a);
    if (!last.empty()) showImage(a, last);
    char buf[200];
    std::snprintf(buf, sizeof buf, "Analysis done: %d new defect(s), %d image(s) with none, %d unreadable.", total, clean, failed);
    setStatus(a, buf);
    if (!a->mgr.unlocated().empty())
        message(a, GTK_MESSAGE_INFO,
                "Some defects have no GPS data in their photo, so they are flagged and not placed on the map.\n"
                "Select one and press \"Set location\" to enter coordinates.");
}

void onSetLocation(GtkButton*, gpointer data) {
    App* a = static_cast<App*>(data);
    std::string p = selectedPath(a);
    if (p.empty()) { message(a, GTK_MESSAGE_INFO, "Select a defect in the list first."); return; }
    GtkWidget* dlg = gtk_dialog_new_with_buttons("Set location for this image", GTK_WINDOW(a->win), GTK_DIALOG_MODAL,
                                                 "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, nullptr);
    GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget* lat = gtk_entry_new();
    GtkWidget* lon = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(lat), "Latitude, e.g. 30.3165");
    gtk_entry_set_placeholder_text(GTK_ENTRY(lon), "Longitude, e.g. 78.0322");
    gtk_box_pack_start(GTK_BOX(box), gtk_label_new("Applies to every defect found in this photo."), FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(box), lat, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(box), lon, FALSE, FALSE, 4);
    gtk_widget_show_all(dlg);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        char *e1 = nullptr, *e2 = nullptr;
        const char *s1 = gtk_entry_get_text(GTK_ENTRY(lat)), *s2 = gtk_entry_get_text(GTK_ENTRY(lon));
        double la = std::strtod(s1, &e1), lo = std::strtod(s2, &e2);
        if (*s1 && *s2 && *e1 == '\0' && *e2 == '\0' && la >= -90 && la <= 90 && lo >= -180 && lo <= 180) {
            GeoPoint g;
            g.lat = la; g.lon = lo; g.valid = true;
            a->mgr.setLocationForImage(p, g);
            a->hasRoute = false;
            refreshList(a);
        } else {
            message(a, GTK_MESSAGE_ERROR, "Enter a latitude between -90 and 90 and a longitude between -180 and 180.");
        }
    }
    gtk_widget_destroy(dlg);
}

void onRoute(GtkButton*, gpointer data) {
    App* a = static_cast<App*>(data);
    a->route = RoutePlanner::plan(a->mgr.ranked());
    if (a->route.empty()) {
        a->hasRoute = false;
        message(a, GTK_MESSAGE_INFO, "No defects with coordinates yet. Analyze images that have GPS data, or set locations manually.");
        return;
    }
    a->hasRoute = true;
    std::string s = "Visit order (defect ids): ";
    for (size_t i = 0; i < a->route.visitOrder.size(); ++i) s += (i ? " -> " : "") + std::to_string(a->route.visitOrder[i]);
    char km[64];
    std::snprintf(km, sizeof km, "\n\nTotal graph distance: %.2f km", a->route.totalKm);
    s += km;
    s += "\n\nPress \"Open map\" to see the route.";
    message(a, GTK_MESSAGE_INFO, s);
}

void onMap(GtkButton*, gpointer data) {
    App* a = static_cast<App*>(data);
    if (a->mgr.empty()) { message(a, GTK_MESSAGE_INFO, "Nothing to show yet. Analyze some images first."); return; }
    std::string dir = SMARTROAD_WEB_DIR;
    std::string dataPath = dir + "/data.js";
    if (!exportMapData(dataPath, a->mgr.ranked(), a->hasRoute ? &a->route : nullptr)) {
        message(a, GTK_MESSAGE_ERROR, "Could not write " + dataPath);
        return;
    }
    // Serve the page over http so map tiles load (OpenStreetMap blocks file:// pages).
    static bool serverStarted = false;
    if (!serverStarted) {
        std::string cmd = "python3 -m http.server 8000 --bind 127.0.0.1 --directory \"" + dir + "\"";
        GError* e = nullptr;
        if (g_spawn_command_line_async(cmd.c_str(), &e)) serverStarted = true;
        if (e) g_error_free(e);
        g_usleep(600000);
    }
    const char* url = "http://localhost:8000/map.html";
    GError* err = nullptr;
    if (g_spawn_command_line_async("explorer.exe http://localhost:8000/map.html", &err)) return;  // WSL
    if (err) { g_error_free(err); err = nullptr; }
    if (!g_app_info_launch_default_for_uri(url, nullptr, &err)) {
        message(a, GTK_MESSAGE_ERROR, std::string("Open this address in your browser:\n") + url);
        if (err) g_error_free(err);
    }
}

void onCsv(GtkButton*, gpointer data) {
    App* a = static_cast<App*>(data);
    if (a->mgr.empty()) { message(a, GTK_MESSAGE_INFO, "No defects to export."); return; }
    GtkWidget* dlg = gtk_file_chooser_dialog_new("Export CSV", GTK_WINDOW(a->win), GTK_FILE_CHOOSER_ACTION_SAVE,
                                                 "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, nullptr);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), "smartroad_defects.csv");
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        gchar* f = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
        if (exportCsv(f, a->mgr.ranked())) setStatus(a, std::string("Saved ") + f);
        else message(a, GTK_MESSAGE_ERROR, "Could not write the file.");
        g_free(f);
    }
    gtk_widget_destroy(dlg);
}

void onClear(GtkButton*, gpointer data) {
    App* a = static_cast<App*>(data);
    a->mgr.clear();
    a->images.clear();
    a->analyzed.clear();
    a->annotated.clear();
    a->hasRoute = false;
    gtk_image_clear(GTK_IMAGE(a->preview));
    refreshList(a);
}

GtkWidget* makeButton(const char* label, GCallback cb, App* a) {
    GtkWidget* b = gtk_button_new_with_label(label);
    g_signal_connect(b, "clicked", cb, a);
    return b;
}

void addColumn(GtkWidget* tree, const char* title, int col) {
    GtkCellRenderer* r = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* c = gtk_tree_view_column_new_with_attributes(title, r, "text", col, nullptr);
    gtk_tree_view_column_set_resizable(c, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), c);
}

}  // namespace

int runGui(int argc, char** argv) {
    gtk_init(&argc, &argv);
    static App app;
    App* a = &app;

    a->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(a->win), "SmartRoad - Road Condition Monitoring");
    gtk_window_set_default_size(GTK_WINDOW(a->win), 1180, 640);
    g_signal_connect(a->win, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
    gtk_container_add(GTK_CONTAINER(a->win), vbox);

    GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(bar), makeButton("Add images", G_CALLBACK(onAdd), a), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bar), makeButton("Analyze", G_CALLBACK(onAnalyze), a), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bar), makeButton("Set location", G_CALLBACK(onSetLocation), a), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bar), makeButton("Plan route", G_CALLBACK(onRoute), a), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bar), makeButton("Open map", G_CALLBACK(onMap), a), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bar), makeButton("Export CSV", G_CALLBACK(onCsv), a), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(bar), makeButton("Clear all", G_CALLBACK(onClear), a), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), bar, FALSE, FALSE, 0);

    GtkWidget* paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

    a->store = gtk_list_store_new(C_COUNT, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                  G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    a->tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(a->store));
    addColumn(a->tree, "Rank", C_RANK);
    addColumn(a->tree, "ID", C_ID);
    addColumn(a->tree, "Type", C_TYPE);
    addColumn(a->tree, "Severity", C_SEV);
    addColumn(a->tree, "Score", C_SCORE);
    addColumn(a->tree, "Location", C_LOC);
    addColumn(a->tree, "Image", C_IMG);
    g_signal_connect(gtk_tree_view_get_selection(GTK_TREE_VIEW(a->tree)), "changed", G_CALLBACK(onSelect), a);
    GtkWidget* sw1 = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(sw1), a->tree);
    gtk_paned_pack1(GTK_PANED(paned), sw1, TRUE, FALSE);

    a->preview = gtk_image_new();
    GtkWidget* sw2 = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_container_add(GTK_CONTAINER(sw2), a->preview);
    gtk_widget_set_size_request(sw2, 540, -1);
    gtk_paned_pack2(GTK_PANED(paned), sw2, FALSE, FALSE);

    a->status = gtk_label_new("Add road photos, then press Analyze.");
    gtk_label_set_xalign(GTK_LABEL(a->status), 0.0f);
    gtk_box_pack_start(GTK_BOX(vbox), a->status, FALSE, FALSE, 0);

    gtk_widget_show_all(a->win);
    gtk_main();
    return 0;
}
