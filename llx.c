#include <stdint.h>
#include "libc.h"
#include "llx.h"

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
		strcopy(&serv_addr.sun_path[16], &display[1], 0);
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
	void* p = ((void*)conn->setup)+sizeof(struct x11_conn_setup)+conn->setup->vendor_length+PAD(conn->setup->vendor_length);	//Ignore the vendor
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
	uint16_t length = 2 + ((len+(4-(len&3))) >> 2);

	packet[0]=X11_OP_REQ_GET_EXT | length<<16;
	packet[1]=len;

	write(conn->sock,packet,8);
	write(conn->sock,name,len+(4-(len&3)));

	uint8_t response[32];

	read(conn->sock,response,32);

	if (response[0]!=1)
		return 0;
	if (response[8]!=1)
		return 0;
	return response[9];
}

//     1     101                             opcode
//     1                                     unused
//     2     2                               request length
//     1     KEYCODE                         first-keycode
//     1     m                               count
//     2                                     unused

//▶
//     1     1                               Reply
//     1     n                               keysyms-per-keycode
//     2     CARD16                          sequence number
//     4     nm                              reply length (m = count field
//                                           from the request)
//     24                                    unused
//     4nm     LISTofKEYSYM                  keysyms

uint8_t x11_get_keymap(struct x11_connection *conn, uint8_t first_key, uint8_t len, uint16_t *result)
{
	uint32_t packet[2];
	uint16_t length = 2;

	packet[0]=X11_OP_REQ_GET_KEYMAP | length << 16;
	packet[1]=first_key | len << 24;

	write(conn->sock,packet,8);

	uint8_t response[8+24+(len << 2)];

	read(conn->sock,response,32);

	read(conn->sock, result, response[1]*len);

	return response[1];
}




//------------------------------------------------------------------------------
//--Shape Extension functions
//------------------------------------------------------------------------------
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
