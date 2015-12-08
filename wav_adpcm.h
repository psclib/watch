#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char     chunk_id[4];
    uint32_t chunk_size;
    char     format[4];

    char     fmtchunk_id[4];
    uint32_t fmt_size;
    uint16_t	fmtformat ;
  	uint16_t	channels ;
  	uint32_t	samplerate ;
  	uint32_t	bytespersec ;
  	uint16_t	blockalign ;
  	uint16_t	bitwidth ;
  	uint16_t	extrabytes ;
  	uint16_t	samplesperblock ;

    char     factchunk_id[4];
    uint32_t factchunk_size;

    uint32_t dwSampleLength;

    char     datachunk_id[4];
    uint32_t datachunk_size;
} WavHeader;

static int ima_indx_adjust [16] =
{	-1, -1, -1, -1,		/* +0 - +3, decrease the step size */
     2,  4,  6,  8,     /* +4 - +7, increase the step size */
    -1, -1, -1, -1,		/* -0 - -3, decrease the step size */
     2,  4,  6,  8,		/* -4 - -7, increase the step size */
} ;

static int ima_step_size [89] =
{	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
	253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
	1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
	3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
	11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767
} ;

typedef int sf_count_t;

#define	ARRAY_LEN(x)	((int) (sizeof (x) / sizeof ((x) [0])))

#define	SF_BUFFER_LEN			(2048 * 2)
short			sbuf	[SF_BUFFER_LEN / sizeof (short)] ;

static inline int
clamp_ima_step_index (int indx)
{	if (indx < 0)
		return 0 ;
	if (indx >= ARRAY_LEN (ima_step_size))
		return ARRAY_LEN (ima_step_size) - 1 ;

	return indx ;
} /* clamp_ima_step_index */

typedef struct IMA_ADPCM_PRIVATE_tag
{
	int				channels, blocksize, samplesperblock, blocks ;
	int				blockcount, samplecount ;
	int				previous [2] ;
	int				stepindx [2] ;
	unsigned char	*block ;
	short			*samples ;
#if HAVE_FLEXIBLE_ARRAY
	short			data	[] ; /* ISO C99 struct flexible array. */
#else
	short			data	[0] ; /* This is a hack and might not work. */
#endif
} IMA_ADPCM_PRIVATE ;

sf_count_t
psf_fread (void *ptr, sf_count_t bytes, sf_count_t items, int fd)
{	sf_count_t total = 0 ;
	ssize_t	count ;

	items *= bytes ;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0 ;

	while (items > 0)
	{	/* Break the read down to a sensible size. */
		count = items ;

		vm_fs_read(fd, ((void*) ptr) + total, count, &count);
		//count = vm_fs_read (fd, ((void*) ptr) + total, count) ;

		if (count == -1)
		{
			break ;
			} ;

		if (count == 0)
			break ;

		total += count ;
		items -= count ;
		} ;

	return total / bytes ;
} /* psf_fread */

static int
wav_w64_ima_decode_block (int fd, IMA_ADPCM_PRIVATE *pima)
{	int		chan, k, predictor, blockindx, indx, indxstart, diff ;
	short	step, bytecode, stepindx [2] ;

	pima->blockcount ++ ;
	pima->samplecount = 0 ;

	if (pima->blockcount > pima->blocks)
	{	memset (pima->samples, 0, pima->samplesperblock * pima->channels * sizeof (short)) ;
		return 1 ;
		} ;

  psf_fread (pima->block, 1, pima->blocksize, fd);
	// if ((k = psf_fread (pima->block, 1, pima->blocksize, psf)) != pima->blocksize)
	// 	psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, pima->blocksize) ;

	/* Read and check the block header. */

	for (chan = 0 ; chan < pima->channels ; chan++)
	{
    predictor = pima->block [chan*4] | (pima->block [chan*4+1] << 8) ;

		if (predictor & 0x8000)
			predictor -= 0x10000 ;

		stepindx [chan] = pima->block [chan*4+2] ;
		stepindx [chan] = clamp_ima_step_index (stepindx [chan]) ;


		// if (pima->block [chan*4+3] != 0)
		// 	psf_log_printf (psf, "IMA ADPCM synchronisation error.\n") ;

		pima->samples [chan] = predictor ;
	} ;

	/*
	**	Pull apart the packed 4 bit samples and store them in their
	**	correct sample positions.
	*/

	blockindx = 4 * pima->channels ;

	indxstart = pima->channels ;
	while (blockindx < pima->blocksize)
	{	for (chan = 0 ; chan < pima->channels ; chan++)
		{	indx = indxstart + chan ;
			for (k = 0 ; k < 4 ; k++)
			{	bytecode = pima->block [blockindx++] ;
				pima->samples [indx] = bytecode & 0x0F ;
				indx += pima->channels ;
				pima->samples [indx] = (bytecode >> 4) & 0x0F ;
				indx += pima->channels ;
				} ;
			} ;
		indxstart += 8 * pima->channels ;
		} ;

	/* Decode the encoded 4 bit samples. */

	for (k = pima->channels ; k < (pima->samplesperblock * pima->channels) ; k ++)
	{	chan = (pima->channels > 1) ? (k % 2) : 0 ;

		bytecode = pima->samples [k] & 0xF ;

		step = ima_step_size [stepindx [chan]] ;
		predictor = pima->samples [k - pima->channels] ;

		diff = step >> 3 ;
		if (bytecode & 1)
			diff += step >> 2 ;
		if (bytecode & 2)
			diff += step >> 1 ;
		if (bytecode & 4)
			diff += step ;
		if (bytecode & 8)
			diff = -diff ;

		predictor += diff ;

		if (predictor > 32767)
			predictor = 32767 ;
		else if (predictor < -32768)
			predictor = -32768 ;

		stepindx [chan] += ima_indx_adjust [bytecode] ;
		stepindx [chan] = clamp_ima_step_index (stepindx [chan]) ;

		pima->samples [k] = predictor ;
		} ;

	return 1 ;
} /* wav_w64_ima_decode_block */

static int
ima_read_block (int fd, IMA_ADPCM_PRIVATE *pima, short *ptr, int len)
{	int		count, total = 0, indx = 0 ;

	while (indx < len)
	{
		if (pima->blockcount >= pima->blocks && pima->samplecount >= pima->samplesperblock)
		{	memset (&(ptr [indx]), 0, (size_t) ((len - indx) * sizeof (short))) ;
			return total ;
			} ;

		if (pima->samplecount >= pima->samplesperblock)
			wav_w64_ima_decode_block (fd, pima) ;

		count = (pima->samplesperblock - pima->samplecount) * pima->channels ;
		count = (len - indx > count) ? count : len - indx ;

		memcpy (&(ptr [indx]), &(pima->samples [pima->samplecount * pima->channels]), count * sizeof (short)) ;
		indx += count ;
		pima->samplecount += count / pima->channels ;
		total = indx ;
		} ;

	return total ;
} /* ima_read_block */

static sf_count_t
ima_read_f (int fd, IMA_ADPCM_PRIVATE* pima, float *ptr, sf_count_t len)
{	short		*sptr ;
	int			k, bufferlen, readcount, count ;
	sf_count_t	total = 0 ;
	float		normfact ;

	if (! pima)
		return 0;

	normfact = 1.0 / ((float) 0x8000);

	sptr = sbuf ;
	bufferlen = ARRAY_LEN (sbuf) ;
	//vm_log_info("bufferlen=%d",bufferlen);
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : (int) len ;
		count = ima_read_block (fd, pima, sptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [total + k] = normfact * (float) (sptr [k]) ;
		total += count ;
		len -= readcount ;
		if (count != readcount)
			break ;
		} ;

	return total ;
} /* ima_read_f */

static IMA_ADPCM_PRIVATE*
ima_reader_init (int fd, WavHeader* header)
{
  int channels = header->channels;
  int blockalign = header->blockalign;
  int samplesperblock = header->samplesperblock;

  IMA_ADPCM_PRIVATE	*pima ;
	int	pimasize, count ;

	pimasize = sizeof (IMA_ADPCM_PRIVATE) + blockalign * channels + 3 * channels * samplesperblock ;

	pima = (IMA_ADPCM_PRIVATE*) vm_calloc (pimasize);

	pima->samples	= pima->data ;
  pima->block		= (unsigned char*) (pima->data + samplesperblock * channels) ;

	pima->channels			= channels ;
	pima->blocksize			= blockalign ;
	pima->samplesperblock	= samplesperblock ;

  sf_count_t datalength = header->datachunk_size;
	// sf_count_t datalength = (psf->dataend) ? psf->dataend - psf->dataoffset :
	// 						psf->filelength - psf->dataoffset ;

	if (datalength % pima->blocksize)
		pima->blocks = datalength / pima->blocksize + 1 ;
	else
		pima->blocks = datalength / pima->blocksize ;

	//SF_FORMAT_W64
	count = 2 * (pima->blocksize - 4 * pima->channels) / pima->channels + 1 ;

	wav_w64_ima_decode_block (fd, pima) ;	/* Read first block. */
  sf_count_t frames = pima->samplesperblock * pima->blocks ;

	return pima;
} /* ima_reader_init */

//void wavread(char *file_name, int16_t **samples)
//{
//    int fd;
//
//    if (!file_name)
//        errx(1, "Filename not specified");
//
//    if ((fd = open(file_name, O_RDONLY)) < 1)
//        errx(1, "Error opening file");
//
//    WavHeader header;
//    if (read(fd, &header, sizeof(WavHeader)) < sizeof(WavHeader))
//        errx(1, "File broken: header");
//
//    // printf("chunk_id: %s\n",     header->chunk_id);
//    // printf("chunk_size: %d\n",     header->chunk_size);
//    // printf("fmtchunk_size: %d\n",     header->fmtchunk_size);
//    // printf("format: %s\n",     header->fmtchunk_id);
//    // printf("audio_format: %d\n",     header->audio_format);
//
//    // if (strncmp(header->chunk_id, "RIFF", 4) ||
//    //     strncmp(header->format, "WAVE", 4))
//    //     errx(1, "Not a wav file");
//    //
//
//    printf("chunk_size: %d\n",     header.chunk_size);
//    printf("header.num_channels: %d\n",    header.channels);
//    printf("header.block_align: %d\n",    header.blockalign);
//    printf("header.dwSampleLength: %d\n",    header.dwSampleLength);
//    printf("header.samplesperblock: %d\n",    header.samplesperblock);
//
//    printf("extrabytes: %d\n",    header.extrabytes);
//    printf("factchunk_id: %s\n",    header.factchunk_id);
//    printf("factchunk_size: %d\n",    header.factchunk_size);
//    printf("datachunk_id: %s\n",    header.datachunk_id);
//    printf("datachunk_size: %d\n",    header.datachunk_size);
//
//    IMA_ADPCM_PRIVATE *pima = ima_reader_init(fd, &header);
//
//    int count = 1;
//    float ptr[count];
//    ima_read_f (fd, pima, ptr, count);
//    int i,j;
//    for (i = 0; i < count; i++) {
//      printf("%f ", ptr[i]);
//    }
//    for (j = 0; j < 500; j++) {
//      ima_read_f (fd, pima, ptr, count);
//      for (i = 0; i < count; i++) {
//        printf("%f ", ptr[i]);
//      }
//    }
//    close(fd);
//}

// void wavwrite(char *file_name, int16_t *samples)
// {
//     int fd;
//
//     if (!file_name)
//         errx(1, "Filename not specified");
//
//     if (!samples)
//         errx(1, "Samples buffer not specified");
//
//     if ((fd = creat(file_name, 0666)) < 1)
//         errx(1, "Error creating file");
//
//     if (write(fd, header, sizeof(WavHeader)) < sizeof(WavHeader))
//         errx(1, "Error writing header");
//
//     if (write(fd, samples, header->datachunk_size) < header->datachunk_size)
//         errx(1, "Error writing samples");
//
//     close(fd);
// }
