#include <gtk/gtk.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include "imagetools.h"
#include "squashfuse_hl.h"
#include "xmltools.h"
#include "mount.h"
#include "common.h"
#include "elf.h"

#define MOUNT_PREFIX "/tmp/.mount_"

static struct {
	GtkBuilder* builder;
	GtkWidget* window;
	const char* image;
} data;


static void on_main_delete_event() {
	gtk_main_quit();
}


static void on_btnLaunch_clicked() {
	// We'll do double-fork so there is one process for mounting & unmounting
	// FUSE and one for child process
	pid_t pid, pid2;
	if ((pid = fork()) == -1) {
		fprintf(stderr, "Fork failed");
	} else if (pid == 0) {
		// Child that does mounting and runnning
		char prefix[PATH_MAX];
		char path[PATH_MAX];
		strcpy(prefix, MOUNT_PREFIX);
		path[0] = 0; strncat(path, data.image, PATH_MAX - 1);
		strncat(prefix, basename(path), PATH_MAX - strlen(prefix) - 1);
		// having prefix passed to mount_image too long would be bad for many
		// reasons, so it is just randomly cut at 8th character.
		// If prefix is shorter, this has no effect
		prefix[strlen(MOUNT_PREFIX) + 8] = 0;
		
		if (mount_image(data.image, prefix, path)) {	// note: path is reused
			fprintf(stderr, "Failed to mount appimage");
			// TODO: all errors should have dialog here. Somehow
			return;
		}
		
		// Another fork is done to execute AppRun
		if ((pid2 = fork()) == -1) {
			fprintf(stderr, "Fork failed");
		} else if (pid2 == 0) {
			// AppRun
			if (chdir(path)) {
				fprintf(stderr, "chdir failed");
				exit(1);
			}
			char apprun[PATH_MAX];
			apprun[0] = 0;
			strncat(apprun, path, PATH_MAX - 1);
			strncat(apprun, "/AppRun", PATH_MAX - strlen(apprun) - 1);
			printf("%s\n", apprun);
			execl(apprun, apprun, NULL);
			fprintf(stderr, "exec failed");
			exit(1);
		} else {
			// Wait until child is finished and do unmount
			int stat;
			waitpid(pid2, &stat, 0);
			if (fork() == 0) {
				// This just silently executes fusermount, all errors are ignored
				execlp("fusermount", "fusermount", "-u", "-z", "-q", path, NULL);
				printf("fusermount failed!\n");
				exit(1);
			}
		}
		exit(0);
	} else {
		// Parent just exits
		gtk_widget_hide(data.window);
		gtk_main_quit();
	}
}


/** Fills dialog widgets with values and returns 0 on success */
static int fill_up_data() {
	unsigned long fs_offset;
	char path[PATH_MAX];
	char buffer[10240];
	GtkWidget* w;
	sqfs fs;
	
	fs_offset = get_elf_size(data.image);
	if (sqfs_open_image(&fs, data.image, fs_offset))
		FAIL("sqfs_open_image error");
	
	if (imagetools_find_icon(&fs, path) == 0) {
		imagetools_extract(&fs, path, "/tmp/icon.png");
		GError* error = NULL;
		GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file("/tmp/icon.png", &error);
		GdkPixbuf* s_pixbuf = NULL;
		if (pixbuf != NULL)
			s_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 64, 64, GDK_INTERP_HYPER);
		
		w = GTK_WIDGET(gtk_builder_get_object(data.builder, "imgAppIcon"));
		if ((s_pixbuf != NULL) && (w != NULL))
			gtk_image_set_from_pixbuf(GTK_IMAGE(w), s_pixbuf);
		
		g_object_unref(pixbuf);
		g_object_unref(s_pixbuf);
	}
	
	if (imagetools_find_appdata_xml(&fs, path) == 0) {
		
		// TODO: Maybe load desktop file?
		imagetools_extract(&fs, path, "/tmp/test.xml");
	
		xmlInitParser();
		xmlDocPtr doc = xmlParseFile("/tmp/test.xml");
		if (doc == NULL)
			FAIL("Failed to parse AppStream file");
		
		w = GTK_WIDGET(gtk_builder_get_object(data.builder, "lblAppName"));
		if ((GTK_LABEL(w) != NULL) && (!get_xpath_string(doc, "/component[@type=\"desktop\"]/name", buffer, 10240)))
			gtk_label_set_text(GTK_LABEL(w), buffer);
		w = GTK_WIDGET(gtk_builder_get_object(data.builder, "lblSummary"));
		if ((GTK_LABEL(w) != NULL) && (!get_xpath_string(doc, "/component[@type=\"desktop\"]/summary", buffer, 10240)))
			gtk_label_set_text(GTK_LABEL(w), buffer);
		xmlCleanupParser();
	
	} else {
		
		// Fallback used when there is no way to read app info
		w = GTK_WIDGET(gtk_builder_get_object(data.builder, "lblAppName"));
		strncpy(path, data.image, PATH_MAX);	// note: path is reused
		gtk_label_set_text(GTK_LABEL(w), basename(path));
		w = GTK_WIDGET(gtk_builder_get_object(data.builder, "lblSummary"));
		gtk_label_set_text(GTK_LABEL(w), " ");
		
	}
	
	sqfs_fd_close(fs.fd);
	return 0;
}


int main(int argc, char *argv[]) {
	gtk_init(&argc, &argv);
	
	data.builder = gtk_builder_new();
	gtk_builder_add_from_file (data.builder, "glade/appimage.glade", NULL);
	
	data.window = GTK_WIDGET(gtk_builder_get_object(data.builder, "main"));
	gtk_builder_add_callback_symbol(data.builder, "on_main_delete_event", on_main_delete_event);
	gtk_builder_add_callback_symbol(data.builder, "on_btnLaunch_clicked", on_btnLaunch_clicked);
	gtk_builder_connect_signals(data.builder, NULL);
	
	data.image = argv[1];
	fill_up_data();
	
	gtk_widget_show(data.window);
	gtk_main();
	return 0;
}
