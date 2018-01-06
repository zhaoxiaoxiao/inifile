
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include <assert.h>

#include <fcntl.h>
#include <errno.h>

#include "inifile.h"

#define IS_SHOWDEBUG_MESSAGE		1

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
static pthread_mutex_t init_lock;

KEY_VALUE_NODE* find_keyvalue_unsect(FILE_SECTION_NODE *p,const char *key,int len);
FILE_SECTION_NODE* find_inifile_section(INI_FILE_S *p,const char *section,int len);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void delete_tmp_name_file(const char *filename)
{
	char cmd_str[INIFILE_SYSTEM_CMD_LEN] = {0};
	sprintf(cmd_str,"rm -rf %s%s",filename,tem_suff_name);
	system (cmd_str);
}

void re_name_ini_file(const char *filename)
{
	char cmd_str[INIFILE_SYSTEM_CMD_LEN] = {0};
	sprintf(cmd_str,"rm -rf %s && mv %s%s %s",filename,filename,tem_suff_name,filename);
	system (cmd_str);
}

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
	if(str == NULL)
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

void add_inifile_section(INI_FILE_S *p,FILE_SECTION_NODE *q)
{
	FILE_SECTION_NODE *qq = p->under;
	if(qq == NULL)
	{
		p->under = q;
	}
	else{
		while(qq->next)
			qq = qq->next;
		qq->next = q;
	}
}

void add_inifile_keyvalue(FILE_SECTION_NODE *p,KEY_VALUE_NODE *q)
{
	KEY_VALUE_NODE *qq = p->under;
	if(qq == NULL)
	{
		p->under = q;
	}else{
		while(qq->next)
			qq = qq->next;
		qq->next = q;
	}
}

int check_inifile_isinit(const char *filename,int len,INI_FILE_S **q)
{
	int ret = 0;
	INI_FILE_S *p = head;

	do{
		pthread_mutex_lock(&(p->file_lock));
		if(p->name)
		{
			ret = memcmp((const void *)(p->name),(const void *)filename,len);
			if(ret == 0)
			{
				PERROR("The inifile is already load in memory\n");
				return INIFILE_ALREADY_INIT;
			}
		}else if(*q == NULL){
			*q = p;
		}
		pthread_mutex_unlock(&(p->file_lock));
		
		p = p->next;
		if(*q == NULL)
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
					{
						PERROR("There is unknow char before comma sign :: %s",line);
						return LINE_ERROR;
					}
				}
				return LINE_NOTE;
			}else if(p_comma > p_brack && p_comma > p_equ)
			{
				PERROR("The equal sign can't be appear in section name :: %s",line);
				return LINE_ERROR;
			}else if(p_comma > p_brack)
			{
				q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
				if(q_brack && q_brack < p_comma)
					return LINE_SECTION;
				else
				{
					PERROR("The is no right brack for the left brack :: %s",line);
					return LINE_ERROR;
				}
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
					{
						PERROR("There is unknow char before comma sign :: %s",line);
						return LINE_ERROR;
					}
				}
				return LINE_NOTE;
			}else{
				q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
				if(q_brack && q_brack < p_comma)
					return LINE_SECTION;
				else
				{
					PERROR("The is no right brack for the left brack :: %s",line);
					return LINE_ERROR;
				}
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
					{
						PERROR("There is unknow char before comma sign :: %s",line);
						return LINE_ERROR;
					}
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
				{
					PERROR("There is unknow char before comma sign :: %s",line);
					return LINE_ERROR;
				}
			}
			return LINE_NOTE;
		}
	}else{
		if(p_brack && p_equ)
		{
			PERROR("There is some format error in this line :: %s",line);
			return LINE_ERROR;
		}
		else if(p_brack)
		{
			q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
			if(q_brack && q_brack > p_brack)
				return LINE_SECTION;
			else
			{
				PERROR("There is some format error in this line :: %s",line);
				return LINE_ERROR;
			}
		}else if (p_equ)
		{
			return LINE_KEYVALUE;
		}
		else
		{
			PERROR("There is some format error in this line :: %s",line);
			return LINE_ERROR;
		}
	}
}

int rewrite_after_add(const char *filename,INI_PARAMETER *parameter)
{
	int ret = 0,len = 0,flag = 0;
	FILE *rp = NULL,*wp = NULL;
	char tmp_name[INIFILE_FILEALL_NAME_LEN] = {0},*p_char = NULL,*q_char = NULL;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0};
	INI_FILE_LINE_TYPE type;

	if(parameter->section == NULL)
		return INIFILE_ERROR_PARAMETER;
	if(parameter->section_len <= 0)
		parameter->section_len = str_int_len(parameter->section);
	sprintf(tmp_name,"%s%s",filename,tem_suff_name);
	rp = fopen(filename,"r");
	if(rp == NULL)
	{
		PERROR("source inifile open failed\n");
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}
	wp = fopen(tmp_name,"w");
	if(wp == NULL)
	{
		PERROR("tmp inifile open failed\n");
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}

	if(parameter->key == NULL)
	{
		flag = 1;
	}else{
		if(parameter->key_len <= 0)
			parameter->key_len = str_int_len(parameter->key);
		if(parameter->value_len <= 0)
			parameter->value_len = str_int_len(parameter->value);
	}

	while(fgets(line,INIFILE_MAX_CONTENT_LINELEN,rp) != NULL)
	{
		if(flag == 0)
		{
			type = judge_ini_file_linetype(line);
			switch(type)
			{
				case LINE_SECTION:
					p_char = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
					if(p_char == NULL)
					{
						ret = INIFILE_REWRITE_ERROR;
						goto error_out;
					}else
						p_char++;
					
					q_char = p_char + parameter->section_len;
					if(*q_char == CONTENT_RIGHT_BRACKET)
					{
						ret = memcmp((const void *)(parameter->section),(const void *)p_char,parameter->section_len);
						if(ret == 0)
						{
							p_char = find_frist_endchar(p_char);
							sprintf(p_char,"%s",parameter->key);
							p_char = find_frist_endchar(p_char);
							sprintf(p_char,"\t=\t");
							p_char = find_frist_endchar(p_char);
							sprintf(p_char,"%s",parameter->value);
							p_char = find_frist_endchar(p_char);
							*p_char = CONTENT_NEELINE_SIGN;
							flag = 1;
						}
					}
					break;
				case LINE_KEYVALUE:
					break;
				case LINE_NOTE:
					break;
				case LINE_BLANK:
					break;
				case LINE_ERROR:
					ret = INIFILE_FORMAT_ERROR;
					goto error_out;
					break;
			}
		}
		len = str_int_len(line);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}

	if(parameter->key == NULL)
	{
		p_char = line;
		sprintf(p_char,"[%s]\n\n",parameter->section);
		len = str_int_len(line);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	ret = 0;
error_out:
	if(rp)
		fclose(rp);
	if(wp)
		fclose(wp);
	if(ret < 0)
		delete_tmp_name_file(filename);
	else
		re_name_ini_file(filename);
	return ret;
}

int rewrite_after_update(const char *filename,INI_PARAMETER *parameter)
{
	int ret = 0,len = 0,flag = 0,mod_flag = 0;
	FILE *rp = NULL,*wp = NULL;
	INI_FILE_LINE_TYPE type;
	char tmp_name[INIFILE_FILEALL_NAME_LEN] = {0},*p_char = NULL,*q_char = NULL,*p_value = NULL,*p_comma = NULL;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0},mod_line[INIFILE_MAX_CONTENT_LINELEN] = {0};

	if(parameter->section == NULL || parameter->key == NULL || parameter->value == NULL)
		return INIFILE_ERROR_PARAMETER;

	if(parameter->section_len <= 0)
		parameter->section_len = str_int_len(parameter->section);
	if(parameter->key_len <= 0)
		parameter->key_len = str_int_len(parameter->key);
	if(parameter->value_len <= 0)
		parameter->value_len = str_int_len(parameter->value);
	sprintf(tmp_name,"%s%s",filename,tem_suff_name);
	rp = fopen(filename,"r");
	if(rp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}
	wp = fopen(tmp_name,"w");
	if(wp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}

	while(fgets(line,INIFILE_MAX_CONTENT_LINELEN,rp) != NULL)
	{
		if(mod_flag== 0)
		{
			type = judge_ini_file_linetype(line);
			switch(type)
			{
				case LINE_SECTION:
					p_char = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
					if(p_char == NULL)
					{
						ret = INIFILE_REWRITE_ERROR;
						goto error_out;
					}else
						p_char++;
					q_char = p_char + parameter->section_len;
					if(*q_char == CONTENT_RIGHT_BRACKET)
					{
						ret = memcmp((const void *)(parameter->section),(const void *)p_char,parameter->section_len);
						if(ret == 0)
							flag = 1;
					}
					break;
				case LINE_KEYVALUE:
					if(flag == 1)
					{
						p_value = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);
						if(p_value == NULL)
						{
							ret = INIFILE_REWRITE_ERROR;
							goto error_out;
						}else{
							p_char = line;
							while(*p_char == CONTENT_SPACE_SIGN || *p_char == CONTENT_TAB_SIGN)
							{
								p_char++;
							}
						}

						q_char = p_char + parameter->key_len;
						if(*q_char == CONTENT_SPACE_SIGN || *q_char == CONTENT_TAB_SIGN || *q_char == CONTENT_EQUALITY_SIGN)
						{
							ret = memcmp((const void *)(parameter->key),(const void *)p_char,parameter->key_len);
							if(ret == 0)
							{
								p_value++;
								while(*p_value == CONTENT_SPACE_SIGN || *p_value == CONTENT_TAB_SIGN)
								{
									p_value++;
								}
								len = p_value - line;
								memcpy(mod_line,line,len);
								p_char = find_frist_endchar(mod_line);
							
								sprintf(p_char,"%s",parameter->value);
								p_comma = str_frist_constchar(p_value,CONTENT_COMMA_SIGN);
								if(p_comma)
								{
									p_char = find_frist_endchar(mod_line);
									sprintf(p_char,"\t%s",p_comma);
								}
								memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
								sprintf(line,"%s\n",mod_line);
								mod_flag = 1;
							}
						}
					}
					break;
				case LINE_NOTE:
					break;
				case LINE_BLANK:
					break;
				case LINE_ERROR:
					ret = INIFILE_FORMAT_ERROR;
					goto error_out;
					break;
			}
		}
		len = str_int_len(line);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	ret = 0;
error_out:
	if(rp)
		fclose(rp);
	if(wp)
		fclose(wp);
	if(ret < 0)
		delete_tmp_name_file(filename);
	else
		re_name_ini_file(filename);
	return ret;
}

int rewrite_after_delete(const char *filename,INI_PARAMETER *parameter)
{
	int ret = 0,flag = 0,len = 0;
	FILE *rp = NULL,*wp = NULL;
	char tmp_name[INIFILE_FILEALL_NAME_LEN] = {0},*p_char = NULL,*q_char = NULL;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0},*p_value = NULL;
	INI_FILE_LINE_TYPE type;

	if(parameter->section == NULL)
		return INIFILE_ERROR_PARAMETER;
	if(parameter->section_len <= 0)
		parameter->section_len = str_int_len(parameter->section);
	sprintf(tmp_name,"%s%s",filename,tem_suff_name);
	rp = fopen(filename,"r");
	if(rp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}
	wp = fopen(tmp_name,"w");
	if(wp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}

	while(fgets(line,INIFILE_MAX_CONTENT_LINELEN,rp) != NULL)
	{
		if(flag != 3)
		{
			type = judge_ini_file_linetype(line);
			switch(type)
			{
				case LINE_SECTION:
					p_char = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
					if(p_char == NULL)
					{
						ret = INIFILE_REWRITE_ERROR;
						goto error_out;
					}else
						p_char++;

					if(flag == 1)
					{
						flag = 3;
						break;
					}else if(flag ==2 )
					{
						PERROR("There is no found key value under section\n");	
						flag = 3;
						break;
					}
					
					q_char = p_char + parameter->section_len;
					if(*q_char == CONTENT_RIGHT_BRACKET)
					{
						ret = memcmp((const void *)(parameter->section),(const void *)p_char,parameter->section_len);
						if(ret == 0)
						{
							if(parameter->key)
							{
								if(parameter->key_len <= 0)
									parameter->key_len = str_int_len(parameter->key);
								flag = 2;
							}else{
								flag = 1;
								continue;
							}
						}
					}
					break;
				case LINE_KEYVALUE:
					if (flag == 1)
						continue;
					else if(flag == 2)
					{
						p_value = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);
						if(p_value == NULL)
						{
							ret = INIFILE_REWRITE_ERROR;
							goto error_out;
						}else{
							p_char = line;
							while(*p_char == CONTENT_SPACE_SIGN || *p_char == CONTENT_TAB_SIGN)
							{
								p_char++;
							}
						}

						q_char = p_char + parameter->key_len;
						if(*q_char == CONTENT_SPACE_SIGN || *q_char == CONTENT_TAB_SIGN || *q_char == CONTENT_EQUALITY_SIGN)
						{
							ret = memcmp((const void *)(parameter->key),(const void *)p_char,parameter->key_len);
							if(ret == 0)
							{
								flag = 3;
								continue;
							}
						}
					}
					break;
				case LINE_NOTE:
					break;
				case LINE_BLANK:
					break;
				case LINE_ERROR:
					ret = INIFILE_FORMAT_ERROR;
					goto error_out;
					break;
			}
		}
		len = str_int_len(line);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	ret = 0;
error_out:
	if(rp)
		fclose(rp);
	if(wp)
		fclose(wp);
	if(ret < 0)
		delete_tmp_name_file(filename);
	else
		re_name_ini_file(filename);
	return ret;
}

int init_line_key_val(char *line,INI_FILE_S *p_file)
{
	int len = 0,ret = 0;
	char *p = line,*p_equ = NULL,*pp = NULL,*p_comma = NULL,*p_key = NULL,*p_value = NULL;
	FILE_SECTION_NODE *q = p_file->under;
	KEY_VALUE_NODE *pk = NULL,*ppk = NULL;

	p_equ = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);
	p_comma = str_frist_constchar(line,CONTENT_COMMA_SIGN);

	if(p_comma && p_comma < p_equ)
	{
		PERROR("There is some format error in this line :: %s",line);
		return INIFILE_FORMAT_ERROR;
	}

	if(q == NULL)
	{
		PERROR("There is no any section under this inifile :: %s",p_file->name);
		return INIFILE_FORMAT_ERROR;
	}
	while(q->next)
		q = q->next;

	if(p_equ && p < p_equ)
	{
		while(*p == CONTENT_TAB_SIGN || *p == CONTENT_SPACE_SIGN)
			p++;

		pp = p_equ - 1;
		while(*pp == CONTENT_TAB_SIGN || *pp == CONTENT_SPACE_SIGN)
			pp--;

		if(pp < p)
		{
			PERROR("There is some format error in this line :: %s",line);
			return INIFILE_FORMAT_ERROR;
		}

		len = pp - p + 2;
		p_key = (char *)malloc_memset(len);
		if(p_key == NULL)
		{
			return INIFILE_SYSTEM_ERROR;
		}
		len--;
		memcpy(p_key,p,len);

		
		ppk = find_keyvalue_unsect(q,p_key,len);
		if(ppk)
		{
			PERROR("There is same key name under this section :: %s",line);
			ret = INIFILE_FORMAT_ERROR;
			ppk = NULL;
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
		p = p_equ + 1;
		while(*p == CONTENT_TAB_SIGN || *p == CONTENT_SPACE_SIGN)
			p++;

		if(pp < p)
		{
			PERROR("There is some format error in this line :: %s",line);
			ret = INIFILE_FORMAT_ERROR;
			goto error_out;
		}
		len = pp - p + 2;
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
		add_inifile_keyvalue(q,pk);
		ret = 0;
	}else{
		PERROR("There is some format error in this line :: %s",line);
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
	char *p_brack = NULL,*q_brack = NULL,*ps = NULL;
	FILE_SECTION_NODE *q = NULL;

	p_brack = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
	q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
	if(p_brack && q_brack)
	{
		p_brack++;q_brack--;
		if(p_brack < q_brack)
		{
			len = q_brack - p_brack + 2;
			ps = (char *)malloc_memset(len);
			if(ps == NULL)
				return INIFILE_SYSTEM_ERROR;
			len--;
			memcpy(ps,p_brack,len);

			q = find_inifile_section(p,ps,len);
			if(q)
			{
				PERROR("There is same section in this ini file\n");
				ret = INIFILE_FORMAT_ERROR;
				q = NULL;
				goto error_out;
			}
			q = (FILE_SECTION_NODE *)malloc_memset(sizeof(FILE_SECTION_NODE));
			if(q == NULL)
			{
				ret = INIFILE_SYSTEM_ERROR;
				goto error_out;
			}
			q->section = ps;
			add_inifile_section(p,q);
			ret = 0;
		}else{
			ret = INIFILE_FORMAT_ERROR;
		}
	}else{
		PERROR("There is some format error in this line :: %s",line);
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
		ret = init_every_line_inifile(line,p);
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
	KEY_VALUE_NODE *pk = p->under;
	if(key == NULL || p == NULL || p->under == NULL)
		return NULL;

	if(len <= 0)
	{
		len = str_int_len(key);
	}

	while(pk)
	{
		c_len = str_int_len(pk->key);
		if(len == c_len)
		{
			ret = memcmp((const void *)(pk->key),(const void *)key,len);
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
	int name_len = 0,ret = 0,add = 0;
	INI_FILE_S *p_file = NULL;

	if(filename == NULL)
	{
		PERROR("parameter is null can't be use\n");
		return INIFILE_ERROR_PARAMETER;
	}
	
	if(head == NULL)
		pthread_mutex_init(&init_lock,PTHREAD_MUTEX_TIMED_NP);
	
	pthread_mutex_lock(&init_lock);

	if(len == 0)
		name_len = strlen(filename) + 1;
	else
		name_len = len + 1;
	len = name_len - 1;

	file_name = (char *)malloc_memset(name_len);
	if(file_name == NULL)
	{
		ret = INIFILE_SYSTEM_ERROR;
		goto error_out;
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
		ret = check_inifile_isinit(filename,len,&p_file);
		if(ret < 0)
		{
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
	pthread_mutex_init(&(p_file->file_lock),PTHREAD_MUTEX_TIMED_NP);

	ret = read_analys_ini_file(file_name,p_file);
	if(ret < 0)
	{
		destroy_all_section_mem(p_file);
		goto error_out;
	}

	p_file->name = file_name;
	if(add)
		add_inifile_node(p_file);
	pthread_mutex_unlock(&init_lock);
	ret = 0;
	return ret;
error_out:
	if(file_name)
		free(file_name);
	if(p_file && add == 1)
		free(p_file);
	pthread_mutex_unlock(&init_lock);
	return ret;
}

int get_value_ofkey(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,i = 0,len = 0;
	char section[INIFILE_MAX_CONTENT_LINELEN] = {0},key[INIFILE_MAX_CONTENT_LINELEN] = {0};
	INI_FILE_S *p = head;
	FILE_SECTION_NODE *q = NULL;
	KEY_VALUE_NODE *pk = NULL;

	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return INIFILE_NEVER_LOAD;
	}

	if(parameter->section == NULL || parameter->key == NULL || parameter->value == NULL || parameter->value_len < 2)
	{
		PERROR("There is somewrong about parameter inside\n");
		return INIFILE_ERROR_PARAMETER;
	}
	pthread_mutex_lock(&(p->file_lock));
	if(parameter->section_len > 0)
	{
		memcpy(section,parameter->section,parameter->section_len);
	}
	else{
		parameter->section_len = str_int_len(parameter->section);
		memcpy(section,parameter->section,parameter->section_len);
	}

	if(parameter->key_len > 0)
	{
		memcpy(key,parameter->key,parameter->key_len);
	}else{
		parameter->key_len = str_int_len(parameter->key);
		memcpy(key,parameter->key,parameter->key_len);
	}

	q = find_inifile_section(p,section,parameter->section_len);
	if(q == NULL)
	{
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}

	pk = find_keyvalue_unsect(q,key,parameter->key_len);
	if(pk == NULL)
	{
		ret = INIFILE_KEYVALUE_NOFOUND;
		goto error_out;
	}

	len = str_int_len(pk->value);
	if(len >= parameter->value_len)
	{
		PERROR("The value len is no enough to storage the value\n");
		ret = INIFILE_NUM_OUT_ARRAY;
		goto error_out;
	}
	memset(parameter->value,0,parameter->value_len);
	memcpy(parameter->value,pk->value,len);
	ret = 0;
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
error_out:
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
	
}

int update_value_ofkey(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,i = 0,len = 0;
	char *p_value = NULL,section[INIFILE_MAX_CONTENT_LINELEN] = {0},key[INIFILE_MAX_CONTENT_LINELEN] = {0};
	INI_FILE_S *p = head;
	FILE_SECTION_NODE *q = NULL;
	KEY_VALUE_NODE *pk = NULL;

	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return INIFILE_NEVER_LOAD;
	}

	if(parameter->section == NULL || parameter->key == NULL || parameter->value == NULL)
		return INIFILE_ERROR_PARAMETER;
	pthread_mutex_lock(&(p->file_lock));
	if(parameter->section_len > 0)
	{
		memcpy(section,parameter->section,parameter->section_len);
	}
	else{
		parameter->section_len = str_int_len(parameter->section);
		memcpy(section,parameter->section,parameter->section_len);
	}

	if(parameter->key_len > 0)
	{
		memcpy(key,parameter->key,parameter->key_len);
	}else{
		parameter->key_len = str_int_len(parameter->key);
		memcpy(key,parameter->key,parameter->key_len);
	}
	if(parameter->value_len <= 0)
	{
		parameter->value_len = str_int_len(parameter->value);
	}

	q = find_inifile_section(p,section,parameter->section_len);
	if(q == NULL)
	{
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}

	pk = find_keyvalue_unsect(q,key,parameter->key_len);
	if(pk == NULL)
	{
		ret = INIFILE_KEYVALUE_NOFOUND;
		goto error_out;
	}

	len = parameter->value_len + 1;
	p_value = (char *)malloc_memset(len);
	if(p_value == NULL)
	{
		ret = INIFILE_SYSTEM_ERROR;
		goto error_out;
	}
	memcpy(p_value,parameter->value,parameter->value_len);
	ret = rewrite_after_update(p->name,parameter);
	if(ret < 0)
	{
		PERROR("There is something wrong sync to io file\n");
		goto error_out;
	}
	if(pk->value)
		free(pk->value);
	pk->value = p_value;
	ret = 0;
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
error_out:
	if(p_value)
		free(p_value);
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
}

int add_value_ofkey(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,i = 0,len = 0;
	char *p_value = NULL,*p_key = NULL,section[INIFILE_MAX_CONTENT_LINELEN] = {0};
	INI_FILE_S *p = head;
	FILE_SECTION_NODE *q = NULL;
	KEY_VALUE_NODE *pk = NULL;

	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return INIFILE_NEVER_LOAD;
	}

	if(parameter->section == NULL || parameter->key == NULL || parameter->value == NULL)
		return INIFILE_ERROR_PARAMETER;
	pthread_mutex_lock(&(p->file_lock));
	if(parameter->section_len > 0)
	{
		memcpy(section,parameter->section,parameter->section_len);
	}
	else{
		parameter->section_len = str_int_len(parameter->section);
		memcpy(section,parameter->section,parameter->section_len);
	}
	if(parameter->key_len <= 0)
	{
		parameter->key_len = str_int_len(parameter->key);
	}
	if(parameter->value_len <= 0)
	{
		parameter->value_len = str_int_len(parameter->value);
	}

	q = find_inifile_section(p,section,parameter->section_len);
	if(q == NULL)
	{
		PERROR("There is no section in this ini file\n");
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}

	len = parameter->key_len + 1;
	p_key = (char *)malloc_memset(len);
	if(p_key == NULL)
	{
		ret = INIFILE_SYSTEM_ERROR;
		goto error_out;
	}
	memcpy(p_key,parameter->key,parameter->key_len);
	pk = find_keyvalue_unsect(q,p_key,parameter->key_len);
	if(pk)
	{
		PERROR("There is same key under this section can't be add\n");
		ret = INIFILE_KEYVALUE_ALREAD;
		pk = NULL;
		goto error_out;
	}
	
	len = parameter->value_len + 1;
	p_value = (char *)malloc_memset(len);
	if(p_value == NULL)
	{
		ret = INIFILE_SYSTEM_ERROR;
		goto error_out;
	}
	memcpy(p_value,parameter->value,parameter->value_len);

	pk = (KEY_VALUE_NODE *)malloc_memset(sizeof(KEY_VALUE_NODE));
	if(pk == NULL)
	{
		ret = INIFILE_SYSTEM_ERROR;
		goto error_out;
	}
	pk->key = p_key;
	pk->value = p_value;
	ret = rewrite_after_add(p->name,parameter);
	if(ret < 0)
	{
		PERROR("There is something wrong sync to io file\n");
		goto error_out;
	}
	add_inifile_keyvalue(q,pk);
	ret = 0;
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
error_out:
	if(p_key)
		free(p_key);
	if(p_value)
		free(p_value);
	if(pk)
		free(pk);
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
}

int delete_value_ofkey(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,i = 0;
	char section[INIFILE_MAX_CONTENT_LINELEN] = {0},key[INIFILE_MAX_CONTENT_LINELEN] = {0};
	INI_FILE_S *p = head;
	FILE_SECTION_NODE *q = NULL;
	KEY_VALUE_NODE *pk = NULL,*ppk = NULL;

	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return INIFILE_NEVER_LOAD;
	}
	if(parameter->section == NULL || parameter->key == NULL )
		return INIFILE_ERROR_PARAMETER;
	pthread_mutex_lock(&(p->file_lock));
	if(parameter->section_len > 0)
	{
		memcpy(section,parameter->section,parameter->section_len);
	}
	else{
		parameter->section_len = str_int_len(parameter->section);
		memcpy(section,parameter->section,parameter->section_len);
	}

	if(parameter->key_len > 0)
	{
		memcpy(key,parameter->key,parameter->key_len);
	}else{
		parameter->key_len = str_int_len(parameter->key);
		memcpy(key,parameter->key,parameter->key_len);
	}

	q = find_inifile_section(p,section,parameter->section_len);
	if(q == NULL)
	{
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}

	pk = find_keyvalue_unsect(q,key,parameter->key_len);
	if(pk == NULL)
	{
		ret = INIFILE_KEYVALUE_NOFOUND;
		goto error_out;
	}

	ret = rewrite_after_delete(p->name,parameter);
	if(ret < 0)
	{
		goto error_out;
	}
	if(q->under == pk)
	{
		q->under = pk->next;
	}else{
		ppk = q->under;
		while(ppk->next)
		{
			if(ppk->next == pk)
				break;
			ppk = ppk->next;
		}

		if(ppk->next)
		{
			ppk->next = pk->next;
		}else{
			PERROR("This suition will never be happen\n");
		}
	}

	if(pk->key)
		free(pk->key);
	if(pk->value)
		free(pk->value);
	free(pk);
	ret = 0;
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
error_out:
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
}

int delete_ini_section(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,i = 0;
	char section[INIFILE_MAX_CONTENT_LINELEN] = {0};
	INI_FILE_S *p = head;
	FILE_SECTION_NODE *q = NULL,*qq = p->under;
	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return INIFILE_NEVER_LOAD;
	}
	if(parameter->section == NULL)
		return INIFILE_ERROR_PARAMETER;
	pthread_mutex_lock(&(p->file_lock));
	if(parameter->section_len > 0)
	{
		memcpy(section,parameter->section,parameter->section_len);
	}
	else{
		parameter->section_len = str_int_len(parameter->section);
		memcpy(section,parameter->section,parameter->section_len);
	}

	q = find_inifile_section(p,section,parameter->section_len);
	if(q == NULL)
	{
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}
	ret = rewrite_after_delete(p->name,parameter);
	if(ret < 0)
	{
		goto error_out;
	}
	destroy_all_keyvalue_mem(q);

	if(p->under == q)
	{
		p->under = q->next;
	}else{
		while(qq->next)
		{
			if(qq->next == q)
				break;
			qq = qq->next;
		}

		if(qq->next)
		{
			qq->next = q->next;
		}
		else
		{
			PERROR("This suition will never be happen\n");
		}
	}

	if(q->section)
		free(q->section);
	free(q);
	ret = 0;
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
error_out:
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
}

int add_ini_section(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,i = 0,len = 0;
	char *p_section = NULL;
	INI_FILE_S *p = head;
	FILE_SECTION_NODE *q = NULL;

	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return INIFILE_NEVER_LOAD;
	}

	if(parameter->section == NULL)
		return INIFILE_ERROR_PARAMETER;

	pthread_mutex_lock(&(p->file_lock));
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
		q = NULL;
		goto error_out;
	}

	q = (FILE_SECTION_NODE *)malloc_memset(sizeof(FILE_SECTION_NODE));
	if(q == NULL)
	{
		ret = INIFILE_SYSTEM_ERROR;
		goto error_out;
	}
	q->section = p_section;
	ret = rewrite_after_add(p->name,parameter);
	if(ret < 0)
	{
		PERROR("There is something wrong sync to io file\n");
		goto error_out;
	}
	add_inifile_section(p,q);
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
error_out:
	if(p_section)
		free(p_section);
	if(q)
		free(q);
	pthread_mutex_unlock(&(p->file_lock));
	return ret;
}

int reload_ini_file(int ini_fd)
{
	int i = 0;
	char filename[INIFILE_FILEALL_NAME_LEN] = {0};
	INI_FILE_S *p = head;

	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return INIFILE_NEVER_LOAD;
	}
	strcpy(filename,p->name);

	destroy_ini_source(ini_fd);
	return init_ini_file(filename,str_int_len(filename));
	
}


void destroy_ini_source(int ini_fd)
{
	int i = 0;
	INI_FILE_S *p = head;
	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return;
	}
	
	pthread_mutex_lock(&(p->file_lock));
	destroy_all_section_mem(p);
	if(p->name)
		free(p->name);
	p->name = NULL;
	pthread_mutex_unlock(&(p->file_lock));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///@test
///
void ini_file_info_out(int ini_fd)
{
	int i = 0;
	INI_FILE_S *p = head;
	FILE_SECTION_NODE *q = NULL;
	KEY_VALUE_NODE *pk = NULL;

	PDEBUG("Test out function****************************\n");
	for(i = 0;i < ini_fd;i++)
	{
		if(p)
		{
			p = p->next;
		}else{
			break;
		}
	}
	if(p == NULL)
	{
		PERROR("The memory never load this inifile\n");
		return;
	}

	if(p->name)
		PDEBUG("ini file name ::%s \n",p->name);
	else
	{
		PDEBUG("This node :: %d inifile source had been release\n",(ini_fd + 1));
		return;
	}

	q = p->under;
	while(q)
	{
		if(q->section)
			PDEBUG("ini file section :: %s \n",q->section);
		pk = q->under;
		while(pk)
		{
			PDEBUG("ini file key value : %s = %s \n",pk->key,pk->value);
			pk = pk->next;
		}
		q = q->next;
	}
}

int main(int argc, char *argv[])
{
	int fd = 0;
	while(1)
	{
		fd = init_ini_file("redis_cluster.ini",0);
		if(fd < 0)
		{
			PERROR("init_ini_file :: %d\n",fd);
			return fd;
		}
		ini_file_info_out(fd);
		destroy_ini_source(fd);
	}
	return 0;
#if 0
	int fd = 0,ret = 0;
	char value[100] = {0};
	char get_set[] = "cluster",get_key[] = "node5";
	char add_sec[] = "xiaoxiao";
	char add_key[] = "wife",add_value[] = "yunying";
	char key[] = "daughter",val[] = "xinxin";
	char age_key[] = "age",age_val[] = "27",up_value[] = "28";
	INI_PARAMETER ini_parameter = {0};
	
	fd = init_ini_file("redis_cluster.ini",0);
	if(fd < 0)
	{
		PERROR("init_ini_file :: %d\n",fd);
		return fd;
	}
	ini_file_info_out(fd);

#if 0
	ini_parameter.section = get_set;
	ini_parameter.key = get_key;
	ini_parameter.value = value;
	ini_parameter.value_len = 100;
	ret = get_value_ofkey(fd,&ini_parameter);
	PDEBUG("get_value_ofkey :: %d\n",ret);
	if(ret == 0)
	{
		PDEBUG("get value of key value :: %s = %s\n",get_key,value);
	}
#endif

#if 0
	ini_parameter.section = add_sec;
	ret = add_ini_section(fd,&ini_parameter);
	PDEBUG("add_ini_section :: %d\n",ret);
#endif
	memset(&ini_parameter,0,sizeof(INI_PARAMETER));
	ini_parameter.section = add_sec;
	ini_parameter.key = key;
	ini_parameter.value = val;
	ret = add_value_ofkey(fd,&ini_parameter);
	PDEBUG("add_value_ofkey :: %d\n",ret);
	
#if 0
	memset(&ini_parameter,0,sizeof(INI_PARAMETER));
	ini_parameter.section = add_sec;
	ini_parameter.key = add_key;
	ini_parameter.value = add_value;
	ret = add_value_ofkey(fd,&ini_parameter);
	PDEBUG("add_value_ofkey :: %d\n",ret);

	memset(&ini_parameter,0,sizeof(INI_PARAMETER));
	ini_parameter.section = add_sec;
	ini_parameter.key = key;
	ini_parameter.value = val;
	ret = add_value_ofkey(fd,&ini_parameter);
	PDEBUG("add_value_ofkey :: %d\n",ret);


	memset(&ini_parameter,0,sizeof(INI_PARAMETER));
	ini_parameter.section = add_sec;
	ini_parameter.key = age_key;
	ini_parameter.value = age_val;
	ret = add_value_ofkey(fd,&ini_parameter);
	PDEBUG("add_value_ofkey :: %d\n",ret);

	memset(&ini_parameter,0,sizeof(INI_PARAMETER));
	ini_parameter.section = add_sec;
	ini_parameter.key = age_key;
	ini_parameter.value = up_value;
	ret = update_value_ofkey(fd,&ini_parameter);
	PDEBUG("update_value_ofkey :: %d\n",ret);

	memset(&ini_parameter,0,sizeof(INI_PARAMETER));
	ini_parameter.section = add_sec;
	ini_parameter.key = age_key;
	ini_parameter.value = up_value;
	ret = delete_value_ofkey(fd,&ini_parameter);
	PDEBUG("delete_value_ofkey :: %d\n",ret);

	memset(&ini_parameter,0,sizeof(INI_PARAMETER));
	ini_parameter.section = add_sec;
	ret = delete_ini_section(fd,&ini_parameter);
	PDEBUG("delete_ini_section :: %d\n",ret);
#endif
	ini_file_info_out(fd);
	destroy_ini_source(fd);
	return 0;
#endif
}

