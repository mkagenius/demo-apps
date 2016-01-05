#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <ft2build.h>
#include <libswscale/swscale.h>
#include FT_FREETYPE_H
#include "thnets.h"
#include "libgl.h"
#include "videocap.h"

#define NBUFFERS 4
static void *vcap;
static int frame_width, frame_height, win_width, win_height;
static char *frames[NBUFFERS];
static int curframe;

#define MAXITEMS 100
static int nitems = 0;
typedef struct {
	int x, y, w, h;
} RECT;
struct ITEM {
	int type;
	RECT rect;
	int width, color;
	char text[100];
	uint8_t *bitmap;
};
static struct ITEM items[MAXITEMS];
static FT_Library ft_lib;
static FT_Face ft_face;

static int yuyv2rgb(unsigned char *src, int srcw, int srch, int src_stride, unsigned char *rgb, int dstw, int dsth)
{
	struct SwsContext *sws_ctx;
	const uint8_t *srcslice[3];
	uint8_t *dstslice[3];
	int srcstride[3], dststride[3];

	srcslice[0] = src;
	srcstride[0] = src_stride;
	dstslice[0] = rgb;
	dststride[0] = 3 * dstw;

	sws_ctx = sws_getContext(srcw, srch, AV_PIX_FMT_YUYV422, dstw, dsth, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, 0, 0, 0);
	sws_scale(sws_ctx, srcslice, srcstride, 0, srch, dstslice, dststride);
	sws_freeContext(sws_ctx);
	return 0;
}

static void PrepareText(struct ITEM *item)
{
	int i, j, k, stride, bwidth, bheight, asc, desc;
	const char *s = item->text;
	int size = item->width;
	unsigned color = item->color;
	unsigned char alpha = (color >> 24);
	unsigned char red = (color >> 16);
	unsigned char green = (color >> 8);
	unsigned char blue = color;
	FT_Set_Char_Size(ft_face, 0, item->width * 64, 0, 0 );
	int scale = ft_face->size->metrics.y_scale;
	asc = FT_MulFix(ft_face->ascender, scale) / 64;
	desc = -FT_MulFix(ft_face->descender, scale) / 64;
	bheight = asc + desc + 1;
	bwidth = 2 * size * strlen(s);
	unsigned char *bitmap = (unsigned char *)calloc(bheight * bwidth, 4);
	stride = strlen(s) * size * 2 * 4;
	FT_Set_Char_Size(ft_face, 0, 64 * size, 0, 0);
	unsigned char *pbitmap = bitmap;
	for(i = 0; s[i]; i++)
	{
		FT_Load_Char(ft_face, s[i], FT_LOAD_RENDER);
		FT_Bitmap *bmp = &ft_face->glyph->bitmap;
		for(j = 0; j < bmp->rows; j++)
			for(k = 0; k < bmp->width; k++)
			{
				pbitmap[(j + asc - ft_face->glyph->bitmap_top) * stride + 4 * (k + ft_face->glyph->bitmap_left) + 0] = red;
				pbitmap[(j + asc - ft_face->glyph->bitmap_top) * stride + 4 * (k + ft_face->glyph->bitmap_left) + 1] = green;
				pbitmap[(j + asc - ft_face->glyph->bitmap_top) * stride + 4 * (k + ft_face->glyph->bitmap_left) + 2] = blue;
				pbitmap[(j + asc - ft_face->glyph->bitmap_top) * stride + 4 * (k + ft_face->glyph->bitmap_left) + 3] =
					bmp->buffer[j * bmp->pitch + k] * alpha / 255;
			}
		pbitmap += 4 * (ft_face->glyph->advance.x / 64);
	}
	//bwidth = (pbitmap - bitmap) / 4;
	item->rect.w = bwidth;
	item->rect.h = bheight;
	if(item->bitmap)
		free(item->bitmap);
	item->bitmap = bitmap;
	item->type = 3;
}

static void DrawItem(struct ITEM *item)
{
	unsigned color;

	switch(item->type)
	{
	case 1:
		color = (item->color & 0xff00ff00) | ((item->color & 0xff0000)>>16) | ((item->color & 0xff)<<16);
		Blt(&color, 0, 1, 1, item->rect.x, item->rect.y, item->rect.w, item->width);
		Blt(&color, 0, 1, 1, item->rect.x, item->rect.y+item->width, item->width, item->rect.h - 2 * item->width);
		Blt(&color, 0, 1, 1, item->rect.x + item->rect.w - item->width, item->rect.y + item->width, item->width, item->rect.h - 2 * item->width);
		Blt(&color, 0, 1, 1, item->rect.x, item->rect.y + item->rect.h - item->width, item->rect.w, item->width);
		break;
	case 2:
		PrepareText(item);
		if(item->type != 3)
			break;
	case 3:
		Blt(item->bitmap, 0, item->rect.w, item->rect.h, item->rect.x, item->rect.y, item->rect.w, item->rect.h);
		break;
	}
}

static void loadfont()
{
	if(FT_Init_FreeType(&ft_lib))
	{
		fprintf(stderr, "Error initializing the FreeType library\n");
		return;
	}
	if(FT_New_Face(ft_lib, "/usr/share/fonts/truetype/freefont/FreeSans.ttf", 0, &ft_face))
	{
		fprintf(stderr, "Error loading FreeSans font\n");
		return;
	}
}

static double seconds()
{
	static double base;
	struct timeval tv;

	gettimeofday(&tv, 0);
	if(!base)
		base = tv.tv_sec + tv.tv_usec * 1e-6;
	return tv.tv_sec + tv.tv_usec * 1e-6 - base;
}

int rectangle(int x, int y, int w, int h, int width, int color)
{
	if(nitems == MAXITEMS)
		return -1;
	items[nitems].type = 1;
	items[nitems].rect.x = x;
	items[nitems].rect.y = y;
	items[nitems].rect.w = w;
	items[nitems].rect.h = h;
	items[nitems].width = width;
	items[nitems].color = color | 0xff000000;
	nitems++;
	return 0;
}

static int text(int x, int y, const char *text, int width, int color)
{
	if(!ft_face)
		return -1;
	if(nitems == MAXITEMS)
		return -1;
	items[nitems].type = 2;
	items[nitems].rect.x = x;
	items[nitems].rect.y = y;
	strncpy(items[nitems].text, text, sizeof(items[nitems].text)-1);
	items[nitems].width = width;
	items[nitems].color = color | 0xff000000;
	nitems++;
	return 0;
}


static void *rendering_thread(void *dummy)
{
	struct timeval tv;
	int i;

	StartWindow();
	for(;;)
	{
		int rc;
		int fn = 0;

		rc = videocap_getframe(vcap, &frames[fn], &tv);
		if(rc < 0)
			break;
		curframe = fn;
		fn = (fn + 1) % NBUFFERS;
		Blt(frames[curframe], 1, frame_width, frame_height, 0, 0, win_width, win_height);
		for(i = 0; i < nitems; i++)
			DrawItem(items+i);
		Present();
	}
	fprintf(stderr, "Failed to capture from camera\n");
	return 0;
}

struct catp {
	float p;
	char *cat;
};

int catpcmp(const void *a, const void *b)
{
	return (((struct catp *)b)->p - ((struct catp *)a)->p) * 1e8;
}

char **categories;
int ncat;

int loadcategories(const char *modelsdir)
{
	char path[200], s[200], *p;
	FILE *fp;
	
	sprintf(path, "%s/categories.txt", modelsdir);
	fp = fopen(path, "r");
	if(!fp)
		THError("Cannot load %s", path);
	ncat = 0;
	if(fgets(s, sizeof(s), fp))
	while(fgets(s, sizeof(s), fp))
	{
		p = strchr(s, ',');
		if(!p)
			continue;
		ncat++;
	}
	rewind(fp);
	categories = calloc(ncat, sizeof(*categories));
	ncat = 0;
	if(fgets(s, sizeof(s), fp))
	while(fgets(s, sizeof(s), fp))
	{
		p = strchr(s, ',');
		if(!p)
			continue;
		*p = 0;
		categories[ncat++] = strdup(s);
	}
	fclose(fp);
	return 0;
}

int main(int argc, char **argv)
{
	THNETWORK *net;
	int rc, alg = 1, i, fullscreen = 0;
	char camera[6] = "cam0";
	const char *modelsdir = 0, *inputfile = camera;
	const int eye = 231;

	frame_width = 640;
	frame_height = 480;
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-')
			continue;
		switch(argv[i][1])
		{
		case 'm':
			if(i+1 < argc)
				modelsdir = argv[++i];
			break;
		case 'i':
			if(i+1 < argc)
				inputfile = argv[++i];
			break;
		case 'a':
			if(i+1 < argc)
				alg = atoi(argv[++i]);
			break;
		case 'f':
			fullscreen = 1;
			break;
		case 'r':
			if(i+1 < argc)
			{
				i++;
				if(!strcasecmp(argv[i], "QVGA"))
				{
					frame_width = 320;
					frame_height = 240;
				} else if(!strcasecmp(argv[i], "HD"))
				{
					frame_width = 1280;
					frame_height = 720;
				} else if(!strcasecmp(argv[i], "FHD"))
				{
					frame_width = 1920;
					frame_height = 1080;
				}
			}
			break;
		}
	}
	if(!modelsdir || !inputfile)
	{
		fprintf(stderr, "Syntax: thnetsdemo -m <models directory> [-i <input file (default cam0)>]\n");
		fprintf(stderr, "                   [-a <alg=0:norm,1:MM,default,2:cuDNN,3:cudNNhalf>]\n");
		fprintf(stderr, "                   [-r <QVGA,VGA (default),HD,FHD>\n");
		return -1;
	}
	if(alg == 3)
	{
		alg = 2;
		THCudaHalfFloat(1);
	}
	loadfont();
	THInit();
	net = THLoadNetwork(modelsdir);
	loadcategories(modelsdir);
	if(net)
	{
		THMakeSpatial(net);
		if(alg == 0)
			THUseSpatialConvolutionMM(net, 0);
		else if(alg == 2)
		{
			THNETWORK *net2 = THCreateCudaNetwork(net);
			if(!net2)
				THError("CUDA not compiled in");
			THFreeNetwork(net);
			net = net2;
		}
		char videodev[20] = "/dev/video";
		if(!memcmp(inputfile, "cam", 3))
			strcpy(videodev+10, inputfile+3);
		else THError("Only cams supported for now");
		vcap = videocap_open(videodev);
		if(!vcap)
			THError("Error opening camera");
		rc = videocap_startcapture(vcap, frame_width, frame_height, V4L2_PIX_FMT_YUYV, 10, NBUFFERS);
		if(rc)
			THError("startcapture error %d", rc);
		win_width = frame_width;
		win_height = frame_height;
		if(fullscreen)
			win_width = win_height = 0;
		rc = CreateWindow("thnetsdemo", 0, 0, win_width, win_height);
		if(rc)
			THError("CreateWindow failed");
		GetWindowSize(&win_width, &win_height);
		pthread_t tid;
		pthread_create(&tid, 0, rendering_thread, 0);
		pthread_detach(tid);
		unsigned char *rgb = malloc(frame_width * frame_height * 3);
		int offset = (frame_width - frame_height) / 2 * 2;
		struct catp *res = malloc(sizeof(*res) * ncat);
		while(!frames[curframe])
			usleep(10000);
		for(;;)
		{
			float *result;
			double t;
			int outwidth, outheight, n;
			char s[300];

			t = seconds();
			yuyv2rgb((unsigned char *)frames[curframe] + offset, frame_height, frame_height, frame_width * 2, (unsigned char *)rgb, eye, eye);
			n = THProcessImages(net, &rgb, 1, eye, eye, 3*eye, &result, &outwidth, &outheight);
			if(n / outwidth != ncat)
				THError("Bug: wrong number of outputs received: %d != %d", n / outwidth, ncat);
			if(outheight != 1)
				THError("Bug: outheight expected 1");
			t = 1.0 / (seconds() - t);
			for(i = 0; i < ncat; i++)
			{
				res[i].p = result[i];
				res[i].cat = categories[i];
			}
			qsort(res, ncat, sizeof(*res), catpcmp);
			nitems = 0;
			sprintf(s, "%.2f fps", t);
			text(10, 10, s, 16, 0xff0000);
			for(i = 0; i < 5; i++)
			{
				text(10, 40 + i * 20, res[i].cat, 16, 0x00a000);
				sprintf(s, "(%.0f %%)", res[i].p * 100);
				text(100, 40 + i * 20, s, 16, 0x00a000);
			}
		}
	} else printf("The network could not be loaded: %d\n", THLastError());
	return 0;
}
