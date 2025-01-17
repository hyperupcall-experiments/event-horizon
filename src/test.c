#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

struct our_state {
    // ...
    struct wl_compositor *compositor;
    // ...
};

static void
registry_handle_global(void *data, struct wl_registry *wl_registry,
		uint32_t name, const char *interface, uint32_t version) {
	printf("interface: '%s', version: %d, name: %d\n",
			interface, version, name);
	struct our_state *state = data;
	printf("%s == %s\n", interface, wl_compositor_interface.name);
	printf("%p\n", state->compositor);
	printf("debug\n");
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(
            wl_registry, name, &wl_compositor_interface, 4);
    }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name) {
	// This space deliberately left blank
}

void surface_enter_handler(void *data, struct wl_surface *surface, struct wl_output *output) {
    printf("enter\n");
}

void surface_leave_handler(void *data, struct wl_surface *surface, struct wl_output *output) {
    printf("leave\n");
}


static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

int main(void) {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Error connecting\n");
        exit(1);
    }
    printf("Connected!\n");
    // wl_display_disconnect(display);
    // exit(0);

 //   	struct our_state state = { 0 };
 //    struct wl_registry *registry = wl_display_get_registry(display);
	// wl_registry_add_listener(registry, &registry_listener, NULL);
	// wl_display_roundtrip(display);

	// struct wl_surface *surface = wl_compositor_create_surface(state.compositor);


	while (wl_display_dispatch(display) != -1) {
	    // struct wl_shell_surface *shell_surface = display;
	    // wl_shell_surface_set_title(shell_surface, "Hello World!");


	    // struct wl_compositor *compositor = wl_display_get_compositor(display);
	    // struct wl_surface *surface = wl_compositor_create_surface(compositor);
	    // struct wl_surface_listener listener = {
	    //     .enter = surface_enter_handler,
	    //     .leave = surface_leave_handler
	    // };
		// wl_surface_add_listener(surface, &listener, NULL);
    }
    wl_display_disconnect(display);
}
