#ifndef RECORD_H
#define RECORD_H

#include <stdio.h>
#include <string.h>
#include "vmtype.h"
#include "vmsystem.h"
#include "vmcmd.h"
#include "vmlog.h"
#include "vmfs.h"
#include "vmaudio_record.h"
#include "vmchset.h"
#include "vmaudio_play.h"

#define RC_COMMAND_PORT  2000 /* AT command port */
#define MAX_NAME_LEN 260 /* Max length of file name */

#define COMMAND_PORT 1000 /* AT command port */

static VMINT g_handle = -1;  /* The handle of play */
static VMINT g_interrupt_handle = 0; /* The handle of interrupt */

/* The callback function when playing. */
void audio_play_callback(VM_AUDIO_HANDLE handle, VM_AUDIO_RESULT result, void* userdata)
{
  switch (result)
  {
      case VM_AUDIO_RESULT_END_OF_FILE:
    	/* When the end of file is reached, it needs to stop and close the handle */
        vm_audio_play_stop(g_handle);
        vm_audio_play_close(g_handle);
        g_handle = -1;
        break;
      case VM_AUDIO_RESULT_INTERRUPT:
        /* The playback is terminated by another application, for example an incoming call */
        vm_audio_play_stop(g_handle);
        vm_audio_play_close(g_handle);
        g_handle = -1;
        break;
      default:
        break;
  }
}

/* Play the audio file. */
void audio_play(VMCHAR *fn)
{
  VMINT drv ;
  VMWCHAR w_file_name[MAX_NAME_LEN] = {0};
  VMCHAR file_name[MAX_NAME_LEN];
  vm_audio_play_parameters_t play_parameters;
  VM_AUDIO_VOLUME volume;

  /* get file path */
  drv = vm_fs_get_removable_drive_letter();
  if(drv <0)
  {
    drv = vm_fs_get_internal_drive_letter();
    if(drv <0)
    {
      vm_log_fatal("not find driver");
      return ;
    }
  }
  sprintf(file_name, "%c:\\%s", drv, fn);
  vm_chset_ascii_to_ucs2(w_file_name, MAX_NAME_LEN, file_name);

  /* set play parameters */
  memset(&play_parameters, 0, sizeof(vm_audio_play_parameters_t));
  play_parameters.filename = w_file_name;
  play_parameters.reserved = 0;  /* no use, set to 0 */
  play_parameters.format = VM_AUDIO_FORMAT_WAV; /* file format */
  play_parameters.output_path = VM_AUDIO_DEVICE_SPEAKER_BOTH; /* set device to output */
  play_parameters.async_mode = 0;
  play_parameters.callback = audio_play_callback;
  play_parameters.user_data = NULL;
  g_handle = vm_audio_play_open(&play_parameters);
  if(g_handle >= VM_OK)
  {
    vm_log_info("open success");
  }
  else
  {
    vm_log_error("open failed");
  }
  /* start to play */
  vm_audio_play_start(g_handle);
  /* set volume */
  vm_audio_set_volume(VM_AUDIO_VOLUME_6);
  /* register interrupt callback */
  g_interrupt_handle = vm_audio_register_interrupt_callback(audio_play_callback,NULL);
}

/* The callback function of recording. */
void audio_record_callback(VM_AUDIO_RECORD_RESULT result, void* userdata)
{
  switch (result)
  {
      case VM_AUDIO_RECORD_ERROR_NO_SPACE:
      case VM_AUDIO_RECORD_ERROR:
    	/* not enough space */
        vm_audio_record_stop();
        break;
      default:
        vm_log_info("callback result = %d", result);
        break;
  }
}

/* Record a file. */
void audio_record(VMCHAR *fn)
{
  VMINT drv ;
  VMCHAR file_name[MAX_NAME_LEN];
  VMWCHAR w_file_name[MAX_NAME_LEN];
  VMINT result;

  /* If there is a removable letter (SD Card) use it, otherwise stored it in the flash storage (vm_fs_get_internal_drive_letter).  */
  drv = vm_fs_get_removable_drive_letter();
  if(drv <0)
  {
    drv = vm_fs_get_internal_drive_letter();
    if(drv <0)
    {
      vm_log_fatal("not find driver");
      return ;
    }
  }
  sprintf(file_name, "%c:\\%s", drv, fn);
  vm_chset_ascii_to_ucs2(w_file_name, MAX_NAME_LEN, file_name);
  /* Record an AMR file and save it in the root directory */
  result = vm_audio_record_start(w_file_name ,VM_AUDIO_FORMAT_WAV, audio_record_callback, NULL);
  if(result == VM_SUCCESS)
  {
    vm_log_info("record success");
  }
  else
  {
    vm_log_error("record failed");
  }
}

#endif
