//------------------------------------------------------------------------------
//--X11 definitions
//------------------------------------------------------------------------------

#define X11_OP_REQ_CREATE_WINDOW	0x01
#define X11_OP_REQ_MAP_WINDOW		0x08
#define X11_OP_REQ_UNMAP_WINDOW		0x0A
#define X11_OP_REQ_UNGRAB_KB		0x20
#define X11_OP_REQ_GRAB_KEY			0x21
#define X11_OP_REQ_WARP				0x29
#define X11_OP_REQ_CREATE_PIX		0x35
#define X11_OP_REQ_CREATE_GC		0x37
#define X11_OP_REQ_COPY_AREA		0x3E
#define X11_OP_REQ_PUT_IMG			0x48
#define X11_OP_REQ_GET_EXT			0x62

#define PAD(x) ((4-(x&3))&3)

//------------------------------------------------------------------------------
//--X11 structures
//------------------------------------------------------------------------------

struct x11_conn_req
{
	uint8_t order;
	uint8_t pad1;
	uint16_t major,
			minor;
	uint16_t auth_proto_len,
			auth_data_len;
	uint16_t pad2;
	uint8_t *auth_proto,
			*auth_data;
};

struct x11_conn_reply
{
	uint8_t success;
	uint8_t pad1;
	uint16_t major,
			minor;
	uint16_t length;
};

struct x11_conn_setup
{
	uint32_t release;
	uint32_t id_base,
			id_mask;
	uint32_t motion_buffer_size;
	uint16_t vendor_length;
	uint16_t request_max;
	uint8_t roots;
	uint8_t formats;
	uint8_t image_order;
	uint8_t bitmap_order;
	uint8_t scanline_unit,
			scanline_pad;
	uint8_t keycode_min,
			keycode_max;
	uint32_t pad;
};

struct x11_pixmap_format
{
	uint8_t depth;
	uint8_t bpp;
	uint8_t scanline_pad;
	uint8_t pad1;
	uint32_t pad2;
};

struct x11_root_window
{
	uint32_t id;
	uint32_t colormap;
	uint32_t white,
			black;
	uint32_t input_mask;   
	uint16_t width,
			height;
	uint16_t width_mm,
			height_mm;
	uint16_t maps_min,
			maps_max;
	uint32_t root_visual_id;
	uint8_t backing_store;
	uint8_t save_unders;
	uint8_t depth;
	uint8_t depths;
};

struct x11_depth
{
	uint8_t depth;
	uint8_t pad1;
	uint16_t visuals;
	uint32_t pad2;
};

struct x11_visual
{
	uint8_t group;
	uint8_t bits;
	uint16_t colormap_entries;
	uint32_t mask_red,
			mask_green,
			mask_blue;
	uint32_t pad;
};

struct x11_connection
{
	int sock;
	struct x11_conn_reply header;
	struct x11_conn_setup *setup;
	struct x11_pixmap_format *format;
	struct x11_root_window *root;
	struct x11_depth *depth;
	struct x11_visual *visual;
};

struct x11_event_key
{
	uint8_t code;
	uint8_t detail;
	uint16_t seq;
	uint32_t time;
	uint32_t root;
	uint32_t event;
	uint32_t target;
	uint16_t root_x, root_y;
	uint16_t x, y;
	uint16_t modifiers;
	uint8_t same_screen;
	uint8_t pad;
};

struct x11_error
{
	uint8_t success;
	uint8_t code;
	uint16_t seq;
	uint32_t id;
	uint16_t op_major;
	uint8_t op_minor;
	uint8_t pad[21];
};

#define X11_EV_KEY_DOWN 0x02
#define X11_EV_KEY_UP 0x03

union x11_event
{
	struct x11_error error;
	struct x11_event_key key;
};

//------------------------------------------------------------------------------
//--X11 functions
//------------------------------------------------------------------------------

int x11_connect(struct x11_connection *conn, char *display, char *xauth);
int x11_read_xauthority(int fd, struct x11_conn_req *req);

int x11_handshake(struct x11_connection *conn);
uint32_t x11_generate_id(struct x11_connection *conn);
#define X11_FLAG_GC_FUNC 0x00000001
#define X11_FLAG_GC_PLANE 0x00000002
#define X11_FLAG_GC_BG 0x00000004
#define X11_FLAG_GC_FG 0x00000008
#define X11_FLAG_GC_LINE_WIDTH 0x00000010
#define X11_FLAG_GC_LINE_STYLE 0x00000020
#define X11_FLAG_GC_FONT 0x00004000
#define X11_FLAG_GC_EXPOSE 0x00010000
void x11_create_gc(struct x11_connection *conn, uint32_t id, uint32_t target, uint32_t flags, uint32_t *list);

#define X11_FLAG_WIN_BG_IMG 0x00000001
#define X11_FLAG_WIN_BG_COLOR 0x00000002
#define X11_FLAG_WIN_BORDER_IMG 0x00000004
#define X11_FLAG_WIN_BORDER_COLOR 0x00000008
#define X11_FLAG_WIN_EVENT 0x00000800

#define X11_EV_MASK_KEY_DOWN 0x00000001
#define X11_EV_MASK_KEY_UP 0x00000002
#define X11_EV_MASK_MOUSE_DOWN 0x00000004
#define X11_EV_MASK_MOUSE_UP 0x00000008
#define X11_EV_MASK_WIN_IN 0x00000010
#define X11_EV_MASK_WIN_OUT 0x00000020
void x11_create_win(struct x11_connection *conn, uint32_t id, uint32_t parent,
					uint16_t x, uint16_t y, uint16_t w, uint16_t h,
					uint16_t border, uint16_t group, uint32_t visual,
					uint32_t flags, uint32_t *list);
void x11_map_window(struct x11_connection *conn, uint32_t id);
void x11_unmap_window(struct x11_connection *conn, uint32_t id);

#define X11_IMG_FORMAT_MONO 0x00
#define X11_IMG_FORMAT_XY 0x01
#define X11_IMG_FORMAT_Z 0x02
void x11_put_img(struct x11_connection *conn, uint8_t format, uint32_t target, uint32_t gc,
				uint16_t w, uint16_t h, uint16_t x, uint16_t y, uint8_t depth, void *data);
void x11_create_pix(struct x11_connection *conn, uint32_t pix, uint32_t target,
					uint16_t w, uint16_t h, uint8_t depth);

void x11_copy_area(struct x11_connection *conn, uint32_t source, uint32_t target, uint32_t gc,
					uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t w, uint16_t h);
void x11_grab_key(struct x11_connection *conn, uint8_t key, uint16_t modifiers, uint32_t win, uint8_t owner, uint8_t sync_key, uint8_t sync_mouse);
void x11_ungrab_kb(struct x11_connection *conn, uint32_t time);
void x11_warp(struct x11_connection *conn, uint32_t src, uint32_t dest, uint16_t src_x, uint16_t src_y, uint16_t w, uint16_t h, uint16_t x, uint16_t y);
uint8_t x11_get_ext(struct x11_connection *conn, uint16_t len, uint8_t *name);

//------------------------------------------------------------------------------
//--Shape Extension functions
//------------------------------------------------------------------------------

#define X11_OP_SHAPE_RECT 1

#define X11_SHAPE_TYPE_BOUND 0x00
#define X11_SHAPE_TYPE_CLIP 0x01
#define X11_SHAPE_TYPE_INPUT 0x02

#define X11_SHAPE_OP_SET 0x00
#define X11_SHAPE_OP_UNION 0x01
#define X11_SHAPE_OP_INTERSECT 0x02
#define X11_SHAPE_OP_SUB 0x03
#define X11_SHAPE_OP_INVERT 0x04


void x11_shape_rect(struct x11_connection *conn, int base, uint8_t op, uint8_t type, uint8_t order,
					uint32_t target, uint16_t x, uint16_t y, uint8_t n, void *rects);
