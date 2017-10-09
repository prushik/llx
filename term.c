//Low-Level X11 Terminal Emulator
//Author: Philip Rushik
//License: Beerware
//
#include "libc.h"
#include <asm/termios.h>
#include <asm/ioctl.h>
#include <asm/poll.h>
#include "llx.h"
#include "psf.h"


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
	display_str(conn,term,buffer,term->cursor_x*psf_get_glyph_width(&term->font),term->cursor_y*psf_get_glyph_height(&term->font));
	term->cursor_x+=len;
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
				write(term.tty, &ev.key.detail, 1);

				//print the letter typed
//				display_char(&conn,win,gc,ev.key.detail,font_pix,&font,10,10);
//				display_str(&conn,&term,"Testing",10,10);

//				write(0,&ev.key.detail,1);
//				printf("%x\n",ev.key.detail);
			}
			else if (ev.key.code==X11_EV_KEY_UP)
			{
//				write(term.tty,"ls\n",3);
//				term.cursor_x=0;
//				term.cursor_y++;
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
			int n = read(term.tty,buff,2);
			//This needs to be a huge big function which takes the whole buffer and draws it on the screen.
			//Maybe we can just deal with stuff as it comes in, but we still need to redraw the whole thing
			//sometimes.
			for (i=0; i<n; i++)
				if (buff[i] == '\n')
				{
					term.cursor_x=0;
					term.cursor_y++;
				}
			display_str(&conn,&term,buff,term.cursor_x*psf_get_glyph_width(&term.font),term.cursor_y*psf_get_glyph_height(&term.font));
			term.cursor_x+=n;
//			write(1,buff,32);
		}
	}
}
