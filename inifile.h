#ifndef __PROGRAM_INIFILE_VTWO_H__
#define __PROGRAM_INIFILE_VTWO_H__
#ifdef __cplusplus
extern "C" {
#endif


#define MAX_OPERATION_FILE_NUM			2//The macro define max inifile can operation same time;use in inifile stack
#define MAX_INIFILE_SECTION_NUM			10//define max section in each inifile;use in inifile stack
#define MAX_KEYVALUE_UNDER_SECTION		100//define max key value in each inifile;use in inifile stack

#define INIFILE_SIZE_SPACE				(1024 * 3)//define max inifile memory size;use in inifile stack

#define INIFILE_MAX_CONTENT_LINELEN		1024
//////////////////////////////////////////////////////////////////////////constant define
#define CONTENT_NEELINE_SIGN			10
#define CONTENT_SPACE_SIGN				32
#define CONTENT_TAB_SIGN				9

#define CONTENT_LEFT_BRACKET			91//[
#define CONTENT_RIGHT_BRACKET			93///]
#define CONTENT_COMMA_SIGN				59//;
#define CONTENT_EQUALITY_SIGN			61//=

//////////////////////////////////////////////////////////////////////////ERROR CODE DEFINE

#define INIFILE_NO_EXIST				-1
#define INIFILE_ALREADY_INIT			-2
#define INIFILE_FORMAT_ERROR			-3
#define INIFILE_POWER_ERROR				-4

#define INIFILE_SYSTEM_ERROR			-11
#define INIFILE_REWRITE_ERROR			-12

#define INIFILE_ERROR_PARAMETER			-21
#define INIFILE_SECTION_NOFOUND			-22
#define INIFILE_KEYVALUE_NOFOUND		-23
#define INIFILE_SECTION_ALREAD			-24
#define INIFILE_KEYVALUE_ALREAD			-25
#define INIFILE_MEMORY_POOL_ERROR		-26
#define INIFILE_MEMORY_POOL_OUT			-27
#define INIFILE_NUM_OUT_ARRAY			-28
#define INIFILE_SECTION_ARRAY			-29
#define INIFILE_KEYVALUE_ARRAY			-20


#define INIFILE_ERROR_ARRAYLEN			-4

/**
* operation parameter;
* every buff char must be end of '\0'
* or The buff char len must be effective
**/
typedef struct ini_parameter{
	char*	section;
	int		section_len;
	char*	key;
	int		key_len;
	char*	value;
	int		value_len;
}INI_PARAMETER;

//if filename end of '\0' the len can be 0
int init_ini_file(const char *filename,int len);

const char *get_value_ofkey(int ini_fd,INI_PARAMETER *parameter);

int update_value_ofkey(int ini_fd,INI_PARAMETER *parameter);

int add_value_ofkey(int ini_fd,INI_PARAMETER *parameter);

int delete_value_ofkey(int ini_fd,INI_PARAMETER *parameter);

int delete_ini_section(int ini_fd,INI_PARAMETER *parameter);

int add_ini_section(int ini_fd,INI_PARAMETER *parameter);

void destroy_ini_source(int ini_fd);

#ifdef __cplusplus
}
#endif
#endif

