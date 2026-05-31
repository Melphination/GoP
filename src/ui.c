// GTK4 Sandboxed File Explorer + Simple Editor
//
// 기능:
// - 파일 추가
// - 삭제
// - 이동
// - 이름 변경 (직접 입력)
// - 파일 편집 (메모장 스타일)
//
// 지정된 SANDBOX_DIR 내부에서만 동작
//
// Compile:
// gcc explorer.c `pkg-config --cflags --libs gtk4` -o explorer

#include <gtk/gtk.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define PATH_SEP "\\"
#else
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEP "/"
#endif

#define SANDBOX_DIR "./sandbox"

static char dragged_name[256];

// -----------------------------------
// 구조체
// -----------------------------------

typedef struct {
    GtkWidget *listbox;
    GtkWidget *archive_box;
    GtkWidget *status;
    GtkWidget *window;
} AppWidgets;

// -----------------------------------
// 유틸
// -----------------------------------

static void set_status(AppWidgets *app, const char *msg) {
    gtk_label_set_text(GTK_LABEL(app->status), msg);
}

static void build_path(char *buffer, const char *filename) {
    snprintf(buffer, 512, "%s%s%s",
             SANDBOX_DIR,
             PATH_SEP,
             filename);
}

static GtkListBoxRow* get_selected_row(AppWidgets *app) {
    return gtk_list_box_get_selected_row(
        GTK_LIST_BOX(app->listbox));
}

static const char* get_row_text(GtkListBoxRow *row) {

    GtkWidget *child =
        gtk_list_box_row_get_child(row);

    return gtk_label_get_text(GTK_LABEL(child));
}


// 파일 드래그

static GdkContentProvider* drag_prepare(
    GtkDragSource *source,
    double x,
    double y,
    gpointer user_data)
{
    GtkWidget *row = GTK_WIDGET(user_data);

    GtkWidget *child =
        gtk_list_box_row_get_child(
            GTK_LIST_BOX_ROW(row));

    const char *text =
        gtk_label_get_text(GTK_LABEL(child));

    strcpy(dragged_name, text);

    GValue value = G_VALUE_INIT;

    g_value_init(&value, G_TYPE_STRING);

    g_value_set_string(&value, text);

    return gdk_content_provider_new_for_value(&value);
}

static void setup_drag_source(GtkWidget *row)
{
    GtkDragSource *source =
        GTK_DRAG_SOURCE(
            gtk_drag_source_new());

    gtk_drag_source_set_actions(
        source,
        GDK_ACTION_MOVE);

    g_signal_connect(source,
                     "prepare",
                     G_CALLBACK(drag_prepare),
                     row);

    gtk_widget_add_controller(
        row,
        GTK_EVENT_CONTROLLER(source));
}

// -----------------------------------
// 파일 목록 새로고침
// -----------------------------------

static void refresh_file_list(AppWidgets *app) {

    GtkWidget *child =
        gtk_widget_get_first_child(app->listbox);

    while (child) {

        GtkWidget *next =
            gtk_widget_get_next_sibling(child);

        gtk_list_box_remove(
            GTK_LIST_BOX(app->listbox),
            child);

        child = next;
    }

    DIR *dir = opendir(SANDBOX_DIR);

    if (!dir) {
        set_status(app, "Cannot open sandbox.");
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        GtkWidget *label =
            gtk_label_new(entry->d_name);

        gtk_widget_set_margin_start(label, 10);
        gtk_widget_set_margin_end(label, 10);
        gtk_widget_set_margin_top(label, 5);
        gtk_widget_set_margin_bottom(label, 5);

        GtkWidget *row =
            gtk_list_box_row_new();

        gtk_list_box_row_set_child(
            GTK_LIST_BOX_ROW(row),
            label);

        setup_drag_source(row);

        gtk_list_box_append(
            GTK_LIST_BOX(app->listbox),
            row);
    }

    closedir(dir);
}

static gboolean drop_archive(GtkDropTarget *target,
                             const GValue *value,
                             double x,
                             double y,
                             gpointer user_data)
{
    AppWidgets *app = user_data;

    const char *dropped_name =
        g_value_get_string(value);

    if (!dropped_name)
        return FALSE;

    char archive_dir[512];

    snprintf(archive_dir,
             sizeof(archive_dir),
             "%s%sarchive",
             SANDBOX_DIR,
             PATH_SEP);

    mkdir(archive_dir, 0777);

    char old_path[512];
    char new_path[512];

    build_path(old_path, dropped_name);

    snprintf(new_path,
             sizeof(new_path),
             "%s%s%s",
             archive_dir,
             PATH_SEP,
             dragged_name);

    if (rename(old_path, new_path) == 0) {

        refresh_file_list(app);

        char msg[256];

        snprintf(msg,
                 sizeof(msg),
                 "Dragged '%s' into archive/",
                 dragged_name);

        set_status(app, msg);
    }

    dragged_name[0] = '\0';

    return TRUE;
}

// -----------------------------------
// 파일 추가
// -----------------------------------

static void add_file(GtkButton *button, gpointer data) {

    AppWidgets *app = data;

    static int counter = 1;

    char filename[128];

    snprintf(filename,
             sizeof(filename),
             "new_file_%d.txt",
             counter++);

    char path[512];
    build_path(path, filename);

    FILE *fp = fopen(path, "w");

    if (!fp) {
        set_status(app, "Create failed.");
        return;
    }

    fprintf(fp, "New file.\n");
    fclose(fp);

    refresh_file_list(app);

    char msg[256];
    snprintf(msg,
             sizeof(msg),
             "Created: %s",
             filename);

    set_status(app, msg);
}

// -----------------------------------
// 삭제
// -----------------------------------

static void delete_file(GtkButton *button, gpointer data) {

    AppWidgets *app = data;

    GtkListBoxRow *row =
        get_selected_row(app);

    if (!row) {
        set_status(app, "No file selected.");
        return;
    }

    const char *name = get_row_text(row);

    char path[512];
    build_path(path, name);

    if (remove(path) == 0) {

        refresh_file_list(app);

        char msg[256];

        snprintf(msg,
                 sizeof(msg),
                 "Deleted: %s",
                 name);

        set_status(app, msg);

    } else {
        set_status(app, "Delete failed.");
    }
}

// -----------------------------------
// 이동
// -----------------------------------

static void move_file(GtkButton *button, gpointer data) {

    AppWidgets *app = data;

    GtkListBoxRow *row =
        get_selected_row(app);

    if (!row) {
        set_status(app, "No file selected.");
        return;
    }

    const char *name = get_row_text(row);

    char archive_dir[512];

    snprintf(archive_dir,
             sizeof(archive_dir),
             "%s%sarchive",
             SANDBOX_DIR,
             PATH_SEP);

    mkdir(archive_dir, 0777);

    char old_path[512];
    build_path(old_path, name);

    char new_path[512];

    snprintf(new_path,
             sizeof(new_path),
             "%s%s%s",
             archive_dir,
             PATH_SEP,
             name);

    if (rename(old_path, new_path) == 0) {

        refresh_file_list(app);

        char msg[256];

        snprintf(msg,
                 sizeof(msg),
                 "Moved '%s' to archive/",
                 name);

        set_status(app, msg);

    } else {
        set_status(app, "Move failed.");
    }
}

// -----------------------------------
// 이름 변경 Dialog
// -----------------------------------

typedef struct {
    AppWidgets *app;
    char old_name[256];
    GtkWidget *entry;
} RenameData;

static void rename_confirm(GtkButton *button,
                           gpointer user_data) {

    RenameData *data = user_data;

    const char *new_name =
        gtk_editable_get_text(
            GTK_EDITABLE(data->entry));

    if (strlen(new_name) == 0)
        return;

    char old_path[512];
    char new_path[512];

    build_path(old_path, data->old_name);
    build_path(new_path, new_name);

    if (rename(old_path, new_path) == 0) {

        refresh_file_list(data->app);

        char msg[256];

        snprintf(msg,
                 sizeof(msg),
                 "Renamed '%s' -> '%s'",
                 data->old_name,
                 new_name);

        set_status(data->app, msg);

    } else {
        set_status(data->app, "Rename failed.");
    }

    GtkWidget *window =
        gtk_widget_get_ancestor(
            GTK_WIDGET(button),
            GTK_TYPE_WINDOW);

    gtk_window_destroy(GTK_WINDOW(window));

    g_free(data);
}

static void rename_file(GtkButton *button,
                        gpointer data) {

    AppWidgets *app = data;

    GtkListBoxRow *row =
        get_selected_row(app);

    if (!row) {
        set_status(app, "No file selected.");
        return;
    }

    const char *old_name =
        get_row_text(row);

    GtkWidget *dialog =
        gtk_window_new();

    gtk_window_set_title(
        GTK_WINDOW(dialog),
        "Rename File");

    gtk_window_set_default_size(
        GTK_WINDOW(dialog),
        300,
        100);

    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);

    gtk_window_set_child(
        GTK_WINDOW(dialog),
        box);

    GtkWidget *entry =
        gtk_entry_new();

    gtk_editable_set_text(
        GTK_EDITABLE(entry),
        old_name);

    gtk_box_append(GTK_BOX(box), entry);

    GtkWidget *confirm =
        gtk_button_new_with_label("Rename");

    gtk_box_append(GTK_BOX(box), confirm);

    RenameData *rename_data =
        g_malloc(sizeof(RenameData));

    rename_data->app = app;
    rename_data->entry = entry;

    strcpy(rename_data->old_name,
           old_name);

    g_signal_connect(confirm,
                     "clicked",
                     G_CALLBACK(rename_confirm),
                     rename_data);

    gtk_window_present(GTK_WINDOW(dialog));
}

// -----------------------------------
// 파일 편집기
// -----------------------------------

typedef struct {
    char path[512];
    GtkWidget *textview;
    AppWidgets *app;
} EditorData;

static void save_file(GtkButton *button,
                      gpointer user_data) {

    EditorData *data = user_data;

    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(data->textview));

    GtkTextIter start, end;

    gtk_text_buffer_get_bounds(
        buffer,
        &start,
        &end);

    char *text =
        gtk_text_buffer_get_text(
            buffer,
            &start,
            &end,
            FALSE);

    FILE *fp = fopen(data->path, "w");

    if (!fp) {
        set_status(data->app,
                   "Save failed.");
        g_free(text);
        return;
    }

    fprintf(fp, "%s", text);

    fclose(fp);

    set_status(data->app,
               "File saved.");

    g_free(text);
}

static void edit_file(GtkButton *button,
                      gpointer user_data) {

    AppWidgets *app = user_data;

    GtkListBoxRow *row =
        get_selected_row(app);

    if (!row) {
        set_status(app,
                   "No file selected.");
        return;
    }

    const char *name =
        get_row_text(row);

    char path[512];
    build_path(path, name);

    GtkWidget *editor =
        gtk_window_new();

    gtk_window_set_title(
        GTK_WINDOW(editor),
        name);

    gtk_window_set_default_size(
        GTK_WINDOW(editor),
        600,
        400);

    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL,
                    5);

    gtk_window_set_child(
        GTK_WINDOW(editor),
        box);

    GtkWidget *scroll =
        gtk_scrolled_window_new();

    gtk_widget_set_vexpand(scroll, TRUE);

    gtk_box_append(GTK_BOX(box), scroll);

    GtkWidget *textview =
        gtk_text_view_new();

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scroll),
        textview);

    GtkTextBuffer *buffer =
        gtk_text_view_get_buffer(
            GTK_TEXT_VIEW(textview));

    FILE *fp = fopen(path, "r");

    if (fp) {

        fseek(fp, 0, SEEK_END);

        long size = ftell(fp);

        rewind(fp);

        char *content =
            g_malloc(size + 1);

        fread(content, 1, size, fp);

        content[size] = '\0';

        gtk_text_buffer_set_text(
            buffer,
            content,
            -1);

        fclose(fp);

        g_free(content);
    }

    GtkWidget *save_btn =
        gtk_button_new_with_label(
            "Save");

    gtk_box_append(GTK_BOX(box),
                   save_btn);

    EditorData *edata =
        g_malloc(sizeof(EditorData));

    strcpy(edata->path, path);

    edata->textview = textview;
    edata->app = app;

    g_signal_connect(save_btn,
                     "clicked",
                     G_CALLBACK(save_file),
                     edata);

    gtk_window_present(GTK_WINDOW(editor));
}

// -----------------------------------
// activate
// -----------------------------------

static void activate(GtkApplication *gtk_app,
                     gpointer user_data) {

    mkdir(SANDBOX_DIR, 0777);

    AppWidgets *app =
        g_malloc(sizeof(AppWidgets));

    GtkWidget *window =
        gtk_application_window_new(
            gtk_app);

    app->window = window;

    gtk_window_set_title(
        GTK_WINDOW(window),
        "Sandbox File Explorer");

    gtk_window_set_default_size(
        GTK_WINDOW(window),
        700,
        500);

    GtkWidget *main_box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL,
                    10);

    gtk_widget_set_margin_top(main_box, 10);
    gtk_widget_set_margin_bottom(main_box, 10);
    gtk_widget_set_margin_start(main_box, 10);
    gtk_widget_set_margin_end(main_box, 10);

    gtk_window_set_child(
        GTK_WINDOW(window),
        main_box);

    app->listbox =
        gtk_list_box_new();

    gtk_widget_set_vexpand(
        app->listbox,
        TRUE);

    gtk_box_append(GTK_BOX(main_box),
                   app->listbox);

    refresh_file_list(app);

    GtkWidget *buttons =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL,
                    10);

    gtk_box_append(GTK_BOX(main_box),
                   buttons);

    GtkWidget *add_btn =
        gtk_button_new_with_label("Add");

    GtkWidget *delete_btn =
        gtk_button_new_with_label("Delete");

    GtkWidget *rename_btn =
        gtk_button_new_with_label("Rename");

    GtkWidget *move_btn =
        gtk_button_new_with_label("Move");

    GtkWidget *edit_btn =
        gtk_button_new_with_label("Edit");

    gtk_box_append(GTK_BOX(buttons),
                   add_btn);

    gtk_box_append(GTK_BOX(buttons),
                   delete_btn);

    gtk_box_append(GTK_BOX(buttons),
                   rename_btn);

    gtk_box_append(GTK_BOX(buttons),
                   move_btn);

    gtk_box_append(GTK_BOX(buttons),
                   edit_btn);

    app->status =
        gtk_label_new("Ready.");

    gtk_box_append(GTK_BOX(main_box),
                   app->status);

    g_signal_connect(add_btn,
                     "clicked",
                     G_CALLBACK(add_file),
                     app);

    g_signal_connect(delete_btn,
                     "clicked",
                     G_CALLBACK(delete_file),
                     app);

    g_signal_connect(rename_btn,
                     "clicked",
                     G_CALLBACK(rename_file),
                     app);

    g_signal_connect(move_btn,
                     "clicked",
                     G_CALLBACK(move_file),
                     app);

    g_signal_connect(edit_btn,
                     "clicked",
                     G_CALLBACK(edit_file),
                     app);

    GtkWidget *archive_frame =
        gtk_frame_new("Archive Folder");

    gtk_widget_set_size_request(
        archive_frame,
        200,
        100);

    gtk_box_append(GTK_BOX(main_box),
                archive_frame);

    app->archive_box =
        gtk_label_new(
            "Drag files here to move");

    gtk_frame_set_child(
        GTK_FRAME(archive_frame),
        app->archive_box);

    GtkDropTarget *target =
        gtk_drop_target_new(
            G_TYPE_STRING,
            GDK_ACTION_MOVE);

    g_signal_connect(target,
                    "drop",
                    G_CALLBACK(drop_archive),
                    app);

    gtk_widget_add_controller(
        archive_frame,
        GTK_EVENT_CONTROLLER(target));

    gtk_window_present(GTK_WINDOW(window));
}

// -----------------------------------
// main
// -----------------------------------

int main(int argc, char **argv) {

    GtkApplication *app =
        gtk_application_new(
            "com.example.explorer",
            G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app,
                     "activate",
                     G_CALLBACK(activate),
                     NULL);

    int status =
        g_application_run(
            G_APPLICATION(app),
            argc,
            argv);

    g_object_unref(app);

    return status;
}