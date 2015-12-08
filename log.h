#ifndef LOG_H
#define LOG_H

#include "vmtype.h"
#include "vmlog.h"
#include "vmsystem.h"
#include "vmkeypad.h"
#include "vmgraphic.h"
#include "vmgraphic_font.h"
#include "vmdcl.h"
#include "vmdcl_pwm.h"
#include "vmdcl_gpio.h"
#include "vmdcl_kbd.h"
#include "vmchset.h"

vm_graphic_frame_t g_frame;
const vm_graphic_frame_t* g_frame_blt_group[1];

void log_init(void){
    vm_graphic_color_argb_t color;		/* use to set screen and text color */
    vm_graphic_point_t frame_position[1] = {0, 0};

    g_frame.width = 240;
    g_frame.height = 240;
    g_frame.color_format = VM_GRAPHIC_COLOR_FORMAT_16_BIT;
    g_frame.buffer = (VMUINT8* )vm_malloc_dma(g_frame.width * g_frame.height * 2);
    g_frame.buffer_length = (g_frame.width * g_frame.height * 2);
    g_frame_blt_group[0] = &g_frame;

    /* set color and draw bg*/
    color.a = 255;
    color.r = 255;
    color.g = 255;
    color.b = 255;
    vm_graphic_set_color(color);
    vm_graphic_draw_solid_rectangle(&g_frame, 0,0, 240,240);
#if defined(__HDK_LINKIT_ASSIST_2502__)
    vm_graphic_blt_frame(g_frame_blt_group, frame_position, 1);
#endif //#if defined(__HDK_LINKIT_ASSIST_2502__)
}

void log_info(VMINT line, VMSTR str){
    VMWCHAR s[260];					/* string's buffer */
    VMUINT32 size;
    vm_graphic_color_argb_t color;		/* use to set screen and text color */
    vm_graphic_point_t frame_position[1] = {0, 0};

    vm_chset_ascii_to_ucs2(s,260, str);
	/* set color and draw bg*/
    color.a = 0;
    color.r = 255;
    color.g = 255;
    color.b = 255;
    vm_graphic_set_color(color);
    vm_graphic_draw_solid_rectangle(&g_frame, 0,line*20, 240,20);

	/* set color and draw text*/
    color.a = 0;
    color.r = 0;
    color.g = 0;
    color.b = 0;
    vm_graphic_set_color(color);
    vm_graphic_set_font_size(18);
    vm_graphic_draw_text(&g_frame,0, line*20, s);

#if defined(__HDK_LINKIT_ASSIST_2502__)
	  /* flush the screen with text data */
    vm_graphic_blt_frame(g_frame_blt_group, frame_position, 1);
#endif //#if defined(__HDK_LINKIT_ASSIST_2502__)
}

#endif
