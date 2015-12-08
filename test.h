#ifndef TEST_NNS_H
#define TEST_NNS_H

#include <stdio.h>
#include "vmtype.h"
#include "vmsystem.h"
#include "vmcmd.h"
#include "vmfs.h"
#include "vmchset.h"
#include "vmaudio_record.h"
#include "vmaudio_play.h"
#include "vmlog.h"

#include "record.h"
#include "wav_adpcm.h"
#include "log.h"

#define OUT_FILE "result_nnu_pca.txt"
#define NNU 1
#define REPEAT 10
#include "nnu_float.h"

//#define OUT_FILE "result_nnu_pca.txt"
//#define NNU 1
//#define REPEAT 10
//#include "nnu_pca_float.h"

//#define OUT_FILE "result_nns.txt"
//#define REPEAT 3
//#include "nns_float.h"

#define VOICE_FILES_LENGTH 1
char *voicefiles[VOICE_FILES_LENGTH] = {
	"forward.wav"
};

#define NUM_SETTINGS 15
int alphas[15] = {1, 2, 3, 4, 5, 5, 5, 5, 5, 10, 10, 10, 15, 20, 20};
int betas[15] = {1, 1, 1, 1, 1, 2, 3, 4, 5, 5, 7, 10, 10, 10, 12};
int Ns[15] = {1, 2, 3, 4, 5, 10, 15, 20, 25, 50, 70, 100, 150, 200, 240};

VMCHAR buff[200];

VMUINT32 run_test_file(char *fn, int *ret, VMUINT *unix_start, VMUINT *unix_end){
	VMINT drv = vm_fs_get_internal_drive_letter();
	VMCHAR file_name[200] = {0};
	VMWCHAR w_file_name[200] = {0};
	sprintf(file_name, "%c:\\%s", drv, fn);
	vm_chset_ascii_to_ucs2(w_file_name, 100, file_name);

	VM_FS_HANDLE fs_handle = vm_fs_open(w_file_name, VM_FS_MODE_READ, VM_TRUE);
	VMUINT bytes;
	WavHeader header;
	vm_fs_read(fs_handle, &header, sizeof(WavHeader), &bytes);

	sprintf(buff, "chunk_id = %s", header.chunk_id);
	log_info(7, buff);
	sprintf(buff, "chunk_size = %d", header.chunk_size);
	log_info(8, buff);
	sprintf(buff, "factchunk_size = %d", header.samplesperblock);
	log_info(9, buff);
	sprintf(buff, "dwSampleLength = %d", header.dwSampleLength);
	log_info(10, buff);

	IMA_ADPCM_PRIVATE *pima = ima_reader_init(fs_handle, &header);
	int chunk_size = header.samplesperblock;
	int num_chunk = header.dwSampleLength / chunk_size;
	float chunk[chunk_size];
	int i;
	vm_time_get_unix_time(unix_start);
	VM_TIME_UST_COUNT start = vm_time_ust_get_count();
	nnu_start();
	for (i = 0; i < num_chunk; ++i) {
		ima_read_f (fs_handle, pima, chunk, chunk_size);
		nnu_stream(chunk, chunk_size);
	}
	*ret = nnu_classify();
	VM_TIME_UST_COUNT end = vm_time_ust_get_count();
	VMUINT32 diff = vm_time_ust_get_duration(start,end);
	vm_time_get_unix_time(unix_end);

	sprintf(buff, "diff = %d", diff);
	log_info(11, buff);

	vm_fs_close(fs_handle);
	vm_free(pima);
	return diff;
}

void run_test_suite(){
	VMCHAR file_name[200] = {0};
	VMWCHAR w_file_name[100] = {0};
	VMCHAR towrite[100] = {0};
	VMUINT bytes;
	VMINT drv = vm_fs_get_internal_drive_letter();
	sprintf(file_name, "%c:\\result\\%s", drv, OUT_FILE);
	vm_chset_ascii_to_ucs2(w_file_name, 100, file_name);
	VM_FS_HANDLE fs_handle = vm_fs_open(w_file_name, VM_FS_MODE_CREATE_ALWAYS_WRITE, VM_FALSE);
	sprintf(buff, "%d", fs_handle);
	log_info(4, buff);
#ifdef NNU
	sprintf(towrite, "File,Alpha,Beta,Time,Result,Start,End\r\n");
#else
	sprintf(towrite, "File,N,Time,Result,Start,End\r\n");
#endif
	vm_fs_write(fs_handle, towrite, strlen(towrite), &bytes);
	vm_fs_flush(fs_handle);
	int i,j;
	for (i = 0; i < VOICE_FILES_LENGTH; ++i) {

		for (j = 0; j < NUM_SETTINGS; ++j) {
#ifdef NNU
			PIPELINE.nnu->alpha = alphas[j];
			PIPELINE.nnu->beta = betas[j];
#else
			PIPELINE.nnu->D_cols = Ns[j];
#endif
			int ret;
			int k;
			VMUINT32 sum = 0;
			VMUINT unix_start;
			VMUINT unix_end;
			VMUINT32 diff;
			for (k = 0; k < REPEAT; ++k) {
				diff = run_test_file(voicefiles[i], &ret, &unix_start, &unix_end);
				sum+=diff;
			}
			diff = (float)sum/REPEAT;

#ifdef NNU
			sprintf(buff, "%d,%d,%d,%d,%d,%d", i, j, alphas[j], betas[j], ret, unix_start);
#else
			sprintf(buff, "%d,%d,%d,%d,%d", i, j, Ns[j], ret, unix_start);
#endif
			log_info(3, buff);

			sprintf(buff, "%d,%d", diff, unix_end);
			log_info(4, buff);

#ifdef NNU
			sprintf(towrite, "%s,%d,%d,%d,%d,%d,%d\r\n", voicefiles[i], alphas[j], betas[j], diff, ret, unix_start, unix_end);
#else
			sprintf(towrite, "%s,%d,%d,%d,%d,%d\r\n", voicefiles[i], Ns[j], diff, ret, unix_start, unix_end);
#endif
			vm_fs_write(fs_handle, towrite, strlen(towrite), &bytes);
			vm_fs_flush(fs_handle);
		}
	}
	vm_fs_close(fs_handle);
	return;
}

VMUINT32 run_test(int *ret){

	PIPELINE.nnu->alpha = 1;
	PIPELINE.nnu->beta = 1;
	VMUINT32 diff;
	int i,k;
	VMUINT32 sum = 0;
	for (k = 0; k < REPEAT; ++k) {

	VMINT drv = vm_fs_get_internal_drive_letter();
	VMCHAR file_name[100] = {0};
	VMWCHAR w_file_name[100] = {0};
	sprintf(file_name, "%c:\\%s", drv, "forward.wav");
	vm_chset_ascii_to_ucs2(w_file_name, 100, file_name);
	VM_FS_HANDLE fs_handle = vm_fs_open(w_file_name, VM_FS_MODE_READ, VM_TRUE);
	VMUINT bytes;
	WavHeader header;
	vm_fs_read(fs_handle, &header, sizeof(WavHeader), &bytes);

	sprintf(buff, "chunk_id = %s", header.chunk_id);
	log_info(7, buff);
	sprintf(buff, "chunk_size = %d", header.chunk_size);
	log_info(8, buff);
	sprintf(buff, "factchunk_size = %d", header.samplesperblock);
	log_info(9, buff);
	sprintf(buff, "dwSampleLength = %d", header.dwSampleLength);
	log_info(10, buff);
//	sprintf(buff, "size = %d", header.datachunk_size);
//	log_info(8, buff);
//	sprintf(buff, "num_channels = %d", header.channels);
//	log_info(9, buff);
//	sprintf(buff, "bytes = %d", bytes);
//	log_info(10, buff);
	IMA_ADPCM_PRIVATE *pima = ima_reader_init(fs_handle, &header);

//	int chunk_size = 10;
//	int num_chunk = 1;
	int chunk_size = header.samplesperblock;
	int num_chunk = header.dwSampleLength / chunk_size;

	float chunk[chunk_size];


	VM_TIME_UST_COUNT start = vm_time_ust_get_count();
	nnu_start();
	for (i = 0; i < num_chunk; ++i) {
		ima_read_f (fs_handle, pima, chunk, chunk_size);
		nnu_stream(chunk, chunk_size);
//		sprintf(buff, "i = %d", i);
//		log_info(6, buff);
	}
	*ret = nnu_classify();
	VM_TIME_UST_COUNT end = vm_time_ust_get_count();
	diff = vm_time_ust_get_duration(start,end);
		sum+=diff;

		vm_fs_close(fs_handle);
		vm_free(pima);
	}

	sprintf(buff, "avg diff = %f", (float)sum/REPEAT);
	log_info(4, buff);

	return diff;
//	return 0;
}

#endif
