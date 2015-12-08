#ifndef VOICECMD_H
#define VOICECMD_H

#include <stdio.h>
#include "vmtype.h"
#include "vmsystem.h"
#include "vmcmd.h"
#include "vmfs.h"
#include "vmchset.h"
#include "record.h"
#include "vmaudio_record.h"
#include "vmaudio_play.h"
//#include "wav.h"

VMINT vcmd_recording = 0;

int voice_evaluate(){
//	VMINT drv = vm_fs_get_internal_drive_letter();
//	VMCHAR file_name[20] = {0};
//	VMWCHAR w_file_name[20] = {0};
//	sprintf(file_name, "%c:\\%s", drv, "eval.wav");
//	vm_chset_ascii_to_ucs2(w_file_name, 20, file_name);
//	VM_FS_HANDLE fs_handle = vm_fs_open(w_file_name, VM_FS_MODE_READ, VM_TRUE);
//	VMUINT bytes;
//	WavHeader header;
//	vm_fs_read(fs_handle, &header, sizeof(WavHeader), &bytes);
//
//	header.datachunk_size = 100;
//	int num = header.datachunk_size/2;
//	int16_t *samples = (int16_t*)vm_malloc(num*sizeof(int16_t));
//	double *buffer = (double*)vm_malloc(num*sizeof(double));
//	vm_fs_read(fs_handle, &samples, header.datachunk_size, &bytes);
//	int i;
//	for (i = 0; i < num; ++i) {
//		buffer[i] = audio_int16_to_double(samples[i]);
//	}
//	int ret = classification_pipeline(buffer, num, &pipeline);
//	vm_free(samples);
//	vm_free(buffer);
//	return ret;
	return -1;
}

int voice_record_stop_and_evaluate(){
	if(vcmd_recording == 1){
		vm_audio_record_stop();
		vcmd_recording = 0;
		return voice_evaluate();
	}
	return -1;
}

void voice_record_start(){
	if(vcmd_recording == 0){
		vcmd_recording = 1;
		VMINT drv = vm_fs_get_internal_drive_letter();
		VMCHAR file_name[20] = {0};
		VMWCHAR w_file_name[20] = {0};
		sprintf(file_name, "%c:\\%s", drv, "eval.wav");
		vm_chset_ascii_to_ucs2(w_file_name, 20, file_name);
		//Remove previous
		vm_fs_delete(w_file_name);
		//Record Audio
		audio_record("eval.wav");
	}

}

//void record_and_upload(){
//	VMINT drv = vm_fs_get_internal_drive_letter();
//	VMCHAR file_name[20] = {0};
//	VMWCHAR w_file_name[20] = {0};
//	sprintf(file_name, "%c:\\%s", drv, "eval.wav");
//	vm_chset_ascii_to_ucs2(w_file_name, 20, file_name);
//	vm_fs_delete(w_file_name);
//	//Record Audio
//	audio_record("eval.wav");
//}

#endif
