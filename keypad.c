/*
  This example code is in public domain.

  This example code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/* 
  This example shows how to receive keypad event and keycode from the board.

  This example use vm_keypad_register_event_callback() to register keypad handler.
  when operate the key on the board,the key event and keycode will be sended to application 
  through this handler. for LINKIT_ONE,background applications will receive key event.
  for LinkIt Assist 2502A or board has LCD,only forground application will receive key event.
  
  Just run this application,and see the catch log or monitor log or lcd.for LinkIt Assist 2502A,if K0 is pressed,
  this log is "key event=2,key code=0"
*/
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
#include "stdio.h"
#include "record.h"
#include "vmfs.h"
//#include "wav.h"

#include "test.h"
#include "voicecmd.h"

#include "log.h"

VMCHAR g_buff[40];

static VMUINT8* g_font_pool;

void key_init(void){
    VM_DCL_HANDLE kbd_handle;
    VM_DCL_HANDLE gpio_handle;
    vm_dcl_kbd_control_pin_t kbdmap;

    gpio_handle = vm_dcl_open(VM_DCL_GPIO,16);
    vm_dcl_control(gpio_handle,VM_DCL_GPIO_COMMAND_SET_MODE_1,NULL);
    vm_dcl_close(gpio_handle);

    gpio_handle = vm_dcl_open(VM_DCL_GPIO,21);
    vm_dcl_control(gpio_handle,VM_DCL_GPIO_COMMAND_SET_MODE_1,NULL);
    vm_dcl_close(gpio_handle);

    gpio_handle = vm_dcl_open(VM_DCL_GPIO,13);
    vm_dcl_control(gpio_handle,VM_DCL_GPIO_COMMAND_SET_MODE_1,NULL);
    vm_dcl_close(gpio_handle);

    gpio_handle = vm_dcl_open(VM_DCL_GPIO,19);
    vm_dcl_control(gpio_handle,VM_DCL_GPIO_COMMAND_SET_MODE_1,NULL);
    vm_dcl_close(gpio_handle);

    kbd_handle = vm_dcl_open(VM_DCL_KBD,0);
    kbdmap.col_map = 0x09;
    kbdmap.row_map =0x05;
    vm_dcl_control(kbd_handle,VM_DCL_KBD_COMMAND_CONFIG_PIN, (void *)(&kbdmap));

   // kbdmap.col_map = 0x04;
   // kbdmap.row_map =0x03;
   // vm_dcl_control(kbd_handle,VM_DCL_KBD_COMMAND_CONFIG_PIN, (void *)(&kbdmap));

    vm_dcl_close(kbd_handle);
}

#if defined(__HDK_LINKIT_ASSIST_2502__)
void lcd_backlight_level(VMUINT32 ulValue) {
    VM_DCL_HANDLE pwm_handle;
    vm_dcl_pwm_set_clock_t pwm_clock;
    vm_dcl_pwm_set_counter_threshold_t pwm_config_adv;
    vm_dcl_config_pin_mode(VM_PIN_P1, VM_DCL_PIN_MODE_PWM);
    pwm_handle = vm_dcl_open(PIN2PWM(VM_PIN_P1),vm_dcl_get_owner_id());
    vm_dcl_control(pwm_handle,VM_PWM_CMD_START,0);
    pwm_config_adv.counter = 100;
    pwm_config_adv.threshold = ulValue;
    pwm_clock.source_clock = 0;
    pwm_clock.source_clock_division =3;
    vm_dcl_control(pwm_handle,VM_PWM_CMD_SET_CLOCK,(void *)(&pwm_clock));
    vm_dcl_control(pwm_handle,VM_PWM_CMD_SET_COUNTER_AND_THRESHOLD,(void *)(&pwm_config_adv));
    vm_dcl_close(pwm_handle);
}
#endif //defined(__HDK_LINKIT_ASSIST_2502__)

static void font_init(void){
    VM_RESULT result;
    VMUINT32 pool_size;

    result = vm_graphic_get_font_pool_size(0, 0, 0, &pool_size);
    if(VM_IS_SUCCEEDED(result))
    {
    	g_font_pool = (VMUINT8* )vm_malloc(pool_size);
        if(NULL != g_font_pool)
        {
            vm_graphic_init_font_pool(g_font_pool, pool_size);
        }
        {
            vm_log_info("allocate font pool memory failed");
        }
    }
    else
    {
        vm_log_info("get font pool size failed, result:%d", result);
    }
}

void handle_sysevt(VMINT message, VMINT param) {
    switch (message){
    case VM_EVENT_CREATE:
    break;
    case VM_EVENT_PAINT:
    	 log_init();    	 
    	 log_info(0, "> RECORD");
    	 log_info(1, "Press K1 to select");
    	 log_info(2, "Press/hold K2 to run");
    break;
    case VM_EVENT_QUIT:
    break;
    }
    //rc_handle_sysevt(message,param);
}

VMINT recording = 0;
VMINT count = 0;

typedef enum{
	CMD_RECORD=0,
	CMD_EVAL,
	CMD_TEST,
	CMD_TEST_SUITE,
	CMD_TOTAL
} CMD;
char *menus[5] = {"RECORD","EVAL","TEST","TEST_SUITE"};

CMD currentCMD = CMD_RECORD;

void handleCMD(VM_KEYPAD_EVENT event, CMD cmd){
	switch (cmd) {
		case CMD_RECORD:
			if(event > 1){ // Pressed
				if(recording == 0){
					VMINT drv = vm_fs_get_internal_drive_letter();
					VMCHAR file_name[100] = {0};
					VMWCHAR w_file_name[10] = {0};
					VMCHAR fn[10] = {0};
					sprintf(fn,"voice%d.wav",count);
					sprintf(file_name, "%c:\\%s", drv, fn);
					vm_chset_ascii_to_ucs2(w_file_name, 100, file_name);
					vm_fs_delete(w_file_name);
					log_info(4, fn);
					audio_record((VMCHAR*)fn);
					recording = 1;
				}
			}else{ // Released
				vm_audio_record_stop();
				recording = 0;
				count++;
			}
			break;
		case CMD_EVAL:
			if(event > 1){ // Pressed
				voice_record_start();
			}else{
				int ret = voice_record_stop_and_evaluate();
				sprintf(g_buff, "ret = %d", ret);
				log_info(5, g_buff);
			}
			break;
		case CMD_TEST:
			if(event > 1){ // Pressed
				int ret = -1;
				VMUINT32 diff = run_test(&ret);
				sprintf(g_buff, "diff = %d", diff);
				log_info(5, g_buff);
				sprintf(g_buff, "ret = %d", ret);
				log_info(6, g_buff);

				VMINT drv = vm_fs_get_internal_drive_letter();
				sprintf(g_buff, "drv = %c", drv);
				log_info(7, g_buff);
			}
			break;
		case CMD_TEST_SUITE:
			if(event > 1){ // Pressed
				sprintf(g_buff, "Running");
				log_info(7, g_buff);
				run_test_suite();
				sprintf(g_buff, "Done");
				log_info(7, g_buff);
			}
			break;
		default:
			break;
	}
}

void showState(){
	sprintf(g_buff, "> %s", menus[currentCMD]);
	log_info(0, g_buff);
}

// CODE K1=0 K2=1 PWR=30
VMINT handle_keypad_event(VM_KEYPAD_EVENT event, VMINT code){
    if(code == 0){ // K_1
		if(event > 1){ // Pressed
			currentCMD = (currentCMD + 1) % CMD_TOTAL;
			showState();
		}
	}else if(code==1){ // K_2
		handleCMD(event,currentCMD);
	}
    // line 4 and up is available for output
//    vm_log_info("key event=%d,key code=%d",event,code); /* event value refer to VM_KEYPAD_EVENT */
//    sprintf(g_buff, "key event = %d, code=%d", event, code);
//    log_info(3, g_buff);    /* output log to LCD if have */
    return 0;
}

void vm_main(void) {
    /* register system events handler */
#if defined(__HDK_LINKIT_ASSIST_2502__)    
	  lcd_st7789s_init();
	  lcd_backlight_level(100);
#endif
    vm_pmng_register_system_event_callback(handle_sysevt);
    vm_log_info("register keypad handler");
    vm_keypad_register_event_callback(handle_keypad_event);
    font_init();
    key_init();

}
