
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <assert.h>

#include <fcntl.h>
#include <errno.h>

#include "inifile.h"

#define IS_SHOWDEBUG_MESSAGE		0

#if IS_SHOWDEBUG_MESSAGE
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
*ini file key value struct
*/
typedef struct key_value_node{
	char					*key;
	char					*value;
	struct key_value_node	*next;
}KEY_VALUE_NODE;
/*
*ini file section struct
*/
typedef struct file_section_node{
	char					*section;
	KEY_VALUE_NODE			*under;
	struct file_section_node *next;
}FILE_SECTION_NODE;
/*
*ini file info struct
*/
typedef struct ini_file_s{
	char					*name;
	FILE_SECTION_NODE		*under;
	struct ini_file_s		*next;
	pthread_mutex_t 		file_lock;
}INI_FILE_S;

static INI_FILE_S	*head = NULL,*tail = NULL;
pthread_mutex_t init_lock;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void* malloc_memset(size_t size)
{
	void *p = NULL;

	p = malloc(size);
	if(p)
	{
		memset(p,0,size);
	}else{
		PERROR("system call malloc error\n");
	}
	
	return p;
}

int str_int_len(const char *str)
{
	int len = 0;
	if(!str)
		return len;
	while(*str)
	{
		str++;
		len++;
	}
	return len;
}

char* str_frist_constchar(char *str,const char c)
{
	char *p = str;
	if(!str)
		return NULL;
	while(*p)
	{
		if(*p == c)
			return p;
		else
			p++;
	}
	return NULL;
}

char *find_frist_endchar(char *buff)
{
	char *p = buff;
	while(*p!=0)
	{
		p++;
	}
	return p;
}

void add_inifile_node(INI_FILE_S *p)
{
	if(head == NULL)
	{
		head = p;
		tail = p;
	}else{
		tail->next = p;
		tail = p;
	}
}

int check_inifile_isinit(const char *filename,int len,INI_FILE_S *q)
{
	int ret = 0;
	INI_FILE_S *p = head;

	do{
		if(p->name)
		{
			ret = memcmp(((const void *))(p->name),((const void *))filename,len);
			if(ret == 0)
			{
				PERROR("The inifile is already load in memory\n");
				return INIFILE_ALREADY_INIT;
			}
		}else if(q == NULL){
			q = p;
		}

		p = p->next;
		if(q == NULL)
			ret++;
	}while(p);
	
	return ret;
}

INI_FILE_LINE_TYPE judge_ini_file_linetype(char *line)
{
	char *pp = line,*p_brack = NULL,*q_brack = NULL,*p_equ = NULL,*p_comma = NULL;

	while(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
		pp++;
	
	if(*pp == CONTENT_NEELINE_SIGN)
		return LINE_BLANK;
	
	p_comma = str_frist_constchar(line,CONTENT_COMMA_SIGN);
	p_brack = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
	p_equ = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);

	if(p_comma)
	{
		if(p_brack && p_equ)
		{
			if(p_comma < p_brack && p_comma < p_equ)
			{
				while(pp < p_comma)
				{
					if(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
						pp++;
					else
						return LINE_ERROR;
				}
				return LINE_NOTE;
			}else if(p_comma > p_brack && p_comma > p_equ)
			{
				return LINE_ERROR;
			}else if(p_comma > p_brack)
			{
				q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
				if(q_brack && q_brack < p_comma)
					return LINE_SECTION;
				else
					return LINE_ERROR;
			}else{
				return LINE_KEYVALUE;
			}
		}else if(p_brack){
			if(p_comma <  p_brack)
			{
				while(pp < p_comma)
				{
					if(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
						pp++;
					else
						return LINE_ERROR;
				}
				return LINE_NOTE;
			}else{
				q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
				if(q_brack && q_brack < p_comma)
					return LINE_SECTION;
				else
					return LINE_ERROR;
			}
		}else if(p_equ)
		{
			if(p_comma < p_equ)
			{
				while(pp < p_comma)
				{
					if(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
						pp++;
					else
						return LINE_ERROR;
				}
				return LINE_NOTE;
			}
			else
				return LINE_KEYVALUE;
		}else{
			while(pp < p_comma)
			{
				if(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
					pp++;
				else
					return LINE_ERROR;
			}
			return LINE_NOTE;
		}
	}else{
		if(p_brack && p_equ)
			return LINE_ERROR;
		else if(p_brack)
		{
			q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
			if(q_brack && q_brack > p_brack)
				return LINE_SECTION;
			else
				return LINE_ERROR;
		}else if (p_equ)
			return LINE_KEYVALUE;
		else
			return LINE_ERROR;
	}
}

int init_line_key_val(char *line,INI_FILE_S *p)
{
	int len = 0,ret = 0;
	char *p = line,*p_equ = NULL,*pp = NULL,*p_comma = NULL,*p_key = NULL,*p_value = NULL;
	FILE_SECTION_NODE *q = p->under;
	KEY_VALUE_NODE *pk = NULL,*ppk = NULL;

	p_equ = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);
	p_comma = str_frist_constchar(line,CONTENT_COMMA_SIGN);

	if(p_comma && p_comma < p_equ)
		return INIFILE_FORMAT_ERROR;

	if(p_equ && p < p_equ)
	{
		while(*p == CONTENT_TAB_SIGN || *p == CONTENT_SPACE_SIGN)
			p++;

		pp = p_equ - 1;
		while(*pp == CONTENT_TAB_SIGN || *pp == CONTENT_SPACE_SIGN)
			pp--;

		if(pp < p)
			return INIFILE_FORMAT_ERROR;

		len = pp - p + 1;
		p_key = (char *)malloc_memset(len);
		if(p_key == NULL)
		{
			return INIFILE_SYSTEM_ERROR;
		}
		len--;
		memcpy(p_key,p,len);

		while(q->next)
			q = q->next;
		ppk = find_keyvalue_unsect(q,p_key,len);
		if(ppk)
		{
			ret = INIFILE_FORMAT_ERROR;
			goto error_out;
		}

		if(p_comma)
		{
			pp = p_comma;
		}else{		
			pp = find_frist_endchar(p_equ);
		}
		pp--;
		while(*pp == CONTENT_NEELINE_SIGN || *pp == CONTENT_TAB_SIGN || *pp == CONTENT_SPACE_SIGN)
			pp--;
		p = p_equ + INIFILE_SPACE_CHAR_LEN;
		while(*p == CONTENT_TAB_SIGN || *p == CONTENT_SPACE_SIGN)
			p++;

		if(pp < p)
		{
			ret = INIFILE_FORMAT_ERROR;
			goto error_out;
		}
		len = pp - p + 1;
		p_value = (char *)malloc_memset(len);
		if(p_value == NULL)
		{
			ret = INIFILE_SYSTEM_ERROR;
			goto error_out;
		}
		len--;
		memcpy(p_value,p,len);

		pk = (KEY_VALUE_NODE *)malloc_memset(sizeof(KEY_VALUE_NODE));
		if(pk == NULL)
		{
			ret = INIFILE_SYSTEM_ERROR;
			goto error_out;
		}
		pk->key = p_key;
		pk->value = p_value;
		if(q->under == NULL)
		{
			q->under = pk;
		}else{
			ppk = q->under;
			while(ppk->next)
				ppk = ppk->next;
			ppk->next = pk;
		}
		
		ret = 0;
	}else{
		ret = INIFILE_FORMAT_ERROR;
	}
	
	return ret;
error_out:
	if(p_key)
		free(p_key);
	if(p_value)
		free(p_value);
	if(pk)
		free(pk);
	return ret;
}

int init_line_section(char *line,INI_FILE_S *p)
{
	int len = 0,ret = 0;
	char *pp = line,*p_brack = NULL,*q_brack = NULL,*ps = NULL;
	FILE_SECTION_NODE *q = NULL,*qq = p->under;

	p_brack = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
	q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
	if(p_brack && q_brack)
	{
		p_brack++;q_brack--;
		if(p_brack < q_brack)
		{
			len = q_brack - p_brack + 1;
			ps = (char *)malloc_memset(len);
			if(ps == NULL)
				return INIFILE_SYSTEM_ERROR;
			len--;
			memcpy(ps,p_brack,len);

			q = find_inifile_section(p,ps,len);
			if(q)
			{
				ret = INIFILE_FORMAT_ERROR;
				goto error_out;
			}
			q = (FILE_SECTION_NODE *)malloc_memset(sizeof(FILE_SECTION_NODE));
			if(q == NULL)
			{
				ret = INIFILE_SYSTEM_ERROR;
				goto error_out;
			}
			q->section = ps;
			if(p->under == NULL)
			{
				p->under = q;
			}else{
				while(qq->next)
					qq = qq->next;
				qq->next = q;
			}
			
			ret = 0;
		}else{
			ret = INIFILE_FORMAT_ERROR;
		}
	}else{
		ret = INIFILE_FORMAT_ERROR;
	}
	return ret;
error_out:
	if(ps)
		free(ps);
	if(q)
		free(q);
	return ret;
}

int init_every_line_inifile(char * line,INI_FILE_S *p)
{
	int ret = 0;
	INI_FILE_LINE_TYPE type = LINE_ERROR;

	type = judge_ini_file_linetype(line);
	switch(type)
	{
		case LINE_SECTION:
			ret = init_line_section(line,p);
			break;
		case LINE_KEYVALUE:
			ret = init_line_key_val(line,p);
			break;
		case LINE_NOTE:
			break;
		case LINE_BLANK:
			break;
		case LINE_ERROR:
			ret = INIFILE_FORMAT_ERROR;
			break;
		default:
			ret = INIFILE_FORMAT_ERROR;
			break;
	}
	return ret;
}

int read_analys_ini_file(const char *file_name,INI_FILE_S *p)
{
	int ret = 0;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0};
	
	FILE *fp = fopen(file_name,"r");
	if(fp == NULL)
	{
		PERROR("File no exise or can't open\n");
		return INIFILE_NO_EXIST;
	}
	
	while(fgets(line,(INIFILE_MAX_CONTENT_LINELEN),fp) != NULL)
	{
		ret = init_every_line_inifile(ini_fd,line);
		if(ret < 0)
			break;
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	fclose(fp);
	return ret;	
}

KEY_VALUE_NODE* find_keyvalue_unsect(FILE_SECTION_NODE *p,const char *key,int len)
{
	int ret = 0,c_len = 0;
	KEY_VALUE_NODE *pk = p->under
	if(key == NULL || p == NULL || p->under == NULL)
		return NULL;

	if(len <= 0)
		len = str_int_len(key);

	while(pk)
	{
		c_len = str_int_len(pk->key);
		if(len == c_len)
		{
			ret = memcmp((const void *)(pk->key),(const void *),,len);
			if(ret == 0)
				break;
		}
		pk = pk->next;
	}
	return pk;
}

void destroy_all_keyvalue_mem(FILE_SECTION_NODE *p)
{
	KEY_VALUE_NODE *pk = p->under,*qk = NULL;
	while(pk)
	{
		qk = pk->next;
		if(pk->key)
			free(pk->key);
		if(pk->value)
			free(pk->value);
		free(pk);
		pk = qk;
	}
	p->under = NULL;
}

FILE_SECTION_NODE* find_inifile_section(INI_FILE_S *p,const char *section,int len)
{
	int ret = 0,c_len = 0;
	FILE_SECTION_NODE *pu = p->under;
	
	if(section == NULL || p == NULL || p->under == NULL)
		return NULL;

	if(len <= 0)
		len = str_int_len(section);
	while(pu)
	{
		c_len = str_int_len(pu->section);
		if(len == c_len)
		{
			ret = memcmp((const void *)(pu->section),(const void *)section,len);
			if(ret == 0)
				break;
		}
		pu = pu->next;
	}

	return pu;
}

void destroy_all_section_mem(INI_FILE_S *p)
{
	FILE_SECTION_NODE *pu = p->under,*qu = NULL;
	while(pu)
	{
		qu = pu->next;
		destroy_all_keyvalue_mem(pu);
		if(pu->section)
			free(pu->section);
		free(pu);
		pu = qu;
	}

	p->under = NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int init_ini_file(const char *filename,int len)
{
	char *file_name = NULL;
	int name_len = 0,ret = 0,result = 0,add = 0;
	INI_FILE_S *p_file = NULL;

	if(filename == NULL)
	{
		PERROR("parameter is null can't be use\n");
		return INIFILE_ERROR_PARAMETER;
	}

	if(len == 0)
		name_len = strlen(filename) + 1;
	else
		name_len = len + 1;
	len = name_len - 1;

	file_name = (char *)malloc_memset(name_len);
	if(file_name == NULL)
	{
		return INIFILE_SYSTEM_ERROR;
	}
	memcpy(file_name,filename,len);
	ret = access(file_name, 6);
	if(ret != 0)
	{
		PERROR("There is no power to access ini file\n");
		ret = INIFILE_POWER_ERROR;
		goto error_out;
	}

	if(head)
	{
		result = check_inifile_isinit(filename,len,&p_file);
		if(result < 0)
		{
			ret = result;
			goto error_out;
		}
	}
	if(p_file == NULL)
	{
		p_file = (INI_FILE_S *)malloc_memset(sizeof(INI_FILE_S));
		if(p_file == NULL)
		{
			ret = INIFILE_SYSTEM_ERROR;
			goto error_out;
		}
		add = 1;
	}

	ret = read_analys_ini_file(filename,p_file);
	if(ret < 0)
	{
		destroy_all_section_mem(p_file);
		goto error_out;
	}

	p_file->name = filename;
	if(add)
		add_inifile_node(p_file);
	return result;
error_out:
	if(file_name)
		free(file_name);
	if(p_file && add == 1)
		free(p_file);
	return ret;
}

int delete_ini_section(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,i = 0,len = 0;
}

int add_ini_section(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,i = 0,len = 0;
	char *p_section = NULL;
	INI_FILE_S *p = head;
	FILE_SECTION_NODE *q = NULL,*qq = NULL;

	if(head == NULL)
	{
		PERROR("The memory never load any inifile\n");
		return INIFILE_NEVER_LOAD;
	}

	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			PERROR("The memory never load this inifile\n");
			return INIFILE_NEVER_LOAD;
		}
	}

	len = parameter->section_len;
	if(len <= 0)
		len = str_int_len(parameter->section) + 1;
	parameter->section_len = len - 1;
	p_section = (char *)malloc_memset(len);
	if(p_section == NULL)
	{
		ret = INIFILE_SYSTEM_ERROR;
		goto error_out;
	}
	memcpy(p_section,parameter->section,parameter->section_len);

	q = find_inifile_section(p,p_section,parameter->section_len);
	if(q)
	{
		ret = INIFILE_SECTION_ALREAD;
		goto error_out;
	}
	q = (FILE_SECTION_NODE *)malloc_memset(sizeof(FILE_SECTION_NODE));
	if(q == NULL)
	{
		ret = INIFILE_SYSTEM_ERROR;
		goto error_out;
	}
	q->section = p_section;
	if(p->under == NULL)
	{
		p->under = q;
	}
	else{
		qq = p->under;
		while(qq->next)
			qq = qq->next;
		qq->next = q;
	}
	//TODO
	ret = 0;
	return ret;
error_out:
	if(p_section)
		free(p_section);
	if(q)
		free(q);
	return ret;
}


void destroy_ini_source(int ini_fd)
{
	int i = 0;
	INI_FILE_S *p = head;
	if(p == NULL)
	{
		PERROR("There is no any inifile load in memory\n");
		return;
	}

	for(i = 0;i < ini_fd;i++)
	{
		p = p->next;
		if(p == NULL)
		{
			PERROR("The paramter is out of load inifile array bounds\n");
			return;
		}
	}

	destroy_all_section_mem(p);
	if(p->name)
		free(p->name);
	p->name = NULL;
	return;
}

