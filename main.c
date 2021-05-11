/* See LICENSE for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <poll.h>

#ifdef __linux__
#include <bsd/string.h>
#endif

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/randr.h>

#include "config.h"

typedef struct monitor_t {
	char *name;
	int x, y, width, height;
	xcb_window_t window;
	xcb_pixmap_t pixmap;
	struct monitor_t *prev, *next;
} monitor_t;

typedef struct font_t {
	char *font_name;
    xcb_font_t font;
    int descent, height, width;
    uint32_t char_max;
    uint32_t char_min;
    xcb_charinfo_t *width_lut;
} font_t;

enum {
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_STATE,
	_NET_WM_STATE_STICKY,
	_NET_WM_STRUT,
	_NET_WM_STRUT_PARTIAL,
};
const char *atom_names[] = {
	"_NET_WM_WINDOW_TYPE",
	"_NET_WM_WINDOW_TYPE_DOCK",
	"_NET_WM_STATE",
	"_NET_WM_STATE_STICKY",
	"_NET_WM_STRUT",
	"_NET_WM_STRUT_PARTIAL",
};

static xcb_connection_t *connection;
static xcb_screen_t *screen;
static monitor_t *monhead, *montail;
static font_t chosen_font;
int bar_height = 30;

char *
get_date_time_string(void)
{
	time_t t = time(NULL);
	char *time_now = ctime(&t);
	return time_now;
}

char *
update_text(void)
{
	char *dt = get_date_time_string();
	char *text = malloc(strlen(dt));

	strlcpy(text, dt, strlen(dt));
	return text;
}

static void
test_cookie (xcb_void_cookie_t cookie,
	xcb_connection_t *c,
	char *errMessage)
{
	xcb_generic_error_t *error = xcb_request_check(c, cookie);
	if (error) {
		fprintf (stderr, "ERROR: %s: %"PRIu8"\n", errMessage, error->error_code);
		xcb_disconnect(c);
		exit (-1);
	}
}

// https://www.x.org/releases/X11R7.7/doc/xorg-docs/fonts/fonts.html
// https://www.x.org/wiki/guide/fonts/
void
load_font(xcb_connection_t *c, char *font_name)
{
	// Old font systems for X is weird
	// https://wiki.archlinux.org/index.php/X_Logical_Font_Description#Font_names
    xcb_query_font_cookie_t font_query;
    xcb_query_font_reply_t *font_info;
    xcb_void_cookie_t font_cookie;
    xcb_font_t font;

	font = xcb_generate_id(c);
	font_cookie = xcb_open_font_checked(c,
			font,
			strlen(font_name),
			font_name);

	test_cookie(font_cookie, c, "can't open font");

	font_query = xcb_query_font(c, font);
	font_info = xcb_query_font_reply(c, font_query, NULL);

	chosen_font.font_name = font_name;
	chosen_font.font = font;
	chosen_font.descent = font_info->font_descent;
	chosen_font.height = font_info->font_ascent + font_info->font_descent;
	chosen_font.width = font_info->max_bounds.character_width;
	free(font_info);
}

static xcb_gc_t
get_font_gc(xcb_connection_t *c,
	xcb_screen_t *s,
	xcb_window_t window,
	xcb_font_t f)
{
	/* create graphics context */
	xcb_gcontext_t gc = xcb_generate_id(c);
	uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
	uint32_t value_list[3] = {
		s->white_pixel, s->black_pixel, f };

	xcb_void_cookie_t gcCookie = xcb_create_gc_checked(c,
		gc,
		window,
		mask,
		value_list);

	test_cookie(gcCookie, c, "can't create gc");

	return gc;
}

static void
drawText(xcb_window_t window,
	int16_t x1,
	int16_t y1,
	const char *text)
{
	/* get graphics context */
	xcb_gcontext_t gc = get_font_gc(connection, screen, window, chosen_font.font);

	/* draw the text */
	xcb_void_cookie_t textCookie = xcb_image_text_8_checked(connection,
		strlen(text),
		window,
		gc,
		x1, y1,
		text);

	test_cookie(textCookie, connection, "can't paste text");

	/* free the gc */
	xcb_void_cookie_t gcCookie = xcb_free_gc(connection, gc);

	test_cookie(gcCookie, connection, "can't free gc");
}

// Add monitor to linked list of monitors
void
add_monitor(monitor_t *m)
{
	if (!monhead) {
		monhead = m;
	} else if (!montail) {
		montail = m;
		monhead->next = m;
		m->prev = monhead;
	} else {
		m->prev = montail;
		montail->next = m;
		montail = montail->next;
	}
}

void
configure_monitors(void)
{
    for (monitor_t *mon = monhead; mon; mon = mon->next) {
		// Set window masks (properties of the window ) and their values
		uint32_t masks = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
		uint32_t values[2];
		values[0] = screen->black_pixel;
		values[1] = XCB_EVENT_MASK_EXPOSURE;

		// Create window and set properties
		xcb_void_cookie_t windowCookie = xcb_create_window_checked(connection, /* Connection */
			screen->root_depth, /* depth (same as root)*/
			mon->window, /* window Id */
			screen->root, /* parent window */
			mon->x, mon->height - bar_height, /* x, y */
			mon->width, bar_height, /* width, height */
			10, /* border_width */
			XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class */
			screen->root_visual, /* visual */
			masks, values); /* properties of window and their values */
		test_cookie(windowCookie, connection, "can't create window");

		// Show window, seems it needs to be done before drawing/changing other properties
		xcb_void_cookie_t mapCookie = xcb_map_window(connection, mon->window);
		test_cookie(mapCookie, connection, "can't map window");

		// Stut tells window manager to reserve room for our bar
		int strut[4] = {0};
		strut[3] = bar_height;
		int strut_partial[12] = {0};
		strut_partial[3] = bar_height;
		strut_partial[9] = 0;
		strut_partial[10] = bar_height;

		const int atoms = sizeof(atom_names)/sizeof(char *);
		xcb_intern_atom_cookie_t atom_cookie[atoms];
		xcb_atom_t atom_list[atoms];
		xcb_intern_atom_reply_t *atom_reply;

		for (int i = 0; i < atoms; i++)
			atom_cookie[i] = xcb_intern_atom(connection, 0, strlen(atom_names[i]), atom_names[i]);

		for (int i = 0; i < atoms; i++) {
			atom_reply = xcb_intern_atom_reply(connection, atom_cookie[i], NULL);
			atom_list[i] = atom_reply->atom;
			free(atom_reply);
		}

		xcb_change_property(
				connection,
				XCB_PROP_MODE_REPLACE,
				mon->window,
				atom_list[_NET_WM_WINDOW_TYPE],
				XCB_ATOM_ATOM,
				32,
				1,
				&atom_list[_NET_WM_WINDOW_TYPE_DOCK]);

		// Make window stay on every workspace
		xcb_change_property(
				connection,
				XCB_PROP_MODE_APPEND,
				mon->window,
				atom_list[_NET_WM_STATE],
				XCB_ATOM_ATOM,
				32,
				2,
				&atom_list[_NET_WM_STATE_STICKY]);

		// Struts reserves room for the bar
		xcb_change_property(
				connection,
				XCB_PROP_MODE_REPLACE,
				mon->window,
				atom_list[_NET_WM_STRUT],
				XCB_ATOM_CARDINAL,
				32,
				4,
				strut);

		xcb_change_property(
				connection,
				XCB_PROP_MODE_REPLACE,
				mon->window,
				atom_list[_NET_WM_STRUT_PARTIAL],
				XCB_ATOM_CARDINAL,
				32,
				12,
				strut_partial);

		xcb_change_property(
				connection,
				XCB_PROP_MODE_REPLACE,
				mon->window,
				XCB_ATOM_WM_NAME,
				XCB_ATOM_STRING,
				8,
				strlen(PROGRAMNAME),
				PROGRAMNAME);

		// Make sure window is in right place
		// Some wms (openbox, awesome others?)
		// fignore or dont't handle? x,y rom xcb_create_window 
		xcb_configure_window(connection, mon->window,
				XCB_CONFIG_WINDOW_X |
				XCB_CONFIG_WINDOW_Y,
				(const uint32_t[]){ mon->x, mon->height - bar_height });
	}
	xcb_flush(connection);
}

void
update_monitor(monitor_t *m, char *text)
{
	// Get window geometty
	xcb_get_geometry_cookie_t geoCookie;
	xcb_get_geometry_reply_t *reply;
	geoCookie = xcb_get_geometry(connection, m->window);
	reply = xcb_get_geometry_reply(connection, geoCookie, NULL);

	if (!reply) {
		fprintf(stderr, "Couldn't get geometry of window\n");
		exit(-1);
	}

	drawText(m->window,
			reply->width - strlen(text) * chosen_font.width,
			reply->height/2 + chosen_font.height/2,
			text);

    free(reply);
}

void
update_monitors(void)
{
	char *text = update_text();
    for (monitor_t *m = monhead; m; m = m->next) {
		update_monitor(m, text);
	}
	free(text);
}
void
get_randr_monitors(void)
{
	xcb_randr_get_screen_resources_current_reply_t *reply = xcb_randr_get_screen_resources_current_reply(
			connection, xcb_randr_get_screen_resources_current(connection, screen->root), NULL);

	xcb_timestamp_t timestamp = reply->config_timestamp;
	int len = xcb_randr_get_screen_resources_current_outputs_length(reply);
	xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply);
	for (int i = 0; i < len; i++) {
		xcb_randr_get_output_info_reply_t *output = xcb_randr_get_output_info_reply(
				connection, xcb_randr_get_output_info(connection, randr_outputs[i], timestamp), NULL);
		if (output == NULL)
			continue;

		if (output->crtc == XCB_NONE || output->connection == XCB_RANDR_CONNECTION_DISCONNECTED)
			continue;

		xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(connection,
				xcb_randr_get_crtc_info(connection, output->crtc, timestamp), NULL);

		monitor_t *m;
		m = calloc(1, sizeof(monitor_t));
		if (!m) {
			fprintf(stderr, "Failed to allocate new monitor\n");
			exit(EXIT_FAILURE);
		}
		m->x = crtc->x;
		m->y = crtc->y;
		m->width = crtc->width;
		m->height = crtc->height;
		m->window = xcb_generate_id(connection);
		m->next = m->prev = NULL;

		add_monitor(m);

		free(crtc);
		free(output);
	}

	free(reply);
}

void
event_loop(void)
{
    xcb_generic_event_t *event;
	// Loop delay time in milliseconds
    struct pollfd fds[1] = {
        { .fd = -1, .events = POLLIN },
    };
    fds[0].fd = xcb_get_file_descriptor(connection);
	int poll_response;

	while(1) {
		if (xcb_connection_has_error(connection)) {
			fprintf(stderr, "xcb connection closed idk\n");
			break;
		}

		poll_response = poll(fds, 1, timeout);

		if (poll_response == 0) {
			update_monitors();
		} else if (poll_response > 0) {
			while((event = xcb_poll_for_event(connection))) {
				switch (event->response_type & ~0x80) {
					case XCB_EXPOSE: {
						xcb_expose_event_t *expose = (xcb_expose_event_t *)event;
						/* Find which monitor the window is on then update */
						for (monitor_t *m = monhead; m; m = m->next) {
							if (m->window == expose->window) {
								char *text = update_text();
								update_monitor(m, text);
								free(text);
							}
						}
						break;
					}
					default: {
						printf("unknown event %d\n", event->response_type);
						break;
					}
				}
			}
			free(event);
		} else {
			printf("poll response %d\n", poll_response);
		}
	}
}

void
sighandle(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
        exit(EXIT_SUCCESS);
}

void
cleanup(void)
{
	xcb_void_cookie_t fontCookie = xcb_close_font_checked(connection, chosen_font.font);
	test_cookie(fontCookie, connection, "can't close chosen_font");

    while (monhead) {
        monitor_t *next = monhead->next;
        xcb_destroy_window(connection, monhead->window);
        free(monhead);
        monhead = next;
    }
	xcb_disconnect(connection);
}


int
main (int argc, char *argv[])
{
	atexit(cleanup);
    signal(SIGINT, sighandle);
    signal(SIGTERM, sighandle);

	monhead = montail = NULL;

	int screenNum;
	/* Open the connection to the X server */
	connection = xcb_connect(NULL, &screenNum);
	if (xcb_connection_has_error(connection)) {
		fprintf(stderr, "Couldn't connect to X\n");
		exit(EXIT_FAILURE);
	}

	// Get current screen
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	// Find screen at index number of iterator
	for (int i = 0; i < screenNum; i++) {
		xcb_screen_next(&iter);
	}

	screen = iter.data;
	if (!screen) {
		fprintf(stderr, "ERROR: can't get screen\n");
		xcb_disconnect(connection);
		return -1;
	}

	get_randr_monitors();

	configure_monitors();

	load_font(connection, font);

	event_loop();

	exit(EXIT_SUCCESS);
	return 0;
}
