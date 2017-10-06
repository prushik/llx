//Low-Level X11 Terminal Emulator
//Author: Philip Rushik
//License: Beerware
//
#include "libc.h"
#include <asm/termios.h>
#include <asm/ioctl.h>
#include <asm/poll.h>
#include "psf.h"


//Check if all flags in y are set in x
#define FLAGS(x,y) ((x&(y))==(y))

//------------------------------------------------------------------------------
//--TermIOs struct (instead of including termios.h)
//------------------------------------------------------------------------------
/*struct termios {
	int c_iflag;
	int c_oflag;
	int c_cflag;
	int c_lflag;
	int c_cc[NCCS];
	int c_ispeed;
	int c_ospeed;
};*/

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

#define PAD(x) = ((4-(x%4))%4)

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
//--Generic / General Purpose functions
//------------------------------------------------------------------------------

//convert c terminated string to integer
int strtoi(char *s, char c)
{
	int i = 0;
	while (*s!=c)
	{
		i = i*10+*s-'0';
		s++;
	}
	return i;
}

//Don't know if you can tell, but this clearly isn't my code, style is all funky
void itostr(char* str, int len, unsigned int val)
{
	int i;

	for(i=1; i<=len; i++)
	{
		str[len-i] = (int) ((val % 10) + '0');
		val=val/10;
	}

	str[i-1] = '\0';
}

//Find character c in string s
//Why do I need this?
char *strchar(char *s, char c)
{
	int i;
	for (i=0; s[i]!='\0'; i++)
		if (s[i]==c)
				return &s[i];
	return 0;
}

//Case-insensitive compare. s2 must be lower case.
int lstrcmp(char *s1, const char *s2, char end)
{
	int i;
	for (i=0; s1[i]!=end; i++)
		if (s1[i]!=s2[i] && s1[i]!=s2[i]-0x20)
				return 0;
	return 1;
}

//Compare n characters, case insensitive. s2 must be lower case.
int lstrncmp(char *s1, const char *s2, size_t n)
{
	int i;
	for (i=0; i<n; i++)
		if (s1[i]!=s2[i] && s1[i]!=s2[i]-0x20)
				return 0;
	return 1;
}

//Copy characters from one string to another until end
int strcopy(char *dest, const char *src, char end)
{
	int i;
	for (i=0; src[i]!=end; i++)
		dest[i]=src[i];
	return i;
}

//Copy n characters from one string to another
char *strncopy(char *dest, const char *src, size_t n)
{
	int i;
	for (i=0; i<n; i++)
		dest[i]=src[i];
	return dest;
}

//This counts set bits. Ideally we would use popcnt, but not everybody has sse4.2 yet
int count_bits(uint32_t n)
{
	unsigned int c;

	c = n - ((n >> 1) & 0x55555555);
	c = ((c >> 2) & 0x33333333) + (c & 0x33333333);
	c = ((c >> 4) + c) & 0x0F0F0F0F;
	c = ((c >> 8) + c) & 0x00FF00FF;
	c = ((c >> 16) + c) & 0x0000FFFF;

	return c;
}

void error(char *msg, int len)
{
	write(2, msg, len);
	exit(1);

	return;
}

//This is not getting implemented because it needs some help getting initialized
char *getenv(const char *env)
{
	//stub - get your env elsewhere
}

//------------------------------------------------------------------------------
//--X11 functions
//------------------------------------------------------------------------------

int x11_connect(struct x11_connection *conn, char *display, char *xauth)
{
	int srv_len;

	if (display[0]==':')
	{
		struct sockaddr_un serv_addr={0};

		//Create the socket
		conn->sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if (conn->sock < 0)
			error("Error opening socket\n",21);

		serv_addr.sun_family = AF_UNIX;
		strncopy(serv_addr.sun_path, "/tmp/.X11-unix/X", 16);
		strcopy(&serv_addr.sun_path[16], &display[1],0);
		srv_len = sizeof(struct sockaddr_un);

		connect(conn->sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
	}
	else
	{
		struct sockaddr_in serv_addr={0};

		//Create the socket
		conn->sock = socket(AF_INET, SOCK_STREAM, 0);
		if (conn->sock < 0)
			error("Error opening socket\n",21);

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr=0x0100007f; //127.0.0.1 in network byte order (and hex)
		serv_addr.sin_port = htons(6000);

		connect(conn->sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
	}

	return conn->sock;
}

int x11_read_xauthority(int fd, struct x11_conn_req *req)
{
}

int x11_handshake(struct x11_connection *conn)
{
	struct x11_conn_req req = {0};
	req.order='l';	//Little endian
	req.major=11; req.minor=0; //Version 11.0
	write(conn->sock,&req,sizeof(struct x11_conn_req)); //Send request

	read(conn->sock,&conn->header,sizeof(struct x11_conn_reply)); //Read reply header

	if (conn->header.success==0)
		return conn->header.success;

	conn->setup = sbrk(conn->header.length*4);	//Allocate memory for remainder of data
	read(conn->sock,conn->setup,conn->header.length*4);	//Read remainder of data
	void* p = ((void*)conn->setup)+sizeof(struct x11_conn_setup)+conn->setup->vendor_length;	//Ignore the vendor
	conn->format = p;	//Align struct with format sections
	p += sizeof(struct x11_pixmap_format)*conn->setup->formats;	//move pointer to end of section
	conn->root = p;	//Align struct with root section(s)
	p += sizeof(struct x11_root_window)*conn->setup->roots;	//move pointer to end of section
	conn->depth = p; //Align depth struct with first depth section
	p += sizeof(struct x11_depth);	//move pointer to end of section
	conn->visual = p; //Align visual with first visual for first depth

	return conn->header.success;
	//Note that if the server sends a "request for authorization", this
	// seems to have the success flag set
//	printf("resIDs: %x & %x\nroots: %d\nroot_id: 0x%x\ndepths: %d\n",conn->setup->id_base,conn->setup->id_mask,conn->setup->roots,conn->root[0].id,conn->root[0].depths);
}

uint32_t x11_generate_id(struct x11_connection *conn)
{
	static uint32_t id = 0;
	return (id++ | conn->setup->id_base);
}


#define X11_FLAG_GC_FUNC 0x00000001
#define X11_FLAG_GC_PLANE 0x00000002
#define X11_FLAG_GC_BG 0x00000004
#define X11_FLAG_GC_FG 0x00000008
#define X11_FLAG_GC_LINE_WIDTH 0x00000010
#define X11_FLAG_GC_LINE_STYLE 0x00000020
#define X11_FLAG_GC_FONT 0x00004000
#define X11_FLAG_GC_EXPOSE 0x00010000
void x11_create_gc(struct x11_connection *conn, uint32_t id, uint32_t target, uint32_t flags, uint32_t *list)
{
	uint16_t flag_count = count_bits(flags);

	uint16_t length = 4 + flag_count;
	uint32_t *packet = sbrk(length*4);

	packet[0]=X11_OP_REQ_CREATE_GC | length<<16;
	packet[1]=id;
	packet[2]=target;
	packet[3]=flags;
	int i;
	for (i=0;i<flag_count;i++)
		packet[4+i] = list[i];

	write(conn->sock,packet,length*4);

	sbrk(-(length*4));

	return;
}

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
					uint32_t flags, uint32_t *list)
{
	uint16_t flag_count = count_bits(flags);

	uint16_t length = 8 + flag_count;
	uint32_t *packet = sbrk(length*4);

	packet[0]=X11_OP_REQ_CREATE_WINDOW | length<<16;
	packet[1]=id;
	packet[2]=parent;
	packet[3]=x | y<<16;
	packet[4]=w | h<<16;
	packet[5]=border<<16 | group;
	packet[6]=visual;
	packet[7]=flags;
	int i;
	for (i=0;i<flag_count;i++)
		packet[8+i] = list[i];

	write(conn->sock,packet,length*4);

	sbrk(-(length*4));

	return;
}

void x11_map_window(struct x11_connection *conn, uint32_t id)
{
	uint32_t packet[2];
	packet[0]=X11_OP_REQ_MAP_WINDOW | 2<<16;
	packet[1]=id;
	write(conn->sock,packet,8);

	return;
}

void x11_unmap_window(struct x11_connection *conn, uint32_t id)
{
	uint32_t packet[2];
	packet[0]=X11_OP_REQ_UNMAP_WINDOW | 2<<16;
	packet[1]=id;
	write(conn->sock,packet,8);

	return;
}

#define X11_IMG_FORMAT_MONO 0x00
#define X11_IMG_FORMAT_XY 0x01
#define X11_IMG_FORMAT_Z 0x02
void x11_put_img(struct x11_connection *conn, uint8_t format, uint32_t target, uint32_t gc,
				uint16_t w, uint16_t h, uint16_t x, uint16_t y, uint8_t depth, void *data)
{
	uint32_t packet[6];
	uint16_t length = ((w * h)) + 6;

	packet[0]=X11_OP_REQ_PUT_IMG | format<<8 | length<<16;
	packet[1]=target;
	packet[2]=gc;
	packet[3]=w | h<<16;
	packet[4]=x | y<<16;
	packet[5]=depth<<8;

	write(conn->sock,packet,24);
	write(conn->sock,data,(w*h)*4);

	return;
}

void x11_create_pix(struct x11_connection *conn, uint32_t pix, uint32_t target,
					uint16_t w, uint16_t h, uint8_t depth)
{
	uint32_t packet[4];
	uint16_t length = 4;

	packet[0]=X11_OP_REQ_CREATE_PIX | depth<<8 | length<<16;
	packet[1]=pix;
	packet[2]=target;
	packet[3]=w | h<<16;

	write(conn->sock,packet,16);

	return;
}

void x11_copy_area(struct x11_connection *conn, uint32_t source, uint32_t target, uint32_t gc,
					uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t w, uint16_t h)
{
	uint32_t packet[7];
	uint16_t length = 7;

	packet[0]=X11_OP_REQ_COPY_AREA | length<<16;
	packet[1]=source;
	packet[2]=target;
	packet[3]=gc;
	packet[4]=x1 | y1<<16;
	packet[5]=x2 | y2<<16;
	packet[6]=w | h<<16;

	write(conn->sock,packet,28);

	return;
}

void x11_grab_key(struct x11_connection *conn, uint8_t key, uint16_t modifiers, uint32_t win, uint8_t owner, uint8_t sync_key, uint8_t sync_mouse)
{
	uint32_t packet[4];
	uint16_t length = 4;

	packet[0]=X11_OP_REQ_GRAB_KEY | owner<<8 | length<<16;
	packet[1]=win;
	packet[2]=modifiers | key<<16 | sync_mouse;	//Error here
	packet[3]=sync_key;

	write(conn->sock,packet,length*4);
}

void x11_ungrab_kb(struct x11_connection *conn, uint32_t time)
{
	uint32_t packet[2];
	uint16_t length = 2;

	packet[0]=X11_OP_REQ_UNGRAB_KB | length<<16;
	packet[1]=time;

	write(conn->sock,packet,length*4);
}

void x11_warp(struct x11_connection *conn, uint32_t src, uint32_t dest, uint16_t src_x, uint16_t src_y, uint16_t w, uint16_t h, uint16_t x, uint16_t y)
{
	uint32_t packet[6];
	uint16_t length = 6;

	packet[0]=X11_OP_REQ_WARP | length<<16;
	packet[1]=src;
	packet[2]=dest;
	packet[3]=src_x | src_y<<16;
	packet[4]=w | h<<16;
	packet[5]=x | y<<16;

	write(conn->sock,packet,length*4);
}

uint8_t x11_get_ext(struct x11_connection *conn, uint16_t len, uint8_t *name)
{
	uint32_t packet[2];
	uint16_t length = 2 + ((len+(4-(len%4))) / 4);

	packet[0]=X11_OP_REQ_GET_EXT | length<<16;
	packet[1]=len;

	write(conn->sock,packet,8);
	write(conn->sock,name,len+(4-(len%4)));

	uint8_t response[32];

	read(conn->sock,response,32);

	if (response[0]!=1)
		return 0;
	if (response[8]!=1)
		return 0;
	return response[9];
}

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
					uint32_t target, uint16_t x, uint16_t y, uint8_t n, void *rects)
{
	uint32_t packet[4];
	uint16_t length = 4 + (n*2);

	packet[0]=base | X11_OP_SHAPE_RECT<<8 | length<<16;
	packet[1]=op | type<<8 | order<<16;
	packet[2]=target;
	packet[3]=x | y<<16;
//	packet[4]=w | h<<16;
//	packet[5]=x | y<<16;

	write(conn->sock,packet,16);
	write(conn->sock,rects,n*8);
}

//------------------------------------------------------------------------------
//--Application functions
//------------------------------------------------------------------------------

struct term_window
{
	int win,gc;
	int font_pix;
	struct psf_font font;
	int tty;
	int tty_stdin, tty_stdout, stderr;
	int cursor_x,cursor_y;

	struct termios settings;
};

int open_tty(struct term_window *term)
{
	int slave;

	term->tty = open("/dev/ptmx",O_RDWR,0);

	ioctl(term->tty,TCGETS,&term->settings);

	ioctl(term->tty,TIOCGPTN,&slave);

	int unlock=0;
	ioctl(term->tty,TIOCSPTLCK,&unlock);

	char device[] = "/dev/pts/   ";

	if (slave<10) 
		itostr(&device[9],1,slave);
	else if (slave<100)
		itostr(&device[9],2,slave);
	else
		itostr(&device[9],3,slave);

//	write(2,&device,13);

	return open(device,O_RDWR,0);
}

void login_tty(int fd)
{
	// Get a new session
	setsid();

	// Set the controlling terminal
	ioctl(fd, TIOCSCTTY, NULL);

	// Close stdin,out, and err. This isn't supposed to be needed, but it was the only way it would work for me...
	close(0);
	close(1);
	close(2);

	// Duplicate terminal over stdin, out, and err
	dup2(fd,0);
	dup2(fd,1);
	dup2(fd,2);

	// Close the 
	close(fd);
}

void shell(int tty)
{
	char *args[] = {"/bin/bash",NULL,NULL};
	login_tty(tty);
	execve("/bin/bash",args,NULL);
}

void display_char(struct x11_connection *conn, struct term_window *term, char c, int x, int y)
{
	x11_copy_area(conn, term->font_pix, term->win, term->gc, c*psf_get_glyph_width(&term->font), 0, x, y, psf_get_glyph_width(&term->font), psf_get_glyph_height(&term->font));
}

void display_str(struct x11_connection *conn, struct term_window *term, char *str, int x, int y)
{
	int i=0;

	for (i=0;str[i]!=0;i++)
		display_char(conn,term,str[i],x+(psf_get_glyph_width(&term->font)*i),y);
}

void draw_text(struct x11_connection *conn, struct term_window *term, char *buffer, int len)
{
	display_str(conn,term,buff,term->cursor_x*psf_get_glyph_width(&term->font),term->cursor_y*psf_get_glyph_height(&term->font));
	term.cursor_x+=n;
}

void draw_buffer()
{
	
}

void config_read()
{
}

int main(int argc, char **argv)
{
	//This is the most best way I could think to get the env
	char **env = &argv[argc+1];

	//Here, let's deal with command line args and envs
	int i;
	for (i=0;i<argc;i++)
	{
		if (argv[i][0]=='-')
		{
			if (argv[i][1]=='f')
			{
				//user is specifying an alternate font
			}
			if (argv[i][1]=='d')
			{
				//user is specifying an alternate display
			}
			if (argv[i][1]=='s')
			{
				//user is specifying an alternate shell
			}
		}
	}

//	int i;
//	for (i=0;env[i]!=0;i++)
//	{
//		syscall3(SYS_write,1,env[i],4);
//		syscall3(SYS_write,1,"\n",1);
//	}

	//
	struct x11_connection conn = {0};

	x11_connect(&conn,":0", NULL);

	x11_handshake(&conn);

	struct term_window term;

	term.gc = x11_generate_id(&conn);
	uint32_t val[32]; //32? why so many?
	val[0]=0x00aa00ff;
	val[1]=0x00000000;
	x11_create_gc(&conn,term.gc,conn.root[0].id,X11_FLAG_GC_BG|X11_FLAG_GC_EXPOSE,val);

	term.win = x11_generate_id(&conn);
	val[0]=0x005500bb;
	val[1]=X11_EV_MASK_KEY_DOWN | X11_EV_MASK_KEY_UP;
//	x11_create_win(&conn,win,conn.root[0].id,200,200,conn.root[0].width,conn.root[0].height,1,1,conn.root[0].root_visual_id,X11_FLAG_WIN_BG_COLOR|X11_FLAG_WIN_EVENT,val);
	x11_create_win(&conn,term.win,conn.root[0].id,200,200,conn.root[0].width/1.5,conn.root[0].height/1.5,1,1,conn.root[0].root_visual_id,X11_FLAG_WIN_BG_COLOR|X11_FLAG_WIN_EVENT,val);

	x11_map_window(&conn,term.win);


//	//This is where we open our PSF font
//	struct psf_font font;

	psf_open_font(&term.font, "font.psf");
	psf_read_header(&term.font);

	//Now we create a buffer to store the data, we need to use the same
	//pixel format as our window
	uint8_t *data = sbrk(psf_get_glyph_size(&term.font)*(conn.root[0].depth));

	term.font_pix = x11_generate_id(&conn);
	x11_create_pix(&conn,term.font_pix,conn.root[0].id,psf_get_glyph_width(&term.font)*psf_get_glyph_total(&term.font),psf_get_glyph_height(&term.font),conn.root[0].depth);

	//iterate through all glyphs in file
	for (i=0;i<psf_get_glyph_total(&term.font);i++)
	{
		//read next glyph
		psf_read_glyph(&term.font,data,4,0x00000000,0xffffffff);

		//copy glyph data to pix
		x11_put_img(&conn,X11_IMG_FORMAT_Z,term.font_pix,term.gc,psf_get_glyph_width(&term.font),psf_get_glyph_height(&term.font),psf_get_glyph_width(&term.font)*i,0,conn.root[0].depth,data);
	}

	//Free up the memory
	brk(data);


	int tty = open_tty(&term);

	int pid = fork();

	if (!pid)
	{
		shell(tty);
	}

	term.cursor_x=0;
	term.cursor_y=0;

	struct pollfd poll_fd[2] = {{conn.sock,POLLIN|POLLRDHUP|POLLPRI,0},{term.tty,POLLIN|POLLRDHUP|POLLPRI,0}};
	while (1)
	{
		union x11_event ev;
		poll(poll_fd,2,-1);

		if (poll_fd[0].revents&(POLLIN|POLLPRI))
		{
			read(conn.sock, &ev, sizeof(ev));
			if (ev.key.code==X11_EV_KEY_DOWN)
			{
				//Send the typed letter to the client


				//print the letter typed
//				display_char(&conn,win,gc,ev.key.detail,font_pix,&font,10,10);
//				display_str(&conn,&term,"Testing",10,10);

//				write(0,&ev.key.detail,1);
//				printf("%x\n",ev.key.detail);
			}
			else if (ev.key.code==X11_EV_KEY_UP)
			{
				write(term.tty,"ls\n",3);
				term.cursor_x=0;
				term.cursor_y++;
//				uint16_t *tmp=&key_coord[ev.key.detail];
//				x11_warp(&conn,conn.root[0].id,conn.root[0].id,0,0,0,0,tmp[0]*25,tmp[1]*100);
//				x11_shape_rect(&conn, shape_base, X11_SHAPE_OP_SUB, X11_SHAPE_TYPE_BOUND, 0, win, tmp[0]*25, tmp[1]*50, 1, &rect);
//				x11_unmap_window(&conn,win);
			}
		}

		// Connection closed, maybe somebody pressed the X button
		if (poll_fd[0].revents&POLLRDHUP)
		{
			exit(0);
		}

		//This means that the shell has something to say
		if (poll_fd[1].revents&(POLLIN|POLLPRI))
		{
			char buff[32];
			int n = read(term.tty,buff,32);
			//This needs to be a huge big function which takes the whole buffer and draws it on the screen.
			//Maybe we can just deal with stuff as it comes in, but we still need to redraw the whole thing
			//sometimes.
			display_str(&conn,&term,buff,term.cursor_x*psf_get_glyph_width(&term.font),term.cursor_y*psf_get_glyph_height(&term.font));
			term.cursor_x+=n;
//			write(1,buff,32);
		}
	}
}
